/*
 * IO-dirent-align — lc_directory_next must not read a getdents64 record
 * through a misaligned pointer.
 *
 * getdents64 starts each record at a multiple of 8 from the start of the
 * buffer it fills. lc_directory's buffer followed an int32_t, at offset 4,
 * so every record sat 4 bytes off the alignment of struct linux_dirent64,
 * which holds two 64-bit fields, and lc_directory_next read d_reclen and
 * d_type through a pointer to that struct: undefined behavior. x86_64 and
 * aarch64 load a misaligned 16-bit field without complaint, so only the
 * ubsan build showed the defect: its alignment check trapped at the first
 * record, and the program died with SIGILL. lc_directory_next now copies
 * the fields out at their offsets, and the buffer is also aligned to 8.
 * The other builds pass with or without the fix, and in each this checks
 * that lc_directory_next lists a directory right.
 *
 * The directory holds entries with names of different lengths, so its
 * records have different lengths too. It goes in build/regress, so run this
 * from the source root, as ninja regress does.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/string.h>
#include <lightc/syscall.h>

#define DIR_PATH "build/regress/IO-dirent-align.d"

typedef struct {
    const char *name;
    uint8_t     type;
    int32_t     seen;
} expected_entry;

static expected_entry expected[] = {
    { ".",                                  LC_DT_DIR, 0 },
    { "..",                                 LC_DT_DIR, 0 },
    { "a",                                  LC_DT_REG, 0 },
    { "a_name_longer_than_one_record_slot", LC_DT_REG, 0 },
    { "sub",                                LC_DT_DIR, 0 },
};
#define EXPECTED_COUNT (sizeof(expected) / sizeof(expected[0]))

/* Remove what an aborted run may have left. */
static void remove_directory(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, DIR_PATH "/a", 0);
    (void)lc_kernel_unlinkat(AT_FDCWD, DIR_PATH "/a_name_longer_than_one_record_slot", 0);
    (void)lc_kernel_unlinkat(AT_FDCWD, DIR_PATH "/sub", AT_REMOVEDIR);
    (void)lc_kernel_unlinkat(AT_FDCWD, DIR_PATH, AT_REMOVEDIR);
}

static void test_directory_next_lists_every_entry(void) {
    remove_directory();
    TEST_REQUIRE_EQ(lc_kernel_mkdirat(AT_FDCWD, DIR_PATH, 0755), 0);
    TEST_REQUIRE_OK(lc_file_write_all(DIR_PATH "/a", "x", 1));
    TEST_REQUIRE_OK(lc_file_write_all(DIR_PATH "/a_name_longer_than_one_record_slot", "x", 1));
    TEST_REQUIRE_EQ(lc_kernel_mkdirat(AT_FDCWD, DIR_PATH "/sub", 0755), 0);

    lc_directory dir;
    TEST_REQUIRE_OK(lc_directory_open(&dir, DIR_PATH));

    /* Bounded: a listing that never ends fails instead of hanging. */
    lc_directory_entry entry;
    size_t entries = 0;
    while (entries < 64 && lc_directory_next(&dir, &entry)) {
        entries++;
        size_t length = lc_string_length(entry.name);
        bool known = false;
        for (size_t i = 0; i < EXPECTED_COUNT; i++) {
            if (!lc_string_equal(entry.name, length, expected[i].name, lc_string_length(expected[i].name)))
                continue;
            known = true;
            expected[i].seen++;
            /* A filesystem may leave the type unknown. */
            if (entry.type != LC_DT_UNKNOWN)
                TEST_ASSERT_EQ(entry.type, expected[i].type);
        }
        if (!known) {
            _test_print_text("  unexpected entry: ");
            _test_print_text(entry.name);
            _test_print_text("\n");
        }
    }
    lc_directory_close(&dir);

    TEST_ASSERT_EQ(entries, EXPECTED_COUNT);
    for (size_t i = 0; i < EXPECTED_COUNT; i++)
        TEST_ASSERT_EQ(expected[i].seen, 1);

    remove_directory();
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_directory_next_lists_every_entry);
    return test_main();
}
