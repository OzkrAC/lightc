/*
 * test_library.c — tests for lightc dynamic library loading.
 *
 * Loads test_plugin.so from the suite's own directory, the one argv[0]
 * names. The build puts the plugin beside the suite, so build/test_library
 * opens build/test_plugin.so from any current directory, and building the
 * suite builds the plugin.
 * The plugin exports: int plugin_add(int a, int b)
 */

#include "test.h"
#include <lightc/library.h>
#include <lightc/syscall.h>
#include <lightc/heap.h>

/* Set by main: the plugin, and the scratch file the malformed copies of it
 * are written to, both in the suite's directory. */
static char plugin_path[4096];
static char malformed_path[4096];
#define PLUGIN_PATH plugin_path
#define TMP_PATH    malformed_path

/* Length of `path`'s directory, up to and including its last '/'; 0 if it
 * has none. */
static size_t directory_length(const char *path) {
    size_t length = 0;
    for (size_t i = 0; path[i] != '\0'; i++)
        if (path[i] == '/') length = i + 1;
    return length;
}

/* Set `out` to argv[0]'s directory, up to its last '/', followed by `name`.
 * An argv[0] with no '/', as when a shell found the suite through PATH, gives
 * the directory of /proc/self/exe instead. A path too long for `out`, or no
 * directory at all, gives `name` alone, in the current directory. */
static void path_beside_suite(char *out, size_t size, const char *argv0, const char *name) {
    static char exe[4096];
    const char *self = argv0;
    size_t dir_length = directory_length(argv0);
    if (dir_length == 0) {
        lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, "/proc/self/exe", exe, sizeof exe);
        if (n > 0 && (size_t)n < sizeof exe) {
            exe[n] = '\0';
            self = exe;
            dir_length = directory_length(exe);
        }
    }
    size_t name_length = lc_string_length(name);
    if (dir_length + name_length >= size) dir_length = 0;
    lc_bytes_copy(out, self, dir_length);
    lc_bytes_copy(out + dir_length, name, name_length + 1);
}

/* ===== test_library_open_close ===== */

static void test_library_open_close(void) {
    lc_result_ptr r = lc_library_open(PLUGIN_PATH);
    TEST_ASSERT_PTR_OK(r);
    TEST_ASSERT_NOT_NULL(r.value);

    lc_library *lib = (lc_library *)r.value;
    lc_library_close(lib);
}

/* ===== test_library_find_symbol ===== */

typedef int (*plugin_add_fn)(int, int);

static void test_library_find_symbol(void) {
    lc_result_ptr lib_r = lc_library_open(PLUGIN_PATH);
    TEST_REQUIRE_PTR_OK(lib_r);

    lc_library *lib = (lc_library *)lib_r.value;

    lc_result_ptr sym_r = lc_library_find_symbol(lib, "plugin_add");
    TEST_REQUIRE_PTR_OK(sym_r);
    TEST_REQUIRE_NOT_NULL(sym_r.value);

    plugin_add_fn add = (plugin_add_fn)sym_r.value;
    int result = add(2, 3);
    TEST_ASSERT_EQ(result, 5);

    result = add(-10, 10);
    TEST_ASSERT_EQ(result, 0);

    lc_library_close(lib);
}

/* ===== test_library_missing_symbol ===== */

static void test_library_missing_symbol(void) {
    lc_result_ptr lib_r = lc_library_open(PLUGIN_PATH);
    TEST_ASSERT_PTR_OK(lib_r);

    lc_library *lib = (lc_library *)lib_r.value;

    lc_result_ptr sym_r = lc_library_find_symbol(lib, "nonexistent_symbol_xyz");
    TEST_ASSERT_PTR_ERR(sym_r);

    lc_library_close(lib);
}

/* ===== test_library_open_nonexistent ===== */

static void test_library_open_nonexistent(void) {
    lc_result_ptr r = lc_library_open("/nonexistent.so");
    TEST_ASSERT_PTR_ERR(r);
}

/* ========================================================================
 * Malformed-ELF regression tests (C6)
 *
 * The loader treats a .so as fully untrusted. These tests take the real,
 * valid plugin, corrupt one structural field at a time, write the result to a
 * scratch file, and confirm lc_library_open rejects it with an error instead
 * of reading/writing out of bounds (which would crash the test process). A
 * round-trip positive control proves the read/mutate/write pipeline itself is
 * sound, so a rejection is attributable to the corruption, not the plumbing.
 * ======================================================================== */

/* Little-endian field access on the raw ELF byte buffer. */
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t rd_u64(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}
static void wr_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void wr_u64(uint8_t *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }

/* ELF64 header field offsets we touch. */
#define EH_PHOFF     32
#define EH_PHENTSIZE 54
#define EH_PHNUM     56
#define PHDR_SIZE    56
#define PH_TYPE      0
#define PH_VADDR     16
#define PH_FILESZ    32
#define PH_MEMSZ     40
#define PT_LOAD_     1
#define PT_DYNAMIC_  2

/* Read the entire valid plugin into a heap buffer. Returns size (0 on failure);
 * *out receives the buffer (caller frees). */
static size_t load_plugin_bytes(uint8_t **out) {
    int32_t fd = (int32_t)lc_kernel_open_file(PLUGIN_PATH, O_RDONLY, 0);
    if (fd < 0) return 0;
    int64_t sz = lc_kernel_seek_position(fd, 0, SEEK_END);
    if (sz <= 0) { lc_kernel_close_file(fd); return 0; }
    lc_kernel_seek_position(fd, 0, SEEK_SET);

    lc_result_ptr buf_r = lc_heap_allocate((size_t)sz);
    if (lc_ptr_is_err(buf_r)) { lc_kernel_close_file(fd); return 0; }
    uint8_t *buf = (uint8_t *)buf_r.value;

    size_t off = 0;
    while (off < (size_t)sz) {
        lc_sysret n = lc_kernel_read_bytes(fd, buf + off, (size_t)sz - off);
        if (n <= 0) { lc_heap_free(buf); lc_kernel_close_file(fd); return 0; }
        off += (size_t)n;
    }
    lc_kernel_close_file(fd);
    *out = buf;
    return (size_t)sz;
}

/* Write `len` bytes to the scratch path. */
static bool write_tmp(const uint8_t *buf, size_t len) {
    int32_t fd = (int32_t)lc_kernel_open_file(TMP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;
    size_t off = 0;
    while (off < len) {
        lc_sysret n = lc_kernel_write_bytes(fd, buf + off, len - off);
        if (n <= 0) { lc_kernel_close_file(fd); return false; }
        off += (size_t)n;
    }
    lc_kernel_close_file(fd);
    return true;
}

static void remove_tmp(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, TMP_PATH, 0);
}

/* Offset of the first program header of type `pt`, or 0 if none. */
static uint64_t find_phdr(const uint8_t *buf, size_t len, uint32_t pt) {
    uint64_t phoff = rd_u64(buf + EH_PHOFF);
    uint16_t phnum = rd_u16(buf + EH_PHNUM);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t off = phoff + (uint64_t)i * PHDR_SIZE;
        if (off + PHDR_SIZE > len) break;
        if (rd_u32(buf + off + PH_TYPE) == pt) return off;
    }
    return 0;
}

/* Positive control: writing the pristine bytes out and loading them must work,
 * proving the read/mutate/write harness is sound. */
static void test_library_roundtrip_control(void) {
    uint8_t *buf = NULL;
    size_t len = load_plugin_bytes(&buf);
    TEST_REQUIRE(len > 0);
    TEST_ASSERT(write_tmp(buf, len));

    lc_result_ptr r = lc_library_open(TMP_PATH);
    TEST_ASSERT_PTR_OK(r);
    lc_library_close((lc_library *)r.value);

    lc_heap_free(buf);
    remove_tmp();
}

/* e_phentsize disagreeing with sizeof(Elf64_Phdr) must be rejected — otherwise
 * our fixed-stride indexing desyncs from the allocation. */
static void test_library_bad_phentsize(void) {
    uint8_t *buf = NULL;
    size_t len = load_plugin_bytes(&buf);
    TEST_REQUIRE(len > 0);

    wr_u16(buf + EH_PHENTSIZE, 55);  /* not 56 */
    TEST_ASSERT(write_tmp(buf, len));

    lc_result_ptr r = lc_library_open(TMP_PATH);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);

    lc_heap_free(buf);
    remove_tmp();
}

/* p_filesz > p_memsz on a LOAD segment is an out-of-bounds copy — reject it. */
static void test_library_oversized_filesz(void) {
    uint8_t *buf = NULL;
    size_t len = load_plugin_bytes(&buf);
    TEST_REQUIRE(len > 0);

    uint64_t load = find_phdr(buf, len, PT_LOAD_);
    TEST_ASSERT(load != 0);
    /* filesz far larger than memsz (and larger than the whole mapping). */
    wr_u64(buf + load + PH_FILESZ, 0xFFFFFFFFull);
    TEST_ASSERT(write_tmp(buf, len));

    lc_result_ptr r = lc_library_open(TMP_PATH);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);

    lc_heap_free(buf);
    remove_tmp();
}

/* A PT_DYNAMIC segment whose vaddr points outside the mapping must be caught by
 * the dynamic-section bounds check, not walked. */
static void test_library_dynamic_out_of_range(void) {
    uint8_t *buf = NULL;
    size_t len = load_plugin_bytes(&buf);
    TEST_REQUIRE(len > 0);

    uint64_t dyn = find_phdr(buf, len, PT_DYNAMIC_);
    TEST_ASSERT(dyn != 0);
    /* Push the dynamic array's vaddr way past any mapped page. */
    wr_u64(buf + dyn + PH_VADDR, 0x7fffffffull);
    TEST_ASSERT(write_tmp(buf, len));

    lc_result_ptr r = lc_library_open(TMP_PATH);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);

    lc_heap_free(buf);
    remove_tmp();
}

/* Truncating the file after the ELF header leaves the program-header table
 * unreadable: the loader must fail cleanly (short read), not crash. */
static void test_library_truncated_phdrs(void) {
    uint8_t *buf = NULL;
    size_t len = load_plugin_bytes(&buf);
    TEST_REQUIRE(len > 0);
    TEST_ASSERT(len > 64);

    TEST_ASSERT(write_tmp(buf, 64));  /* only the 64-byte ELF header */

    lc_result_ptr r = lc_library_open(TMP_PATH);
    TEST_ASSERT_PTR_ERR(r);

    lc_heap_free(buf);
    remove_tmp();
}

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    const char *argv0 = argc > 0 ? argv[0] : "";
    path_beside_suite(plugin_path, sizeof plugin_path, argv0, "test_plugin.so");
    path_beside_suite(malformed_path, sizeof malformed_path, argv0, "malformed_plugin.so");

    TEST_RUN(test_library_open_close);
    TEST_RUN(test_library_find_symbol);
    TEST_RUN(test_library_missing_symbol);
    TEST_RUN(test_library_open_nonexistent);

    /* Malformed-ELF hardening (C6) */
    TEST_RUN(test_library_roundtrip_control);
    TEST_RUN(test_library_bad_phentsize);
    TEST_RUN(test_library_oversized_filesz);
    TEST_RUN(test_library_dynamic_out_of_range);
    TEST_RUN(test_library_truncated_phdrs);

    return test_main();
}
