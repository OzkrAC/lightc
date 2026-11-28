/*
 * LIO-coro-stack-align — a lightio handler must run on a stack aligned as
 * the ABI has it, with a guard page below it.
 *
 * lightio set up its coroutines' stacks itself, apart from coroutine.c's,
 * and got none of coroutine.c's fixes: on x86_64 a handler started with
 * rsp misaligned by 8, so the first aligned SSE spill (movaps) of its call
 * tree faulted, and its stack had no guard page, so an overflow ran on into
 * the mapping below. The first fix aligned it; now both use one helper,
 * which maps a guard page below each stack and enters the coroutine as
 * after a call.
 *
 * A handler reads its stack pointer as a call leaves it, as test_coroutine
 * does: on x86_64 rsp & 15 must be 8, on aarch64 sp & 15 must be 0. It then
 * finds the mapping of its stack in /proc/self/maps, and the mapping right
 * below that must be PROT_NONE. The program needs io_uring, which
 * qemu-user lacks, and skips there.
 */

#include "test.h"
#include <lightio/lightio.h>
#include <lightc/async.h>
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/socket.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define PORT 19941

/* The stack pointer at the entry of a function, as the call left it. */
#if defined(__x86_64__)
__asm__(".text\n"
        ".globl lio_test_entry_sp\n"
        ".type lio_test_entry_sp, @function\n"
        "lio_test_entry_sp:\n"
        "    mov %rsp, %rax\n"
        "    ret\n"
        ".size lio_test_entry_sp, . - lio_test_entry_sp\n");
#define ENTRY_SP_MOD 8u
#elif defined(__aarch64__)
__asm__(".text\n"
        ".globl lio_test_entry_sp\n"
        ".type lio_test_entry_sp, %function\n"
        "lio_test_entry_sp:\n"
        "    mov x0, sp\n"
        "    ret\n"
        ".size lio_test_entry_sp, . - lio_test_entry_sp\n");
#define ENTRY_SP_MOD 0u
#endif
extern uintptr_t lio_test_entry_sp(void);

/* The hex number at *p, up to `end`; moves *p past it. */
static uintptr_t parse_hex(const char **p, const char *end) {
    uintptr_t value = 0;
    for (; *p < end; (*p)++) {
        char c = **p;
        if (c >= '0' && c <= '9')      value = value * 16 + (uintptr_t)(c - '0');
        else if (c >= 'a' && c <= 'f') value = value * 16 + (uintptr_t)(c - 'a' + 10);
        else break;
    }
    return value;
}

/* Whether, in /proc/self/maps, the mapping right below the one that holds
 * `address` is PROT_NONE ("---p"). */
static bool guard_below(uintptr_t address) {
    uint8_t *data;
    size_t size;
    if (lc_file_read_all("/proc/self/maps", &data, &size).error) return false;
    const char *text = (const char *)data, *end = text + size;
    uintptr_t stack_start = 0;
    bool found = false, guard = false;
    for (int pass = 0; pass < 2; pass++) {
        for (const char *p = text; p < end; ) {
            const char *line = p;
            uintptr_t start = parse_hex(&p, end);
            if (p < end && *p == '-') p++;
            uintptr_t stop = parse_hex(&p, end);
            if (p < end && *p == ' ') p++;
            if (pass == 0 && start <= address && address < stop) {
                stack_start = start;
                found = true;
            }
            if (pass == 1 && found && stop == stack_start && end - p >= 4 &&
                p[0] == '-' && p[1] == '-' && p[2] == '-')
                guard = true;
            for (p = line; p < end && *p != '\n'; p++) {}
            if (p < end) p++;
        }
    }
    lc_heap_free(data);
    return guard;
}

static _Atomic(int32_t) handled;
static uintptr_t entry_sp;
static bool guarded;

static void probe_handler(lio_stream *stream) {
    (void)stream;
    entry_sp = lio_test_entry_sp();
    volatile char here = 0;
    guarded = guard_below((uintptr_t)&here);
    atomic_store(&handled, 1);
}

static int32_t serve(void *arg) {
    lio_loop_run(arg);
    return 0;
}

static void test_handler_stack_is_aligned_and_guarded(void) {
    lc_result_ptr probe = lc_async_ring_create(4);
    if (lc_ptr_is_err(probe)) TEST_SKIP("io_uring not available");
    lc_async_ring_destroy(probe.value);

    lio_loop *loop = lio_loop_create();
    TEST_REQUIRE_NOT_NULL(loop);
    TEST_REQUIRE(lc_is_ok(lio_tcp_serve(loop, PORT, probe_handler)));
    lc_thread server;
    TEST_REQUIRE(lc_is_ok(lc_thread_create(&server, serve, loop)));
    lc_time_sleep_milliseconds(50);

    lc_result client = lc_socket_connect_to(127, 0, 0, 1, PORT);
    for (int i = 0; i < 500 && !atomic_load(&handled); i++) lc_time_sleep_milliseconds(10);
    if (lc_is_ok(client)) lc_socket_close((int32_t)client.value);

    /* The loop may wait in io_uring_enter: a connection wakes it to stop. */
    lio_loop_stop(loop);
    lc_result wake = lc_socket_connect_to(127, 0, 0, 1, PORT);
    if (lc_is_ok(wake)) lc_socket_close((int32_t)wake.value);
    lc_thread_join(&server);
    lio_loop_destroy(loop);

    TEST_REQUIRE(lc_is_ok(client));
    TEST_ASSERT_EQ(atomic_load(&handled), 1);
    TEST_ASSERT_EQ((unsigned)(entry_sp & 15), ENTRY_SP_MOD);
    TEST_ASSERT(guarded);   /* the defect: no guard page */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_handler_stack_is_aligned_and_guarded);
    return test_main();
}
