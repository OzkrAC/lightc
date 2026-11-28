/*
 * HEAP-thread-exit-leak — a thread that exits gives its heap state back.
 *
 * Each thread that allocates gets a heap local, a mapping with its lists of
 * 64 KiB slab pages, one list per size class. The heap kept all of it when
 * the thread exited: the local, the one emptied page per size class that
 * it keeps resident, and every page that still held a block. A block that
 * another thread freed into such a page, as a joiner frees the result its
 * worker allocated, went onto the page's list of remote frees, which only
 * the page's owner ever drains, so the page was never used again. Every
 * create/join cycle of a thread that allocated left memory behind that
 * nothing could reclaim.
 *
 * The entry of a thread of lc_thread_create now calls lc_heap_thread_exit
 * once the thread's function has returned. It drains the remote frees of
 * the thread's pages, returns the empty ones to their segments, abandons
 * the ones that still hold blocks and unmaps the local. An allocation of
 * another thread adopts an abandoned page of its size class that has a
 * quarter of its blocks free before it takes a fresh page, and the free
 * that empties an abandoned page returns it, at once, or, when another
 * thread holds the heap's lock at that moment, by the time that thread has
 * released it.
 *
 * test_create_join_cycles runs 10,000 cycles of a worker that allocates and
 * frees blocks of five size classes and a large block, and returns two
 * blocks that the joiner frees after the join. After the last cycle there
 * must be no more than 4 mappings more than after the tenth, and VmRSS
 * must be within 1 MiB of what it was then (test_create_join_cycles_rss).
 * Before the fix, on x86_64, VmRSS went from 512 kB after 10 cycles to
 * 422 MiB after 10,000, and the mappings from 10 to 1113.
 *
 * test_producer_rounds runs rounds of 8 threads that each allocate 1000
 * blocks of 1000 bytes and exit, holding them; main frees every block after
 * the joins. Main's frees into the abandoned pages are applied as it makes
 * them, and the free of a page's last block returns the page, so the mapping
 * count after round 12 is within 4 of round 2's, and VmRSS within 1 MiB
 * (test_producer_rounds_rss); and once main has freed the last round's
 * blocks and one more thread has exited, VmRSS is within 1 MiB of what it
 * was before round 1. Before the fix each round took fresh pages, and no
 * page of an exited thread came back.
 *
 * test_partial_frees_are_adopted runs 12 rounds of 4 threads that each
 * allocate 512 blocks of 1000 bytes and exit; main frees three in four of
 * each round's blocks, and keeps the rest to the end. The next round's
 * threads adopt the pages that leaves three quarters free, so the 6144 kept
 * blocks end up in no more than twice the 96 pages they fill; without
 * adoption each round would take fresh pages, and they would be in 384. It
 * counts the pages from the blocks' addresses, not from VmRSS, so it runs
 * under qemu-user too.
 *
 * test_live_owner_exit_rss: a thread allocates 50,000 blocks of 48 bytes,
 * main frees them all while the thread is still running, and the thread
 * then exits. Its exit drains those frees and returns the pages, so after
 * the join VmRSS is within 1 MiB of what it was before the thread started.
 * Before the fix the pages stayed resident.
 *
 * test_reused_id_frees_remotely checks what the fix must keep true when the
 * kernel gives an exited thread's id out again: an abandoned page has no
 * owner, so the new thread of that id frees a block of it as a remote free,
 * as any other thread does, instead of into its bitmap as if it owned the
 * page, which would race with whoever adopts the page and would return it
 * to its segment while it is still on the abandoned list. Marked threads
 * each allocate a block of a size class of their own and exit holding it;
 * probe threads then run, one at a time, until the ids have wrapped round
 * and some probes got the id of a marked thread, which free that thread's
 * block. The debug heap stops the program at such a free, and a release
 * build breaks when a thread then allocates from the classes. As in
 * HEAP-tid-reuse-tls, the ids wrap in a second or two at a kernel.pid_max
 * of 32768, and the test skips above that.
 *
 * Under qemu-user, VmRSS in /proc/self/status is the emulator's, which
 * counts memory of its own that the program's mappings in /proc/self/maps
 * do not hold. The program tells that from VmSize: when it is more than
 * 64 MiB above the size of the mappings /proc/self/maps lists, the three
 * VmRSS tests skip and say so. The mapping counts are the program's own
 * there too, as qemu-user lists only the guest's, so the other tests run.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define RSS_SLACK_KB   1024    /* VmRSS may end this much above the baseline */
#define MAPS_SLACK     4       /* and the mapping count this much */
#define EMULATOR_KB    (64 * 1024)   /* VmSize above the mappings that says VmRSS is not the program's */
#define WAIT_LIMIT_MS  10000   /* a bound for each wait for another thread */

/* --- What /proc says of the process --- */

typedef struct {
    int64_t rss_kb;    /* VmRSS */
    int64_t size_kb;   /* VmSize */
    int64_t maps;      /* lines of /proc/self/maps */
    int64_t maps_kb;   /* the size of those mappings */
} footprint;

/* Large enough for /proc/self/maps of this program, and only ever touched
 * as far as a read fills it, the same each time. */
static char proc_text[64 * 1024];

static int64_t read_proc(const char *path) {
    lc_sysret fd = lc_kernel_open_file(path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    lc_sysret n = lc_kernel_read_all((int32_t)fd, proc_text, sizeof(proc_text) - 1);
    lc_kernel_close_file((int32_t)fd);
    if (n < 0 || (size_t)n >= sizeof(proc_text) - 1) return -1;
    proc_text[n] = '\0';
    return n;
}

/* The number in kB after `name` in proc_text, as "VmRSS:\t  1234 kB". */
static int64_t status_field(const char *name) {
    size_t name_length = lc_string_length(name);
    for (const char *line = proc_text; *line != '\0'; ) {
        if (lc_bytes_compare(line, name, name_length) == 0 && line[name_length] == ':') {
            const char *c = line + name_length + 1;
            while (*c == ' ' || *c == '\t') c++;
            int64_t value = 0;
            if (*c < '0' || *c > '9') return -1;
            for (; *c >= '0' && *c <= '9'; c++) value = value * 10 + (*c - '0');
            return value;
        }
        while (*line != '\0' && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    return -1;
}

static uint64_t hex_number(const char **text) {
    uint64_t value = 0;
    for (const char *c = *text; ; c++) {
        int digit = *c >= '0' && *c <= '9' ? *c - '0'
                  : *c >= 'a' && *c <= 'f' ? *c - 'a' + 10 : -1;
        if (digit < 0) {
            *text = c;
            return value;
        }
        value = value * 16 + (uint64_t)digit;
    }
}

/* Fills *f, or returns false when /proc cannot be read. */
static bool measure(footprint *f) {
    if (read_proc("/proc/self/status") < 0) return false;
    f->rss_kb = status_field("VmRSS");
    f->size_kb = status_field("VmSize");
    if (f->rss_kb < 0 || f->size_kb < 0) return false;

    if (read_proc("/proc/self/maps") < 0) return false;
    f->maps = 0;
    uint64_t bytes = 0;
    for (const char *line = proc_text; *line != '\0'; ) {
        const char *c = line;
        uint64_t start = hex_number(&c);
        if (*c == '-') {
            c++;
            uint64_t end = hex_number(&c);
            if (end > start) bytes += end - start;
        }
        f->maps++;
        while (*line != '\0' && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    f->maps_kb = (int64_t)(bytes / 1024);
    return true;
}

/* Whether VmRSS counts the program's memory only: not under an emulator,
 * whose own mappings VmSize counts and /proc/self/maps does not list. */
static bool rss_is_the_programs(const footprint *f) {
    return f->size_kb <= f->maps_kb + EMULATOR_KB;
}

static void print_footprint(const char *when, const footprint *f) {
    _test_print_text("  ");
    _test_print_text(when);
    _test_print_text(": VmRSS ");
    lc_print_signed(TEST_FD, f->rss_kb);
    _test_print_text(" kB, ");
    lc_print_signed(TEST_FD, f->maps);
    _test_print_text(" mappings\n");
}

#define RSS_SKIP_REASON "VmRSS is not this program's alone: VmSize is more than 64 MiB above " \
                        "the mappings of /proc/self/maps, as under qemu-user"

/* Waits until *flag is `value`, for at most WAIT_LIMIT_MS. */
static bool wait_for(_Atomic(int32_t) *flag, int32_t value) {
    int64_t start = lc_time_start_timer();
    while (atomic_load_explicit(flag, memory_order_acquire) != value) {
        if (lc_time_elapsed_milliseconds(start) > WAIT_LIMIT_MS) return false;
        lc_time_sleep_microseconds(100);
    }
    return true;
}

/* --- Create/join cycles, with results the joiner frees --- */

#define CYCLES        10000
#define FIRST_CYCLES  10
#define CLEAN_BLOCKS  4
#define LARGE_SIZE    20000   /* a large block: its own mapping, cached on free */

static const size_t clean_sizes[] = { 24, 100, 300, 1000, 3000 };
static const size_t result_sizes[] = { 200, 2000 };
#define CLEAN_CLASSES (sizeof(clean_sizes) / sizeof(clean_sizes[0]))
#define RESULTS       (sizeof(result_sizes) / sizeof(result_sizes[0]))

typedef struct {
    uint8_t *result[RESULTS];   /* allocated by the worker, freed by the joiner */
    int32_t  failures;
} cycle_work;

/* Marks a block with `value` at its first, middle and last byte, which
 * touches the pages it is on, as filling it would, at less cost. */
static void mark(uint8_t *block, size_t size, uint8_t value) {
    block[0] = value;
    block[size / 2] = value;
    block[size - 1] = value;
}

static bool marked(const uint8_t *block, size_t size, uint8_t value) {
    return block[0] == value && block[size / 2] == value && block[size - 1] == value;
}

static int32_t cycle_worker(void *arg) {
    cycle_work *w = arg;
    uint8_t *blocks[CLEAN_BLOCKS];
    for (size_t c = 0; c < CLEAN_CLASSES; c++) {
        size_t size = clean_sizes[c];
        for (int i = 0; i < CLEAN_BLOCKS; i++) {
            lc_result_ptr r = lc_heap_allocate(size);
            blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
            if (blocks[i] == NULL) w->failures++;
            else mark(blocks[i], size, (uint8_t)(c * CLEAN_BLOCKS + (size_t)i));
        }
        for (int i = 0; i < CLEAN_BLOCKS; i++) {
            if (blocks[i] == NULL) continue;
            if (!marked(blocks[i], size, (uint8_t)(c * CLEAN_BLOCKS + (size_t)i))) w->failures++;
            lc_heap_free(blocks[i]);
        }
    }
    lc_result_ptr large = lc_heap_allocate(LARGE_SIZE);
    if (lc_ptr_is_err(large)) w->failures++;
    else {
        mark(large.value, LARGE_SIZE, 0x5C);
        lc_heap_free(large.value);
    }
    for (size_t k = 0; k < RESULTS; k++) {
        lc_result_ptr r = lc_heap_allocate(result_sizes[k]);
        w->result[k] = lc_ptr_is_ok(r) ? r.value : NULL;
        if (w->result[k] == NULL) w->failures++;
        else mark(w->result[k], result_sizes[k], (uint8_t)(0xA0 + k));
    }
    return 0;
}

static footprint cycles_first, cycles_last;
static bool cycles_measured;

static void test_create_join_cycles(void) {
    for (int k = 1; k <= CYCLES; k++) {
        cycle_work w = { 0 };
        lc_thread thread;
        TEST_REQUIRE_OK(lc_thread_create(&thread, cycle_worker, &w));
        lc_thread_join(&thread);
        TEST_REQUIRE_EQ(w.failures, 0);
        for (size_t r = 0; r < RESULTS; r++) {
            TEST_REQUIRE_NOT_NULL(w.result[r]);
            TEST_REQUIRE(marked(w.result[r], result_sizes[r], (uint8_t)(0xA0 + r)));
            lc_heap_free(w.result[r]);   /* into a page of a thread that has exited */
        }
        if (k == FIRST_CYCLES) TEST_REQUIRE(measure(&cycles_first));
    }
    TEST_REQUIRE(measure(&cycles_last));
    cycles_measured = true;

    print_footprint("after 10 cycles", &cycles_first);
    print_footprint("after 10000 cycles", &cycles_last);
    TEST_ASSERT(cycles_last.maps - cycles_first.maps <= MAPS_SLACK);
}

static void test_create_join_cycles_rss(void) {
    TEST_REQUIRE(cycles_measured);
    if (!rss_is_the_programs(&cycles_last)) TEST_SKIP(RSS_SKIP_REASON);
    TEST_ASSERT(cycles_last.rss_kb - cycles_first.rss_kb <= RSS_SLACK_KB);
}

/* Uses the heap and exits: what the tests measure after it must not wait
 * for such an exit, which a program may never make. */
static int32_t collector(void *arg) {
    (void)arg;
    lc_result_ptr r = lc_heap_allocate(48);
    if (lc_ptr_is_ok(r)) lc_heap_free(r.value);
    return 0;
}

/* Runs collector in a thread, and returns when it has exited. */
static bool run_collector(void) {
    lc_thread thread;
    if (lc_is_err(lc_thread_create(&thread, collector, NULL))) return false;
    lc_thread_join(&thread);
    return true;
}

/* --- Producers that exit holding their blocks, which main frees --- */

#define PRODUCERS        8
#define PRODUCER_BLOCKS  1000
#define PRODUCER_SIZE    1000
#define ROUNDS           12
#define FIRST_ROUNDS     2

typedef struct {
    uint8_t *blocks[PRODUCER_BLOCKS];
    uint32_t count;     /* how many blocks to allocate, up to PRODUCER_BLOCKS */
    uint8_t  tag;
    int32_t  failures;
} production;

static production produced[PRODUCERS];

static int32_t producer(void *arg) {
    production *p = arg;
    for (uint32_t i = 0; i < p->count; i++) {
        lc_result_ptr r = lc_heap_allocate(PRODUCER_SIZE);
        p->blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
        if (p->blocks[i] == NULL) p->failures++;
        else mark(p->blocks[i], PRODUCER_SIZE, (uint8_t)(p->tag + i));
    }
    return 0;
}

static footprint rounds_start, rounds_first, rounds_last, rounds_end;
static bool rounds_measured;

static void test_producer_rounds(void) {
    TEST_REQUIRE(measure(&rounds_start));
    for (int round = 1; round <= ROUNDS; round++) {
        lc_thread threads[PRODUCERS];
        int started = 0;
        for (int k = 0; k < PRODUCERS; k++) {
            produced[k].count = PRODUCER_BLOCKS;
            produced[k].tag = (uint8_t)(round * PRODUCERS + k);
            produced[k].failures = 0;
            lc_result r = lc_thread_create(&threads[k], producer, &produced[k]);
            TEST_ASSERT_OK(r);
            if (lc_is_ok(r)) started++;
        }
        for (int k = 0; k < started; k++) lc_thread_join(&threads[k]);
        TEST_REQUIRE_EQ(started, PRODUCERS);
        /* Every block holds what its producer wrote: no two producers were
         * given one block, whatever page each adopted. */
        for (int k = 0; k < PRODUCERS; k++) {
            TEST_REQUIRE_EQ(produced[k].failures, 0);
            for (int i = 0; i < PRODUCER_BLOCKS; i++) {
                TEST_REQUIRE(marked(produced[k].blocks[i], PRODUCER_SIZE,
                                    (uint8_t)(produced[k].tag + i)));
                lc_heap_free(produced[k].blocks[i]);
            }
        }
        if (round == FIRST_ROUNDS) TEST_REQUIRE(measure(&rounds_first));
    }
    TEST_REQUIRE(measure(&rounds_last));
    TEST_REQUIRE(run_collector());
    TEST_REQUIRE(measure(&rounds_end));
    rounds_measured = true;

    print_footprint("before round 1", &rounds_start);
    print_footprint("after round 2", &rounds_first);
    print_footprint("after round 12", &rounds_last);
    print_footprint("after one more thread exit", &rounds_end);
    TEST_ASSERT(rounds_last.maps - rounds_first.maps <= MAPS_SLACK);
}

static void test_producer_rounds_rss(void) {
    TEST_REQUIRE(rounds_measured);
    if (!rss_is_the_programs(&rounds_last)) TEST_SKIP(RSS_SKIP_REASON);
    TEST_ASSERT(rounds_last.rss_kb - rounds_first.rss_kb <= RSS_SLACK_KB);
    TEST_ASSERT(rounds_end.rss_kb - rounds_start.rss_kb <= RSS_SLACK_KB);
}

/* --- Partly freed pages of exited threads --- */

#define PARTIAL_PRODUCERS 4
#define PARTIAL_BLOCKS    512
#define PARTIAL_ROUNDS    12
#define PARTIAL_KEPT      (PARTIAL_ROUNDS * PARTIAL_PRODUCERS * PARTIAL_BLOCKS / 4)
#define BLOCKS_PER_PAGE   64      /* of PRODUCER_SIZE, in the 1024-byte class */
#define PAGE_SHIFT        16      /* a page is 64 KiB, aligned to its size */
#define PAGE_SET_SIZE     16384   /* a power of two above twice PARTIAL_KEPT */

static uint8_t *partial_kept[PARTIAL_KEPT];
static uintptr_t page_set[PAGE_SET_SIZE];

/* How many distinct pages the `count` blocks of `blocks` are in. */
static uint32_t distinct_pages(uint8_t *const *blocks, uint32_t count) {
    lc_bytes_fill(page_set, 0, sizeof(page_set));
    uint32_t pages = 0;
    for (uint32_t i = 0; i < count; i++) {
        uintptr_t page = ((uintptr_t)blocks[i] >> PAGE_SHIFT) + 1;   /* never 0 */
        uint32_t slot = (uint32_t)(page * 0x9E3779B97F4A7C15ull >> 50) & (PAGE_SET_SIZE - 1);
        while (page_set[slot] != 0 && page_set[slot] != page) slot = (slot + 1) & (PAGE_SET_SIZE - 1);
        if (page_set[slot] == 0) {
            page_set[slot] = page;
            pages++;
        }
    }
    return pages;
}

static void test_partial_frees_are_adopted(void) {
    uint32_t kept = 0;
    for (int round = 1; round <= PARTIAL_ROUNDS; round++) {
        lc_thread threads[PARTIAL_PRODUCERS];
        int started = 0;
        for (int k = 0; k < PARTIAL_PRODUCERS; k++) {
            produced[k].count = PARTIAL_BLOCKS;
            produced[k].tag = (uint8_t)(round * PARTIAL_PRODUCERS + k);
            produced[k].failures = 0;
            lc_result r = lc_thread_create(&threads[k], producer, &produced[k]);
            TEST_ASSERT_OK(r);
            if (lc_is_ok(r)) started++;
        }
        for (int k = 0; k < started; k++) lc_thread_join(&threads[k]);
        TEST_REQUIRE_EQ(started, PARTIAL_PRODUCERS);
        for (int k = 0; k < PARTIAL_PRODUCERS; k++) {
            TEST_REQUIRE_EQ(produced[k].failures, 0);
            for (uint32_t i = 0; i < PARTIAL_BLOCKS; i++) {
                uint8_t *block = produced[k].blocks[i];
                TEST_REQUIRE(marked(block, PRODUCER_SIZE, (uint8_t)(produced[k].tag + i)));
                if (i % 4 == 0) partial_kept[kept++] = block;
                else lc_heap_free(block);
            }
        }
    }
    uint32_t pages = distinct_pages(partial_kept, kept);
    _test_print_text("  ");
    lc_print_unsigned(TEST_FD, kept);
    _test_print_text(" kept blocks are in ");
    lc_print_unsigned(TEST_FD, pages);
    _test_print_text(" pages; they fill ");
    lc_print_unsigned(TEST_FD, kept / BLOCKS_PER_PAGE);
    lc_print_newline(TEST_FD);
    TEST_ASSERT(pages <= 2 * (kept / BLOCKS_PER_PAGE));
    for (uint32_t i = 0; i < kept; i++) lc_heap_free(partial_kept[i]);
}

/* --- An owner that is still running when its blocks are freed --- */

#define LIVE_BLOCKS 50000
#define LIVE_SIZE   48

static uint8_t *live_blocks[LIVE_BLOCKS];
static _Atomic(int32_t) live_phase;   /* 1: allocated; 2: main freed them */
static _Atomic(int32_t) live_failures;

static int32_t live_owner(void *arg) {
    (void)arg;
    for (int i = 0; i < LIVE_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(LIVE_SIZE);
        live_blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
        if (live_blocks[i] == NULL) atomic_fetch_add(&live_failures, 1);
        else mark(live_blocks[i], LIVE_SIZE, 0x3C);
    }
    atomic_store_explicit(&live_phase, 1, memory_order_release);
    if (!wait_for(&live_phase, 2)) atomic_fetch_add(&live_failures, 1);
    /* The owner goes on in another size class, so no allocation of its
     * drains the frees above: only its exit does. */
    for (int i = 0; i < 100; i++) {
        lc_result_ptr r = lc_heap_allocate(500);
        if (lc_ptr_is_ok(r)) lc_heap_free(r.value);
    }
    return 0;
}

static void test_live_owner_exit_rss(void) {
    footprint before, after;
    lc_bytes_fill(live_blocks, 0, sizeof(live_blocks));   /* resident in both */
    TEST_REQUIRE(run_collector());
    TEST_REQUIRE(measure(&before));
    if (!rss_is_the_programs(&before)) TEST_SKIP(RSS_SKIP_REASON);

    lc_thread thread;
    TEST_REQUIRE_OK(lc_thread_create(&thread, live_owner, NULL));
    bool allocated = wait_for(&live_phase, 1);
    TEST_ASSERT(allocated);
    if (allocated) {
        for (int i = 0; i < LIVE_BLOCKS; i++)
            if (live_blocks[i] != NULL) lc_heap_free(live_blocks[i]);   /* remote frees */
    }
    atomic_store_explicit(&live_phase, 2, memory_order_release);
    lc_thread_join(&thread);
    TEST_ASSERT_EQ(atomic_load(&live_failures), 0);
    TEST_REQUIRE(measure(&after));

    print_footprint("before the owner started", &before);
    print_footprint("after its join", &after);
    TEST_ASSERT(after.rss_kb - before.rss_kb <= RSS_SLACK_KB);
}

/* --- A thread that gets the id of one that exited holding blocks --- */

#define PID_MAX_PATH  "/proc/sys/kernel/pid_max"
#define PID_MAX_LIMIT 32768     /* the largest kernel.pid_max the test runs at */
#define MARKED        16        /* threads that exit holding a block */
#define WANTED        8         /* probes with a marked id that the test waits for */
#define REUSE_LIMIT_MS 60000    /* a bound for a slow emulator; normally ~1 s */

/* One size class each, none of them used by the tests above, so that each
 * marked thread's page holds only its own block and has had no other owner. */
static const size_t marked_sizes[MARKED] = {
    16, 64, 80, 96, 144, 176, 240, 368, 432, 496, 752, 880, 1264, 1520, 1776, 2544,
};

typedef struct {
    size_t           size;
    int32_t          tid;
    _Atomic(void *)  block;   /* until a probe of its id, or main, frees it */
} marked_thread;

static marked_thread marked_threads[MARKED];
static int16_t marked_by_tid[PID_MAX_LIMIT];   /* 1 + the index in marked_threads[], or 0 */

static int32_t marker(void *arg) {
    marked_thread *m = arg;
    m->tid = lc_kernel_get_thread_id();
    lc_result_ptr r = lc_heap_allocate(m->size);
    if (lc_ptr_is_ok(r)) mark(r.value, m->size, 0x77);
    atomic_store(&m->block, lc_ptr_is_ok(r) ? r.value : NULL);
    return 0;
}

typedef struct {
    int32_t tid;
    int32_t freed;   /* 1 + the index of the marked thread whose block it freed, or 0 */
    bool    intact;
} probe_report;

static int32_t prober(void *arg) {
    probe_report *r = arg;
    int32_t tid = lc_kernel_get_thread_id();
    r->tid = tid;
    if (tid <= 0 || tid >= PID_MAX_LIMIT || marked_by_tid[tid] == 0) return 0;
    marked_thread *m = &marked_threads[marked_by_tid[tid] - 1];
    void *block = atomic_exchange(&m->block, NULL);
    if (block == NULL) return 0;
    r->intact = marked(block, m->size, 0x77);
    lc_heap_free(block);   /* into the page the thread of this id abandoned */
    r->freed = marked_by_tid[tid];
    return 0;
}

/* Allocates, writes and frees a block of each marked size class, as a page
 * that a wrong free returned to its segment while it was still on its
 * abandoned list would be adopted again here, and break. */
static int32_t class_user(void *arg) {
    int32_t *failures = arg;
    for (int i = 0; i < MARKED; i++) {
        lc_result_ptr r = lc_heap_allocate(marked_sizes[i]);
        if (lc_ptr_is_err(r)) {
            (*failures)++;
            continue;
        }
        mark(r.value, marked_sizes[i], 0x55);
        if (!marked(r.value, marked_sizes[i], 0x55)) (*failures)++;
        lc_heap_free(r.value);
    }
    return 0;
}

/* kernel.pid_max, or -1 when it cannot be read. */
static int64_t read_pid_max(void) {
    if (read_proc(PID_MAX_PATH) < 0) return -1;
    int64_t value = 0;
    const char *c = proc_text;
    for (; *c >= '0' && *c <= '9'; c++) value = value * 10 + (*c - '0');
    return c > proc_text ? value : -1;
}

static void test_reused_id_frees_remotely(void) {
    int64_t pid_max = read_pid_max();
    if (pid_max < 0) TEST_SKIP(PID_MAX_PATH " cannot be read: is /proc mounted?");
    if (pid_max > PID_MAX_LIMIT)
        TEST_SKIP("kernel.pid_max is above 32768, so thread ids take minutes to wrap;"
                  " sysctl -w kernel.pid_max=32768 lets this test run");

    for (int i = 0; i < MARKED; i++) {
        marked_threads[i].size = marked_sizes[i];
        lc_thread thread;
        TEST_REQUIRE_OK(lc_thread_create(&thread, marker, &marked_threads[i]));
        lc_thread_join(&thread);
        TEST_REQUIRE_NOT_NULL(atomic_load(&marked_threads[i].block));
        TEST_REQUIRE(marked_threads[i].tid > 0 && marked_threads[i].tid < PID_MAX_LIMIT);
        marked_by_tid[marked_threads[i].tid] = (int16_t)(i + 1);
    }

    lc_heap_stats before, after;
    lc_heap_get_stats(&before);
    int64_t limit = 2 * pid_max + MARKED;
    int64_t threads = 0, reused = 0, damaged = 0;
    int64_t start = lc_time_start_timer();
    while (reused < WANTED && threads < limit
           && lc_time_elapsed_milliseconds(start) < REUSE_LIMIT_MS) {
        probe_report r = { 0 };
        lc_thread thread;
        TEST_REQUIRE_OK(lc_thread_create(&thread, prober, &r));
        lc_thread_join(&thread);
        threads++;
        if (r.freed == 0) continue;
        reused++;
        if (!r.intact) damaged++;
    }

    /* The blocks whose ids did not come back, freed from here; each is its
     * page's last, so its free returns the page, or a thread's exit then
     * does. */
    for (int i = 0; i < MARKED; i++) {
        void *block = atomic_exchange(&marked_threads[i].block, NULL);
        if (block != NULL) lc_heap_free(block);
    }
    TEST_REQUIRE(run_collector());
    lc_heap_get_stats(&after);
    int32_t class_failures = 0;
    lc_thread user;
    TEST_REQUIRE_OK(lc_thread_create(&user, class_user, &class_failures));
    lc_thread_join(&user);
    TEST_ASSERT_EQ(class_failures, 0);

    _test_print_text("  ");
    lc_print_signed(TEST_FD, threads);
    _test_print_text(" probes in ");
    lc_print_signed(TEST_FD, lc_time_elapsed_milliseconds(start));
    _test_print_text(" ms; ");
    lc_print_signed(TEST_FD, reused);
    _test_print_text(" got the id of a marked thread and freed its block\n");
    if (reused == 0)
        _test_print_text("  no probe got a marked id, so the ids did not wrap round in time\n");
    TEST_ASSERT(reused > 0);
    TEST_ASSERT_EQ(damaged, 0);
#if LC_STATS
    /* Each marked thread's page, which the free of its one block returned. */
    TEST_ASSERT(after.heap_pages_reclaimed >= before.heap_pages_reclaimed + MARKED);
#else
    (void)before;
    (void)after;
#endif
}

int main(int argc, char **argv, char **envp) {
    test_init(argc, argv);
    test_chaos_init(envp);   /* LC_CHAOS, in the faultinject build */
    TEST_RUN(test_create_join_cycles);
    TEST_RUN(test_create_join_cycles_rss);
    TEST_RUN(test_producer_rounds);
    TEST_RUN(test_producer_rounds_rss);
    TEST_RUN(test_partial_frees_are_adopted);
    TEST_RUN(test_live_owner_exit_rss);
    TEST_RUN(test_reused_id_frees_remotely);
    return test_main();
}
