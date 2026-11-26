#include <lightc/async.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/heap.h>
#include "kernel.h"          /* io_uring_sqe, IORING_*, MAP_POPULATE */

/* ========================================================================
 * Ring structure
 * ======================================================================== */

struct lc_async_ring {
    int32_t fd;

    /* SQ ring pointers (into mmap'd memory) */
    uint32_t *sq_head;
    uint32_t *sq_tail;
    uint32_t *sq_ring_mask;
    uint32_t *sq_ring_entries;
    uint32_t *sq_flags;
    uint32_t *sq_array;
    io_uring_sqe *sqes;

    /* CQ ring pointers */
    uint32_t *cq_head;
    uint32_t *cq_tail;
    uint32_t *cq_ring_mask;
    uint32_t *cq_ring_entries;
    io_uring_cqe *cqes;

    /* mmap tracking for cleanup */
    void   *sq_ring_ptr;
    size_t  sq_ring_size;
    void   *cq_ring_ptr;
    size_t  cq_ring_size;
    void   *sqes_ptr;
    size_t  sqes_size;

    /* Local submission tracking */
    uint32_t sq_pending;  /* entries written but not submitted */

    /* The errno of an io_uring_enter that failed in a call that cannot report
     * it (lc_async_flush, lc_async_wait, lc_async_peek), or 0. The next
     * lc_async_flush_ex or lc_async_wait_ex reports it and clears it. */
    int32_t error;
};

/* ========================================================================
 * Atomic helpers for shared memory ring buffers
 * ======================================================================== */

static inline uint32_t load_acquire(const uint32_t *p) {
    return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}

static inline void store_release(uint32_t *p, uint32_t v) {
    __atomic_store_n(p, v, __ATOMIC_RELEASE);
}

/* ========================================================================
 * Internal: reap completions from the CQ ring
 * ======================================================================== */

static uint32_t reap_completions(lc_async_ring *ring,
                                 lc_async_result *results, uint32_t max_results) {
    uint32_t head  = load_acquire(ring->cq_head);
    uint32_t tail  = load_acquire(ring->cq_tail);
    uint32_t count = 0;

    while (head != tail && count < max_results) {
        io_uring_cqe *cqe = &ring->cqes[head & *ring->cq_ring_mask];
        results[count].tag    = cqe->user_data;
        results[count].result = cqe->res;
        count++;
        head++;
    }

    store_release(ring->cq_head, head);
    return count;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

lc_result_ptr lc_async_ring_create(uint32_t queue_size) {
    io_uring_params params;
    lc_bytes_fill(&params, 0, sizeof(params));

    /* Ask the kernel to create an io_uring instance */
    lc_sysret ret = lc_kernel_io_ring_setup(queue_size, &params);
    if (ret < 0) return lc_err_ptr((int32_t)(-ret));
    int32_t ring_fd = (int32_t)ret;

    /* Without NODROP (Linux 5.5) a full CQ drops completions, and an operation
     * whose completion is dropped never ends. Every kernel with IORING_OP_READ
     * (5.6) has it. */
    if ((params.features & IORING_FEAT_NODROP) == 0) {
        lc_kernel_close_file(ring_fd);
        return lc_err_ptr(LC_ERR_OPNOTSUPP);
    }

    /* Calculate mmap sizes */
    size_t sq_ring_size = params.sq_off.array + params.sq_entries * sizeof(uint32_t);
    size_t cq_ring_size = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);
    size_t sqes_size    = params.sq_entries * sizeof(io_uring_sqe);

    /* If kernel supports single mmap, the SQ and CQ rings share one mapping.
     * Use the larger of the two sizes. */
    bool single_mmap = (params.features & IORING_FEAT_SINGLE_MMAP) != 0;
    if (single_mmap) {
        if (cq_ring_size > sq_ring_size) {
            sq_ring_size = cq_ring_size;
        }
        cq_ring_size = sq_ring_size;
    }

    /* mmap the SQ ring */
    void *sq_ring_ptr = lc_kernel_map_memory(
        NULL, sq_ring_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        ring_fd, (int64_t)IORING_OFF_SQ_RING
    );
    if (sq_ring_ptr == MAP_FAILED) {
        lc_kernel_close_file(ring_fd);
        return lc_err_ptr(LC_ERR_NOMEM);
    }

    /* mmap the CQ ring (or reuse SQ mapping if single mmap) */
    void *cq_ring_ptr;
    if (single_mmap) {
        cq_ring_ptr = sq_ring_ptr;
    } else {
        cq_ring_ptr = lc_kernel_map_memory(
            NULL, cq_ring_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE,
            ring_fd, (int64_t)IORING_OFF_CQ_RING
        );
        if (cq_ring_ptr == MAP_FAILED) {
            lc_kernel_unmap_memory(sq_ring_ptr, sq_ring_size);
            lc_kernel_close_file(ring_fd);
            return lc_err_ptr(LC_ERR_NOMEM);
        }
    }

    /* mmap the SQE array */
    void *sqes_ptr = lc_kernel_map_memory(
        NULL, sqes_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        ring_fd, (int64_t)IORING_OFF_SQES
    );
    if (sqes_ptr == MAP_FAILED) {
        lc_kernel_unmap_memory(sq_ring_ptr, sq_ring_size);
        if (!single_mmap) {
            lc_kernel_unmap_memory(cq_ring_ptr, cq_ring_size);
        }
        lc_kernel_close_file(ring_fd);
        return lc_err_ptr(LC_ERR_NOMEM);
    }

    /* Allocate the ring struct */
    lc_result_ptr alloc = lc_heap_allocate(sizeof(lc_async_ring));
    if (lc_ptr_is_err(alloc)) {
        lc_kernel_unmap_memory(sqes_ptr, sqes_size);
        lc_kernel_unmap_memory(sq_ring_ptr, sq_ring_size);
        if (!single_mmap) {
            lc_kernel_unmap_memory(cq_ring_ptr, cq_ring_size);
        }
        lc_kernel_close_file(ring_fd);
        return lc_err_ptr(LC_ERR_NOMEM);
    }
    lc_async_ring *ring = alloc.value;

    ring->fd = ring_fd;

    /* Set up SQ ring pointers using offsets from params */
    uint8_t *sq_base = (uint8_t *)sq_ring_ptr;
    ring->sq_head         = (uint32_t *)(sq_base + params.sq_off.head);
    ring->sq_tail         = (uint32_t *)(sq_base + params.sq_off.tail);
    ring->sq_ring_mask    = (uint32_t *)(sq_base + params.sq_off.ring_mask);
    ring->sq_ring_entries = (uint32_t *)(sq_base + params.sq_off.ring_entries);
    ring->sq_flags        = (uint32_t *)(sq_base + params.sq_off.flags);
    ring->sq_array        = (uint32_t *)(sq_base + params.sq_off.array);
    ring->sqes            = (io_uring_sqe *)sqes_ptr;

    /* Set up CQ ring pointers */
    uint8_t *cq_base = (uint8_t *)cq_ring_ptr;
    ring->cq_head         = (uint32_t *)(cq_base + params.cq_off.head);
    ring->cq_tail         = (uint32_t *)(cq_base + params.cq_off.tail);
    ring->cq_ring_mask    = (uint32_t *)(cq_base + params.cq_off.ring_mask);
    ring->cq_ring_entries = (uint32_t *)(cq_base + params.cq_off.ring_entries);
    ring->cqes            = (io_uring_cqe *)(cq_base + params.cq_off.cqes);

    /* Track mmap regions for cleanup */
    ring->sq_ring_ptr  = sq_ring_ptr;
    ring->sq_ring_size = sq_ring_size;
    ring->cq_ring_ptr  = cq_ring_ptr;
    ring->cq_ring_size = cq_ring_size;
    ring->sqes_ptr     = sqes_ptr;
    ring->sqes_size    = sqes_size;

    ring->sq_pending = 0;
    ring->error      = 0;

    /* Initialize sq_array to identity mapping: sq_array[i] = i */
    for (uint32_t i = 0; i < params.sq_entries; i++) {
        ring->sq_array[i] = i;
    }

    return lc_ok_ptr(ring);
}

void lc_async_ring_destroy(lc_async_ring *ring) {
    if (ring == NULL) return;

    /* Unmap SQE array */
    lc_kernel_unmap_memory(ring->sqes_ptr, ring->sqes_size);

    /* Unmap CQ ring (only if it's a separate mapping) */
    if (ring->cq_ring_ptr != ring->sq_ring_ptr) {
        lc_kernel_unmap_memory(ring->cq_ring_ptr, ring->cq_ring_size);
    }

    /* Unmap SQ ring */
    lc_kernel_unmap_memory(ring->sq_ring_ptr, ring->sq_ring_size);

    /* Close the ring fd */
    lc_kernel_close_file(ring->fd);

    /* Free the struct */
    lc_heap_free(ring);
}

/* Internal: prepare an SQE for a read or write operation */
static lc_result submit_rw(lc_async_ring *ring, uint8_t opcode, int32_t fd,
                           uint64_t addr, uint32_t len, uint64_t offset, uint64_t tag) {
    /* Check if SQ is full */
    uint32_t tail    = load_acquire(ring->sq_tail);
    uint32_t head    = load_acquire(ring->sq_head);
    uint32_t entries = *ring->sq_ring_entries;
    uint32_t mask    = *ring->sq_ring_mask;

    if (tail + ring->sq_pending - head >= entries) {
        return lc_err(LC_ERR_AGAIN);  /* queue is full */
    }

    /* Get the next SQE slot */
    uint32_t index = (tail + ring->sq_pending) & mask;
    io_uring_sqe *sqe = &ring->sqes[index];

    /* Zero the SQE */
    lc_bytes_fill(sqe, 0, sizeof(*sqe));

    /* Fill in the operation */
    sqe->opcode    = opcode;
    sqe->fd        = fd;
    sqe->addr      = addr;
    sqe->len       = len;
    sqe->off       = offset;
    sqe->user_data = tag;

    /* Update the sq_array entry */
    ring->sq_array[(tail + ring->sq_pending) & mask] = index;

    ring->sq_pending++;
    return lc_ok(0);
}

lc_result lc_async_submit_read(lc_async_ring *ring, int32_t fd,
                               void *buf, uint32_t count,
                               uint64_t offset, uint64_t tag) {
    return submit_rw(ring, IORING_OP_READ, fd,
                     (uint64_t)(uintptr_t)buf, count, offset, tag);
}

lc_result lc_async_submit_write(lc_async_ring *ring, int32_t fd,
                                const void *buf, uint32_t count,
                                uint64_t offset, uint64_t tag) {
    return submit_rw(ring, IORING_OP_WRITE, fd,
                     (uint64_t)(uintptr_t)buf, count, offset, tag);
}

/* Publish the staged SQEs to the kernel by advancing the shared tail. The
 * release pairs with the kernel's acquire, so the SQEs are written before it
 * sees the tail. Once published they are the kernel's, between its sq_head and
 * sq_tail, whether or not an enter takes them, so sq_pending is cleared: kept,
 * the next flush would advance sq_tail by it a second time. */
static void publish(lc_async_ring *ring) {
    if (ring->sq_pending == 0) return;
    store_release(ring->sq_tail, *ring->sq_tail + ring->sq_pending);
    ring->sq_pending = 0;
}

/* The published SQEs the kernel has not taken yet. These include any an
 * earlier enter left, one that failed with EAGAIN or EBUSY: each enter offers
 * them all again, so none is stranded. */
static uint32_t unconsumed(const lc_async_ring *ring) {
    return *ring->sq_tail - load_acquire(ring->sq_head);
}

/* An errno the caller did not get, kept for the next _ex call. */
static void keep_error(lc_async_ring *ring, lc_sysret ret) {
    if (ret < 0 && ret != -LC_ERR_INTR) ring->error = (int32_t)-ret;
}

static int32_t take_error(lc_async_ring *ring) {
    int32_t error = ring->error;
    ring->error = 0;
    return error;
}

/* Submit every published SQE. An enter may take fewer than offered, so it
 * loops while the kernel makes progress; it retries EINTR only. Any other
 * error, EAGAIN and EBUSY too, returns at once: those clear when completions
 * are reaped, which the caller must do, so retrying here would spin. What the
 * kernel did not take stays published, for the next flush or wait. */
static lc_result submit(lc_async_ring *ring) {
    publish(ring);
    uint32_t submitted = 0;
    for (;;) {
        uint32_t offered = unconsumed(ring);
        if (offered == 0) break;
        lc_sysret ret = lc_kernel_io_ring_enter(ring->fd, offered, 0, 0);
        if (ret == -LC_ERR_INTR) continue;
        if (ret < 0) return (lc_result){ .value = submitted, .error = (int32_t)-ret };
        if (ret == 0) break;
        submitted += (uint32_t)ret;
    }
    return (lc_result){ .value = submitted, .error = 0 };
}

uint32_t lc_async_flush(lc_async_ring *ring) {
    lc_result r = submit(ring);
    if (r.error != 0) ring->error = r.error;
    return (uint32_t)r.value;
}

lc_result lc_async_flush_ex(lc_async_ring *ring) {
    int32_t kept = take_error(ring);
    lc_result r = submit(ring);
    if (r.error == 0) r.error = kept;
    return r;
}

/* Submit what is published, then wait for one completion with an enter that
 * offers again what the kernel left. Refusing them again, the kernel returns
 * its error without sleeping, so a wait never sleeps on SQEs it did not
 * submit. An error of the submit either recurs in that enter or has cleared,
 * so the enter's result is the one to report. */
static lc_sysret submit_and_wait(lc_async_ring *ring) {
    (void)submit(ring);
    return lc_kernel_io_ring_enter(ring->fd, unconsumed(ring), 1, IORING_ENTER_GETEVENTS);
}

uint32_t lc_async_wait(lc_async_ring *ring,
                       lc_async_result *results, uint32_t max_results) {
    lc_sysret ret;
    do {
        ret = submit_and_wait(ring);
    } while (ret == -LC_ERR_INTR);
    keep_error(ring, ret);
    return reap_completions(ring, results, max_results);
}

lc_result lc_async_wait_ex(lc_async_ring *ring,
                           lc_async_result *results, uint32_t max_results) {
    /* A kept error is reported now, with what has completed, and nothing
     * waits: the caller learns of it before it sleeps. */
    int32_t error = take_error(ring);
    if (error == 0) {
        lc_sysret ret = submit_and_wait(ring);
        if (ret < 0 && ret != -LC_ERR_INTR) error = (int32_t)-ret;
    }
    uint32_t n = reap_completions(ring, results, max_results);
    return (lc_result){ .value = n, .error = error };
}

uint32_t lc_async_peek(lc_async_ring *ring,
                       lc_async_result *results, uint32_t max_results) {
    /* Completions the CQ had no room for wait in the kernel's overflow list,
     * and only an enter with GETEVENTS moves them to the CQ, as room allows:
     * without it a peek-driven loop sees none of them, nor any later one. */
    if (load_acquire(ring->sq_flags) & IORING_SQ_CQ_OVERFLOW)
        keep_error(ring, lc_kernel_io_ring_enter(ring->fd, 0, 0, IORING_ENTER_GETEVENTS));
    return reap_completions(ring, results, max_results);
}

lc_result lc_async_submit_raw(lc_async_ring *ring, uint8_t opcode, int32_t fd,
                              uint64_t addr, uint32_t len, uint64_t offset,
                              uint32_t op_flags, uint64_t tag) {
    /* Check if SQ is full */
    uint32_t tail    = load_acquire(ring->sq_tail);
    uint32_t head    = load_acquire(ring->sq_head);
    uint32_t entries = *ring->sq_ring_entries;
    uint32_t mask    = *ring->sq_ring_mask;

    if (tail + ring->sq_pending - head >= entries) {
        return lc_err(LC_ERR_AGAIN);  /* queue is full */
    }

    /* Get the next SQE slot */
    uint32_t index = (tail + ring->sq_pending) & mask;
    io_uring_sqe *sqe = &ring->sqes[index];

    /* Zero the SQE */
    lc_bytes_fill(sqe, 0, sizeof(*sqe));

    /* Fill in all fields */
    sqe->opcode    = opcode;
    sqe->fd        = fd;
    sqe->addr      = addr;
    sqe->len       = len;
    sqe->off       = offset;
    sqe->rw_flags  = op_flags;
    sqe->user_data = tag;

    /* Update the sq_array entry */
    ring->sq_array[(tail + ring->sq_pending) & mask] = index;

    ring->sq_pending++;
    return lc_ok(0);
}

uint32_t lc_async_get_free_slots(const lc_async_ring *ring) {
    uint32_t entries = *ring->sq_ring_entries;
    uint32_t tail    = load_acquire(ring->sq_tail);
    uint32_t head    = load_acquire(ring->sq_head);
    uint32_t used    = tail - head + ring->sq_pending;
    if (used >= entries) return 0;
    return entries - used;
}
