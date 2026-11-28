/*
 * ARENA-abi-lcstats — a program built with another NDEBUG than liblightc.a
 * must agree with it on the layout of lc_arena.
 *
 * lc_arena had three counters only when LC_STATS was set, and LC_STATS
 * follows NDEBUG, so the struct was 24 bytes in a release build and 48 in a
 * debug one. A program compiled with -DNDEBUG against a debug library had
 * the library write the counters past the end of its 24-byte arena, and a
 * debug program against a release library got counters the library never
 * set. The counters are now there in every build, and LC_STATS only decides
 * whether they are kept.
 *
 * This program flips NDEBUG against the mode ninja builds it in, before any
 * lightc header, so its LC_STATS is always the other one from the
 * library's. Against the old library each test below fails in one of the
 * modes: the guard after the arena in a debug build, the counters after
 * create in a release one.
 */

#ifdef NDEBUG
# undef NDEBUG
#else
# define NDEBUG 1
#endif

#include "test.h"
#include <lightc/memory.h>
#include <lightc/string.h>

#define GUARD 0xa5a5a5a5a5a5a5a5u

typedef struct {
    uint64_t before[4];
    lc_arena arena;
    uint64_t after[8];
} guarded_arena;

/* The library's functions write no byte outside the arena as this program
 * lays it out. */
static void test_library_stays_inside_the_arena(void) {
    guarded_arena g;
    lc_bytes_fill(&g, 0xa5, sizeof g);
    g.arena = lc_arena_create(65536);
    TEST_REQUIRE_NOT_NULL(g.arena.base);

    for (int i = 0; i < 100; i++)
        TEST_ASSERT_NOT_NULL(lc_arena_allocate_aligned(&g.arena, 24, 64));
    TEST_ASSERT_NOT_NULL(lc_arena_allocate(&g.arena, 16));

    for (size_t i = 0; i < sizeof g.before / sizeof g.before[0]; i++)
        TEST_ASSERT_EQ(g.before[i], (uint64_t)GUARD);
    for (size_t i = 0; i < sizeof g.after / sizeof g.after[0]; i++)
        TEST_ASSERT_EQ(g.after[i], (uint64_t)GUARD);
    lc_arena_destroy(&g.arena);
}

/* Fills the stack below the caller with 0xa5, where the next call's frame
 * will be. */
[[gnu::noinline]] static void dirty_the_stack(void) {
    volatile uint8_t junk[2048];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = 0xa5;
}

/* The counters of a new arena, as this program reads them. The arena is
 * initialized from the call, so the compiler returns the struct straight
 * into it, on the stack dirty_the_stack has just filled: a field the
 * library does not write keeps 0xa5. */
[[gnu::noinline]] static lc_arena_stats stats_of_a_new_arena(void) {
    lc_arena a = lc_arena_create(4096);
    lc_arena_stats stats;
    lc_arena_get_stats(&a, &stats);
    lc_arena_destroy(&a);
    return stats;
}

/* lc_arena_create sets every field this program reads. The counters are
 * read through lc_arena_get_stats, which the old headers have too, so this
 * builds against the library from before the fix. */
static void test_create_sets_every_counter(void) {
    dirty_the_stack();
    lc_arena_stats stats = stats_of_a_new_arena();
    TEST_ASSERT_NEQ(stats.capacity, (size_t)0);
    TEST_ASSERT_EQ(stats.used, (size_t)0);
    TEST_ASSERT_EQ(stats.peak_used, (size_t)0);
    TEST_ASSERT_EQ(stats.alloc_count, (size_t)0);
    TEST_ASSERT_EQ(stats.reset_count, (size_t)0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_library_stays_inside_the_arena);
    TEST_RUN(test_create_sets_every_counter);
    return test_main();
}
