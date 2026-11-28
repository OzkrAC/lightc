/*
 * test_index — a function does what api/index.md and api/syscalls.tsv say.
 *
 * tools/api-snapshot.sh derives, from the library's sources, the system
 * calls each public function can make and whether the child of a raw clone
 * can call it. This suite checks both claims by running the functions: each
 * function of the headers in `covered` below, the ones whose calls a
 * seccomp filter has to list.
 *
 * Each function has a case: a few lines that set up what the call needs,
 * then enter(), then the call. Each case runs in a forked child, and enter()
 * puts that child under a seccomp filter that allows only exit_group and the
 * system calls api/syscalls.tsv lists for the function on this arch. Any
 * other call traps, and the SIGSYS handler reports it to the parent through
 * a shared page. For a function that api/index.md calls clone-safe, enter()
 * first also makes the program's writable segment read-only and points the
 * thread pointer at a page that cannot be read: the child of a raw clone has
 * no lightc thread, so a write to a variable of lightc, or a use of TLS or
 * of the heap, then kills the child with SIGSEGV. A case still running after
 * 10 s dies of SIGALRM.
 *
 * A call that could block or change the machine gets arguments that make
 * the kernel refuse it, such as fd -1. The filter sees a system call when it
 * is made, whatever its arguments, so the call is still checked.
 *
 * test_every_function_has_a_case fails when a function of those headers has
 * no case, or a case names no function of the index. The suite reads api/
 * and syscalls.def from the source root, the directory above its own,
 * build/. qemu-user does not run a guest's seccomp filter, so there every
 * test but that one skips.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/log.h>
#include <lightc/memory.h>
#include <lightc/print.h>
#include <lightc/probe.h>
#include <lightc/signal.h>
#include <lightc/socket.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/time.h>
#include "../src/kernel.h"   /* SA_RESTORER, ARCH_SET_FS, Elf64_Ehdr, Elf64_Phdr */

/* --- seccomp and BPF, as <linux/seccomp.h> and <linux/filter.h> give them --- */

#define PR_SET_NO_NEW_PRIVS      38
#define PR_GET_DUMPABLE          3
#define SECCOMP_SET_MODE_FILTER  1
#define SECCOMP_RET_KILL_PROCESS 0x80000000u
#define SECCOMP_RET_TRAP         0x00030000u
#define SECCOMP_RET_ALLOW        0x7fff0000u
#define BPF_LD_W_ABS             0x20    /* BPF_LD | BPF_W | BPF_ABS */
#define BPF_JEQ_K                0x15    /* BPF_JMP | BPF_JEQ | BPF_K */
#define BPF_RET_K                0x06    /* BPF_RET | BPF_K */
#define SA_SIGINFO               4
#define SIGSYS_NUMBER            31
#define ITIMER_REAL              0
#define PF_W                     2

#if defined(__x86_64__)
#define AUDIT_ARCH_THIS 0xc000003eu
#define THIS_ARCH "x86_64"
#elif defined(__aarch64__)
#define AUDIT_ARCH_THIS 0xc00000b7u
#define THIS_ARCH "aarch64"
#endif

typedef struct {
    uint16_t code;
    uint8_t  jt;
    uint8_t  jf;
    uint32_t k;
} bpf_insn;

typedef struct {
    uint16_t  length;
    bpf_insn *filter;
} bpf_program;

/* --- The files the suite reads --- */

typedef struct {
    uint8_t *data;
    size_t   size;
} text;

static text tsv, index_md, defs;
static bool loaded = false;

static char root[4096];

/* Sets root to the directory above the suite's, with a trailing '/'. */
static void find_root(const char *argv0) {
    static char exe[4096];
    const char *self = argv0;
    size_t dir = 0;
    for (size_t i = 0; self[i] != '\0'; i++)
        if (self[i] == '/') dir = i + 1;
    if (dir == 0) {
        lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, "/proc/self/exe", exe, sizeof exe - 1);
        if (n > 0) {
            exe[n] = '\0';
            self = exe;
            for (size_t i = 0; self[i] != '\0'; i++)
                if (self[i] == '/') dir = i + 1;
        }
    }
    if (dir + 4 >= sizeof root) dir = 0;
    lc_bytes_copy(root, self, dir);
    lc_bytes_copy(root + dir, "../", 4);
}

static bool load(text *t, const char *name) {
    static char path[4096 + 64];
    size_t r = lc_string_length(root), n = lc_string_length(name);
    lc_bytes_copy(path, root, r);
    lc_bytes_copy(path + r, name, n + 1);
    if (lc_file_read_all(path, &t->data, &t->size).error) {
        _test_print_text("  cannot read ");
        _test_print_text(path);
        _test_print_text("\n");
        return false;
    }
    return true;
}

static bool load_all(void) {
    if (!loaded)
        loaded = load(&tsv, "api/syscalls.tsv") && load(&index_md, "api/index.md") &&
                 load(&defs, "syscalls.def");
    return loaded;
}

/* A line of t from *at, without its '\n'; false at the end. */
static bool next_line(const text *t, size_t *at, const char **line, size_t *length) {
    if (*at >= t->size) return false;
    const char *s = (const char *)t->data + *at;
    size_t n = 0;
    while (*at + n < t->size && s[n] != '\n') n++;
    *line = s;
    *length = n;
    *at += n + 1;
    return true;
}

/* Field `which`, from 0, of a line split at `sep`, or false. With sep ' ', a
 * run of spaces is one separator, and spaces before the first field are
 * skipped. */
static bool field(const char *line, size_t length, char sep, size_t which,
                  const char **out, size_t *out_length) {
    size_t i = 0;
    if (sep == ' ')
        while (i < length && line[i] == ' ') i++;
    for (;;) {
        size_t start = i;
        while (i < length && line[i] != sep) i++;
        if (which == 0) {
            *out = line + start;
            *out_length = i - start;
            return sep != ' ' || i > start;
        }
        if (i >= length) return false;
        which--;
        i++;
        if (sep == ' ')
            while (i < length && line[i] == ' ') i++;
    }
}

static bool same(const char *a, size_t a_length, const char *b) {
    size_t n = lc_string_length(b);
    return a_length == n && lc_bytes_compare(a, b, n) == 0;
}

/* Has the space-separated list s the word w? */
static bool has_word(const char *s, size_t length, const char *w) {
    const char *f;
    size_t n;
    for (size_t i = 0; field(s, length, ' ', i, &f, &n); i++)
        if (same(f, n, w)) return true;
    return false;
}

/* The number of the system call `name` on this arch, from syscalls.def, or -1. */
static int32_t syscall_number(const char *name, size_t length) {
    size_t at = 0;
    const char *line, *f;
    size_t n, fn;
    while (next_line(&defs, &at, &line, &n)) {
        if (!field(line, n, ' ', 0, &f, &fn) || !same(f, fn, "syscall")) continue;
        if (!field(line, n, ' ', 1, &f, &fn) || fn != length || lc_bytes_compare(f, name, length) != 0) continue;
#if defined(__x86_64__)
        if (!field(line, n, ' ', 2, &f, &fn)) return -1;
#else
        if (!field(line, n, ' ', 3, &f, &fn)) return -1;
#endif
        int32_t nr = 0;
        for (size_t i = 0; i < fn; i++) {
            if (f[i] < '0' || f[i] > '9') return -1;
            nr = nr * 10 + (f[i] - '0');
        }
        return nr;
    }
    return -1;
}

/* Prints the name of system call nr, or the number when syscalls.def lacks it. */
static void print_syscall(int32_t nr) {
    size_t at = 0;
    const char *line, *name;
    size_t n, nn;
    while (next_line(&defs, &at, &line, &n)) {
        const char *f;
        size_t fn;
        if (!field(line, n, ' ', 0, &f, &fn) || !same(f, fn, "syscall")) continue;
        if (!field(line, n, ' ', 1, &name, &nn)) continue;
        if (syscall_number(name, nn) == nr) {
            lc_print_string(TEST_FD, name, nn);
            _test_print_text(" (");
            lc_print_signed(TEST_FD, nr);
            _test_print_text(")");
            return;
        }
    }
    _test_print_text("system call ");
    lc_print_signed(TEST_FD, nr);
}

/* The row of api/index.md for `name`, or false. */
static bool index_row(const char *name, const char **row, size_t *length) {
    size_t at = 0, n = lc_string_length(name);
    const char *line;
    size_t ln;
    while (next_line(&index_md, &at, &line, &ln)) {
        if (ln > n + 4 && lc_bytes_compare(line, "| `", 3) == 0 &&
                lc_bytes_compare(line + 3, name, n) == 0 && line[3 + n] == '`') {
            *row = line;
            *length = ln;
            return true;
        }
    }
    return false;
}

/* The header cell of a row, without its backticks. */
static bool row_header(const char *row, size_t length, const char **h, size_t *n) {
    size_t i = 3;
    while (i < length && row[i] != '`') i++;
    i += 5;   /* "` | `" */
    if (i >= length) return false;
    size_t start = i;
    while (i < length && row[i] != '`') i++;
    *h = row + start;
    *n = i - start;
    return true;
}

/* Does the Clone-safe cell, the last of the row, start with "yes"? A | in a
 * cell is written \|, so a " | " separates cells. */
static bool row_clone_safe(const char *row, size_t length) {
    if (length < 6) return false;
    size_t end = length - 2;   /* the space before the closing " |" */
    for (size_t j = end - 1; j >= 1; j--) {
        if (row[j] == '|' && row[j - 1] == ' ' && row[j + 1] == ' ')
            return end >= j + 2 + 3 && lc_bytes_compare(row + j + 2, "yes", 3) == 0;
    }
    return false;
}

/* --- The child's restrictions --- */

#define MAX_ALLOWED 64

typedef struct {
    volatile int32_t entered;
    volatile int32_t syscall;
} report_page;

static report_page *report;
static bpf_insn     filter[4 + 2 * MAX_ALLOWED + 2];
static bpf_program  program;
static bool         clone_safe;
static void        *tls_guard;
static uintptr_t    data_start, data_end;

/* The writable PT_LOAD of the program, page-aligned, from the program
 * headers of its file. AT_PHDR cannot give them: the kernel sets it only when
 * a PT_LOAD maps the headers, and lightc's linker scripts start the first
 * one after them. The program is static and not PIE, so its addresses are
 * those of the file. */
static bool find_data_segment(void) {
    uint8_t *exe = NULL;
    size_t size = 0;
    if (lc_file_read_all("/proc/self/exe", &exe, &size).error) return false;
    bool found = false;
    Elf64_Ehdr eh;
    if (size >= sizeof eh) {
        lc_bytes_copy(&eh, exe, sizeof eh);
        uint64_t page = lc_runtime_page_size();
        for (uint64_t i = 0; i < eh.e_phnum && !found; i++) {
            Elf64_Phdr ph;
            uint64_t at = eh.e_phoff + i * sizeof ph;
            if (at > size || size - at < sizeof ph) break;
            lc_bytes_copy(&ph, exe + at, sizeof ph);
            if (ph.p_type != PT_LOAD || !(ph.p_flags & PF_W)) continue;
            data_start = (uintptr_t)(ph.p_vaddr & ~(page - 1));
            data_end = (uintptr_t)((ph.p_vaddr + ph.p_memsz + page - 1) & ~(page - 1));
            found = true;
        }
    }
    lc_heap_free(exe);
    return found;
}

static void on_sigsys(int signo, void *info, void *context) {
    (void)signo;
    (void)context;
    /* si_syscall: after si_signo, si_errno, si_code, a pad and si_call_addr */
    int32_t nr;
    lc_bytes_copy(&nr, (const uint8_t *)info + 24, sizeof nr);
    report->syscall = nr;
    lc_kernel_exit(99);
}

/* Puts the child under what the index says of the function its case calls
 * next. It writes nothing of the program's own data. */
static void enter(void) {
    report->entered = 1;
    if (clone_safe) {
        (void)lc_kernel_protect_memory((void *)data_start, data_end - data_start, PROT_READ);
#if defined(__x86_64__)
        (void)lc_syscall2(SYS_arch_prctl, ARCH_SET_FS, (int64_t)(uintptr_t)tls_guard);
#else
        __asm__ volatile("msr tpidr_el0, %0" :: "r"(tls_guard));
#endif
    }
    lc_kernel_sigaction sa = {0};
    sa.handler = (void *)(uintptr_t)on_sigsys;
    sa.flags = SA_SIGINFO;
#if defined(__x86_64__)
    /* The handler exits, so the restorer is never called; x86_64 wants one. */
    sa.flags |= SA_RESTORER;
    sa.restorer = (void *)(uintptr_t)on_sigsys;
#endif
    (void)lc_syscall4(SYS_rt_sigaction, SIGSYS_NUMBER, (int64_t)&sa, 0, 8);
    (void)lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    if (lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &program) != 0) lc_kernel_exit(98);
}

static void build_filter(const int32_t *allowed, size_t count) {
    size_t k = 0;
    filter[k++] = (bpf_insn){ BPF_LD_W_ABS, 0, 0, 4 };                    /* arch */
    filter[k++] = (bpf_insn){ BPF_JEQ_K, 1, 0, AUDIT_ARCH_THIS };
    filter[k++] = (bpf_insn){ BPF_RET_K, 0, 0, SECCOMP_RET_KILL_PROCESS };
    filter[k++] = (bpf_insn){ BPF_LD_W_ABS, 0, 0, 0 };                    /* nr */
    for (size_t i = 0; i < count; i++) {
        filter[k++] = (bpf_insn){ BPF_JEQ_K, 0, 1, (uint32_t)allowed[i] };
        filter[k++] = (bpf_insn){ BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW };
    }
    filter[k++] = (bpf_insn){ BPF_RET_K, 0, 0, SECCOMP_RET_TRAP };
    program.length = (uint16_t)k;
    program.filter = filter;
}

/* --- Cases --- */

static int32_t devnull(void) {
    return (int32_t)lc_kernel_open_file("/dev/null", O_RDWR | O_CLOEXEC, 0);
}

static int clone_child(void *arg) {
    (void)arg;
    return 0;
}

static void any_handler(int signo) {
    (void)signo;
}

/* lightc/syscall.h */

static void case_lc_syscall0(void) { enter(); (void)lc_syscall0(SYS_getpid); }
static void case_lc_syscall1(void) { enter(); (void)lc_syscall1(SYS_close, -1); }
static void case_lc_syscall2(void) { enter(); (void)lc_syscall2(SYS_munmap, 0, 0); }
static void case_lc_syscall3(void) { enter(); (void)lc_syscall3(SYS_write, -1, 0, 0); }
static void case_lc_syscall4(void) { enter(); (void)lc_syscall4(SYS_rt_sigprocmask, LC_SIG_BLOCK, 0, 0, 8); }
static void case_lc_syscall5(void) { enter(); (void)lc_syscall5(SYS_prctl, PR_GET_DUMPABLE, 0, 0, 0, 0); }
static void case_lc_syscall6(void) { enter(); (void)lc_syscall6(SYS_futex, 0, 0, 0, 0, 0, 0); }
static void case_lc_kernel_exit(void) { enter(); lc_kernel_exit(0); }
static void case_lc_kernel_get_process_id(void) { enter(); (void)lc_kernel_get_process_id(); }
static void case_lc_kernel_fork(void) {
    enter();
    if (lc_kernel_fork() == 0) lc_kernel_exit(0);
}
static void case_lc_kernel_clone(void) {
    void *stack = lc_kernel_map_memory(NULL, 65536, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    enter();
    (void)lc_kernel_clone(SIGCHLD, (uint8_t *)stack + 65536, clone_child, NULL);
}
static void case_lc_kernel_execute(void) {
    char *none[] = { NULL };
    enter();
    (void)lc_kernel_execute("/nonexistent-lightc", none, none);
}
static void case_lc_kernel_wait_for_child(void) {
    int32_t status;
    enter();
    (void)lc_kernel_wait_for_child(-1, &status, 1 /* WNOHANG */);
}
static void case_lc_kernel_create_pipe(void) { int32_t fds[2]; enter(); (void)lc_kernel_create_pipe(fds, O_CLOEXEC); }
static void case_lc_kernel_duplicate_fd(void) { enter(); (void)lc_kernel_duplicate_fd(-1, 100, 0); }
static void case_lc_kernel_create_session(void) { enter(); (void)lc_kernel_create_session(); }
static void case_lc_kernel_open_file(void) { enter(); (void)lc_kernel_open_file("/dev/null", O_RDONLY, 0); }
static void case_lc_kernel_open_file_at(void) { enter(); (void)lc_kernel_open_file_at(AT_FDCWD, "/dev/null", O_RDONLY, 0); }
static void case_lc_kernel_close_file(void) { enter(); (void)lc_kernel_close_file(-1); }
static void case_lc_kernel_read_bytes(void) { int32_t fd = devnull(); char b[8]; enter(); (void)lc_kernel_read_bytes(fd, b, sizeof b); }
static void case_lc_kernel_write_bytes(void) { int32_t fd = devnull(); enter(); (void)lc_kernel_write_bytes(fd, "x", 1); }
static void case_lc_kernel_write_all(void) { int32_t fd = devnull(); enter(); (void)lc_kernel_write_all(fd, "xy", 2); }
static void case_lc_kernel_read_retry(void) { int32_t fd = devnull(); char b[8]; enter(); (void)lc_kernel_read_retry(fd, b, sizeof b); }
static void case_lc_kernel_read_all(void) { int32_t fd = devnull(); char b[8]; enter(); (void)lc_kernel_read_all(fd, b, sizeof b); }
static void case_lc_kernel_seek_position(void) { int32_t fd = devnull(); enter(); (void)lc_kernel_seek_position(fd, 0, 0); }
static void case_lc_kernel_device_control(void) {
    int32_t fd = devnull();
    uint8_t buf[64];
    enter();
    (void)lc_kernel_device_control(fd, 0x5401 /* TCGETS */, buf);
}
static void case_lc_kernel_read_directory(void) {
    int32_t fd = (int32_t)lc_kernel_open_file("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    uint8_t buf[1024];
    enter();
    (void)lc_kernel_read_directory(fd, buf, sizeof buf);
}
static void case_lc_kernel_getsockname(void) {
    int32_t fd = (int32_t)lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0).value;
    uint8_t addr[64];
    uint32_t length = sizeof addr;
    enter();
    (void)lc_kernel_getsockname(fd, addr, &length);
}
static void case_lc_kernel_map_memory(void) {
    enter();
    (void)lc_kernel_map_memory(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
static void *page(void) {
    return lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
static void case_lc_kernel_unmap_memory(void) { void *p = page(); enter(); (void)lc_kernel_unmap_memory(p, 4096); }
static void case_lc_kernel_protect_memory(void) { void *p = page(); enter(); (void)lc_kernel_protect_memory(p, 4096, PROT_READ); }
static void case_lc_kernel_set_heap_end(void) { enter(); (void)lc_kernel_set_heap_end(NULL); }
static void case_lc_kernel_advise_memory(void) { void *p = page(); enter(); (void)lc_kernel_advise_memory(p, 4096, MADV_DONTNEED); }
static void case_lc_kernel_get_thread_id(void) { enter(); (void)lc_kernel_get_thread_id(); }
static void case_lc_kernel_futex_wait(void) { int32_t w = 0; enter(); (void)lc_kernel_futex_wait(&w, 1); }
static void case_lc_kernel_futex_wake(void) { int32_t w = 0; enter(); (void)lc_kernel_futex_wake(&w, 1); }
static void case_lc_kernel_get_clock(void) { lc_timespec ts; enter(); (void)lc_kernel_get_clock(LC_CLOCK_MONOTONIC, &ts); }
static void case_lc_kernel_sleep(void) { lc_timespec ts = { 0, 0 }; enter(); (void)lc_kernel_sleep(&ts); }
static void case_lc_kernel_clock_sleep(void) { lc_timespec ts = { 0, 0 }; enter(); (void)lc_kernel_clock_sleep(LC_CLOCK_MONOTONIC, LC_TIMER_ABSTIME, &ts, NULL); }
static void case_lc_kernel_reset_signal(void) { enter(); (void)lc_kernel_reset_signal(LC_SIGQUIT); }
static void case_lc_kernel_set_signal_mask(void) { uint64_t none = 0; enter(); (void)lc_kernel_set_signal_mask(LC_SIG_BLOCK, &none, NULL); }
static void case_lc_kernel_create_signal_fd(void) { uint64_t none = 0; enter(); (void)lc_kernel_create_signal_fd(-1, &none, 0); }
static void case_lc_kernel_send_signal(void) {
    int32_t self = lc_kernel_get_process_id();
    enter();
    (void)lc_kernel_send_signal(self, 0);
}
static void case_lc_kernel_mount(void) { enter(); (void)lc_kernel_mount(NULL, NULL, NULL, 0, NULL); }
static void case_lc_kernel_umount(void) { enter(); (void)lc_kernel_umount("/nonexistent-lightc", 0); }
static void case_lc_kernel_pivot_root(void) { enter(); (void)lc_kernel_pivot_root("/nonexistent-lightc", "/nonexistent-lightc"); }
static void case_lc_kernel_chdir(void) { enter(); (void)lc_kernel_chdir("/"); }
static void case_lc_kernel_mkdirat(void) { enter(); (void)lc_kernel_mkdirat(-1, "x", 0); }
static void case_lc_kernel_mknodat(void) { enter(); (void)lc_kernel_mknodat(-1, "x", 0, 0); }
static void case_lc_kernel_symlinkat(void) { enter(); (void)lc_kernel_symlinkat("x", -1, "y"); }
static void case_lc_kernel_unlinkat(void) { enter(); (void)lc_kernel_unlinkat(-1, "x", 0); }
static void case_lc_kernel_readlinkat(void) { char b[16]; enter(); (void)lc_kernel_readlinkat(-1, "x", b, sizeof b); }
static void case_lc_kernel_fchownat(void) { enter(); (void)lc_kernel_fchownat(-1, "x", 0, 0, 0); }
static void case_lc_kernel_fchmodat(void) {
    enter();
    (void)lc_kernel_fchmodat(-1, "x", 0, 0);
    (void)lc_kernel_fchmodat(-1, "x", 0, AT_SYMLINK_NOFOLLOW);
}
static void case_lc_kernel_faccessat2(void) { enter(); (void)lc_kernel_faccessat2(-1, "x", F_OK, 0); }
static void case_lc_kernel_unshare(void) { enter(); (void)lc_kernel_unshare(0); }
static void case_lc_kernel_setns(void) { enter(); (void)lc_kernel_setns(-1, 0); }
static void case_lc_kernel_sethostname(void) { enter(); (void)lc_kernel_sethostname("x", 1000); }
static void case_lc_kernel_prctl(void) { enter(); (void)lc_kernel_prctl(PR_GET_DUMPABLE, 0, 0, 0, 0); }
static void case_lc_kernel_seccomp(void) { enter(); (void)lc_kernel_seccomp(0xffff, 0, NULL); }
static void case_lc_kernel_getuid(void) { enter(); (void)lc_kernel_getuid(); }
static void case_lc_kernel_setuid(void) { enter(); (void)lc_kernel_setuid((uint32_t)-1); }
static void case_lc_kernel_setgid(void) { enter(); (void)lc_kernel_setgid((uint32_t)-1); }
static void case_lc_kernel_setgroups(void) { enter(); (void)lc_kernel_setgroups(70000, NULL); }
static void case_lc_kernel_setresuid(void) { enter(); (void)lc_kernel_setresuid((uint32_t)-1, (uint32_t)-1, (uint32_t)-1); }
static void case_lc_kernel_setresgid(void) { enter(); (void)lc_kernel_setresgid((uint32_t)-1, (uint32_t)-1, (uint32_t)-1); }
static void case_lc_kernel_io_ring_setup(void) { uint8_t params[128] = {0}; enter(); (void)lc_kernel_io_ring_setup(0, params); }
static void case_lc_kernel_io_ring_enter(void) { enter(); (void)lc_kernel_io_ring_enter(-1, 0, 0, 0); }

/* lightc/io.h */

static lc_writer writer(void) {
    lc_writer w = lc_writer_create(devnull(), 64);
    (void)lc_writer_put_byte(&w, 'x');
    return w;
}
static lc_reader reader(void) { return lc_reader_create(devnull(), 64); }

static void case_lc_writer_create(void) { int32_t fd = devnull(); enter(); lc_writer w = lc_writer_create(fd, 64); (void)w; }
static void case_lc_writer_destroy(void) { lc_writer w = writer(); enter(); (void)lc_writer_destroy(&w); }
static void case_lc_writer_flush(void) { lc_writer w = writer(); enter(); (void)lc_writer_flush(&w); }
static void case_lc_writer_put_byte(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_byte(&w, 'y'); }
static void case_lc_writer_put_char(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_char(&w, 'y'); }
static void case_lc_writer_put_hex(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_hex(&w, 0xabc); }
static void case_lc_writer_put_line(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_line(&w, "line", 4); }
static void case_lc_writer_put_newline(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_newline(&w); }
static void case_lc_writer_put_signed(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_signed(&w, -42); }
static void case_lc_writer_put_string(void) {
    lc_writer w = writer();
    static const char big[200] = "a string longer than the buffer, so the put writes";
    enter();
    (void)lc_writer_put_string(&w, big, sizeof big);
}
static void case_lc_writer_put_unsigned(void) { lc_writer w = writer(); enter(); (void)lc_writer_put_unsigned(&w, 42); }
static void case_lc_reader_create(void) { int32_t fd = devnull(); enter(); lc_reader r = lc_reader_create(fd, 64); (void)r; }
static void case_lc_reader_destroy(void) { lc_reader r = reader(); enter(); lc_reader_destroy(&r); }
static void case_lc_reader_is_end(void) { lc_reader r = reader(); enter(); (void)lc_reader_is_end(&r); }
static void case_lc_reader_read_byte(void) { lc_reader r = reader(); enter(); (void)lc_reader_read_byte(&r); }
static void case_lc_reader_read_bytes(void) { lc_reader r = reader(); char b[8]; enter(); (void)lc_reader_read_bytes(&r, b, sizeof b); }
static void case_lc_reader_read_line(void) { lc_reader r = reader(); char b[8]; enter(); (void)lc_reader_read_line(&r, b, sizeof b); }
static void case_lc_file_read_all(void) {
    uint8_t *data;
    size_t size;
    enter();
    (void)lc_file_read_all("/proc/self/stat", &data, &size);
}
static void case_lc_file_write_all(void) { enter(); (void)lc_file_write_all("/dev/null", "x", 1); }
static void case_lc_file_get_size(void) { enter(); (void)lc_file_get_size("/dev/null"); }
static void case_lc_directory_open(void) { lc_directory d; enter(); (void)lc_directory_open(&d, "/"); }
static void case_lc_directory_next(void) {
    lc_directory d;
    lc_directory_entry e;
    (void)lc_directory_open(&d, "/");
    enter();
    (void)lc_directory_next(&d, &e);
}
static void case_lc_directory_close(void) { lc_directory d; (void)lc_directory_open(&d, "/"); enter(); lc_directory_close(&d); }

/* lightc/socket.h */

static int32_t tcp(void) { return (int32_t)lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0).value; }
static const uint8_t loopback6[16] = { [15] = 1 };

static void case_lc_host_to_network_16(void) { enter(); (void)lc_host_to_network_16(1); }
static void case_lc_host_to_network_32(void) { enter(); (void)lc_host_to_network_32(1); }
static void case_lc_network_to_host_16(void) { enter(); (void)lc_network_to_host_16(1); }
static void case_lc_network_to_host_32(void) { enter(); (void)lc_network_to_host_32(1); }
static void case_lc_socket_address_create(void) { enter(); (void)lc_socket_address_create(127, 0, 0, 1, 80); }
static void case_lc_socket_address_any(void) { enter(); (void)lc_socket_address_any(80); }
static void case_lc_socket_address6_create(void) { enter(); (void)lc_socket_address6_create(loopback6, 80); }
static void case_lc_socket_address6_loopback(void) { enter(); (void)lc_socket_address6_loopback(80); }
static void case_lc_socket_address6_any(void) { enter(); (void)lc_socket_address6_any(80); }
/* The deprecated forms still get a case, as every function of the header does. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static void case_lc_socket_create(void) { enter(); (void)lc_socket_create(LC_SOCK_STREAM); }
static void case_lc_socket_accept(void) { lc_socket_address a; enter(); (void)lc_socket_accept(-1, &a); }
#pragma GCC diagnostic pop
static void case_lc_socket_open(void) { enter(); (void)lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0); }
static void case_lc_socket_accept_flags(void) { lc_socket_address a; uint32_t n = sizeof a; enter(); (void)lc_socket_accept_flags(-1, &a, &n, LC_SOCK_NONBLOCK); }
static void case_lc_socket_get_option(void) { int32_t v; uint32_t n = sizeof v; enter(); (void)lc_socket_get_option(-1, LC_SOL_SOCKET, LC_SO_ERROR, &v, &n); }
static void case_lc_socket_set_option(void) { int32_t v = 1; enter(); (void)lc_socket_set_option(-1, LC_SOL_SOCKET, LC_SO_REUSEADDR, &v, sizeof v); }
static void case_lc_socket_send_to_sockaddr(void) { lc_socket_address a = lc_socket_address_any(1); enter(); (void)lc_socket_send_to_sockaddr(-1, "x", 1, 0, &a, sizeof a); }
static void case_lc_socket_receive_from_sockaddr(void) { char b[8]; lc_socket_address a; uint32_t n = sizeof a; enter(); (void)lc_socket_receive_from_sockaddr(-1, b, sizeof b, 0, &a, &n); }
static void case_lc_socket_close(void) { enter(); lc_socket_close(-1); }
static void case_lc_socket_set_reuse_address(void) { int32_t fd = tcp(); enter(); (void)lc_socket_set_reuse_address(fd); }
static void case_lc_socket_bind(void) {
    int32_t fd = tcp();
    lc_socket_address a = lc_socket_address_create(127, 0, 0, 1, 0);
    enter();
    (void)lc_socket_bind(fd, &a);
}
static void case_lc_socket_listen(void) { int32_t fd = tcp(); enter(); (void)lc_socket_listen(fd, 1); }
static void case_lc_socket_connect(void) { lc_socket_address a = lc_socket_address_any(1); enter(); (void)lc_socket_connect(-1, &a); }
static void case_lc_socket_send(void) { enter(); (void)lc_socket_send(-1, "x", 1); }
static void case_lc_socket_receive(void) { char b[8]; enter(); (void)lc_socket_receive(-1, b, sizeof b); }
static void case_lc_socket_send_to(void) { lc_socket_address a = lc_socket_address_any(1); enter(); (void)lc_socket_send_to(-1, "x", 1, &a); }
static void case_lc_socket_receive_from(void) { char b[8]; lc_socket_address a; enter(); (void)lc_socket_receive_from(-1, b, sizeof b, &a); }
static void case_lc_socket_shutdown(void) { enter(); (void)lc_socket_shutdown(-1, 0); }
static void case_lc_socket_listen_on(void) { lc_socket_address a = lc_socket_address_any(0); enter(); (void)lc_socket_listen_on(&a, 1); }
static void case_lc_socket_connect_to(void) { enter(); (void)lc_socket_connect_to(127, 0, 0, 1, 1); }
static void case_lc_socket_tcp6(void) { enter(); (void)lc_socket_tcp6(); }
static void case_lc_socket_udp6(void) { enter(); (void)lc_socket_udp6(); }
static void case_lc_socket_bind6(void) { lc_socket_address6 a = lc_socket_address6_loopback(0); enter(); (void)lc_socket_bind6(-1, &a); }
static void case_lc_socket_listen6(void) { enter(); (void)lc_socket_listen6(-1, 1); }
static void case_lc_socket_accept6(void) { lc_socket_address6 a; enter(); (void)lc_socket_accept6(-1, &a); }
static void case_lc_socket_connect6(void) { lc_socket_address6 a = lc_socket_address6_loopback(1); enter(); (void)lc_socket_connect6(-1, &a); }

/* lightc/time.h */

static void case_lc_time_now(void) { enter(); (void)lc_time_now(); }
static void case_lc_time_now_unix(void) { enter(); (void)lc_time_now_unix(); }
static void case_lc_time_now_monotonic(void) { enter(); (void)lc_time_now_monotonic(); }
static void case_lc_time_now_ns(void) { enter(); (void)lc_time_now_ns(LC_CLOCK_REALTIME); }
static void case_lc_time_from_unix(void) { enter(); (void)lc_time_from_unix(0); }
static void case_lc_time_to_unix(void) { lc_date_time dt = lc_time_from_unix(0); enter(); (void)lc_time_to_unix(&dt); }
static void case_lc_time_start_timer(void) { enter(); (void)lc_time_start_timer(); }
static void case_lc_time_elapsed_nanoseconds(void) { int64_t t = lc_time_start_timer(); enter(); (void)lc_time_elapsed_nanoseconds(t); }
static void case_lc_time_elapsed_microseconds(void) { int64_t t = lc_time_start_timer(); enter(); (void)lc_time_elapsed_microseconds(t); }
static void case_lc_time_elapsed_milliseconds(void) { int64_t t = lc_time_start_timer(); enter(); (void)lc_time_elapsed_milliseconds(t); }
static void case_lc_time_format(void) { lc_date_time dt = lc_time_from_unix(0); char b[64]; enter(); (void)lc_time_format(&dt, b, sizeof b); }
static void case_lc_time_format_date(void) { lc_date_time dt = lc_time_from_unix(0); char b[64]; enter(); (void)lc_time_format_date(&dt, b, sizeof b); }
static void case_lc_time_format_time(void) { lc_date_time dt = lc_time_from_unix(0); char b[64]; enter(); (void)lc_time_format_time(&dt, b, sizeof b); }
static void case_lc_time_format_iso8601(void) { lc_date_time dt = lc_time_from_unix(0); char b[64]; enter(); (void)lc_time_format_iso8601(&dt, b, sizeof b); }
static void case_lc_time_sleep_milliseconds(void) { enter(); lc_time_sleep_milliseconds(1); }
static void case_lc_time_sleep_microseconds(void) { enter(); lc_time_sleep_microseconds(1); }
static void case_lc_time_sleep_until(void) { enter(); lc_time_sleep_until(1); }

/* lightc/signal.h */

static void case_lc_signal_handle(void) { enter(); (void)lc_signal_handle(LC_SIGQUIT, any_handler); }
static void case_lc_signal_block(void) { enter(); (void)lc_signal_block(LC_SIGQUIT); }
static void case_lc_signal_unblock(void) { enter(); (void)lc_signal_unblock(LC_SIGQUIT); }
static void case_lc_on_crash(void) { enter(); lc_on_crash(any_handler); }
static void case_lc_on_shutdown(void) { enter(); lc_on_shutdown(any_handler); }
static void case_lc_signal_ignore(void) { enter(); (void)lc_signal_ignore(LC_SIGQUIT); }
static void case_lc_signal_reset(void) { enter(); (void)lc_signal_reset(LC_SIGQUIT); }

/* lightc/print.h */

static void case_lc_print_char(void) { int32_t fd = devnull(); enter(); lc_print_char(fd, 'x'); }
static void case_lc_print_string(void) { int32_t fd = devnull(); enter(); lc_print_string(fd, "x", 1); }
static void case_lc_print_line(void) { int32_t fd = devnull(); enter(); lc_print_line(fd, "x", 1); }
static void case_lc_print_newline(void) { int32_t fd = devnull(); enter(); lc_print_newline(fd); }
static void case_lc_print_signed(void) { int32_t fd = devnull(); enter(); lc_print_signed(fd, -42); }
static void case_lc_print_unsigned(void) { int32_t fd = devnull(); enter(); lc_print_unsigned(fd, 42); }
static void case_lc_print_hex(void) { int32_t fd = devnull(); enter(); lc_print_hex(fd, 0xabc); }
static void case_lc_print_byte_hex(void) { int32_t fd = devnull(); enter(); lc_print_byte_hex(fd, 0xab); }
static void case_lc_print_double(void) { int32_t fd = devnull(); enter(); lc_print_double(fd, 1.5); }

/* lightc/log.h: each case sends the log to /dev/null first. */

static void quiet(void) { lc_log_set_output(devnull()); }
static void case_lc_log_set_level(void) { enter(); lc_log_set_level(LC_LOG_INFO); }
static void case_lc_log_set_output(void) { enter(); lc_log_set_output(2); }
static void case_lc_log_set_format(void) { enter(); lc_log_set_format(LC_LOG_FORMAT_TEXT); }
static void case_lc_log(void) { quiet(); enter(); lc_log(LC_LOG_INFO, "x"); }
static void case_lc_log_string(void) { quiet(); enter(); lc_log_string(LC_LOG_INFO, "x", 1); }
static void case_lc_log_begin(void) { lc_log_entry e; quiet(); enter(); lc_log_begin(&e, LC_LOG_INFO); }
static void case_lc_log_commit(void) { lc_log_entry e; quiet(); lc_log_begin(&e, LC_LOG_INFO); enter(); lc_log_commit(&e); }
static void case_lc_log_is_enabled(void) { enter(); (void)lc_log_is_enabled(LC_LOG_INFO); }

/* lightc/probe.h: the child has no answer kept yet, as the suite never
 * probes, so each probe makes its calls. */

static void case_lc_probe_kernel(void) {
    enter();
    for (int32_t f = LC_KFEAT_FCHMODAT2; f <= (int32_t)LC_KFEAT_IO_URING; f++)
        (void)lc_probe_kernel((lc_kernel_feature)f);
}

/* --- The table --- */

typedef struct {
    const char *name;
    void      (*run)(void);
    const char *also;   /* a system call the case itself names, for lc_syscallN */
} index_case;

#define CASE(f) { #f, case_##f, NULL }
/* lc_syscallN by its N, so that ci/abilint.sh, which checks the arity of
 * each lc_syscallN written out, sees only the calls. */
#define CASE_SYSCALL(n, s) { "lc_syscall" #n, case_lc_syscall##n, s }

static const index_case cases[] = {
    CASE_SYSCALL(0, "getpid"), CASE_SYSCALL(1, "close"), CASE_SYSCALL(2, "munmap"), CASE_SYSCALL(3, "write"),
    CASE_SYSCALL(4, "rt_sigprocmask"), CASE_SYSCALL(5, "prctl"), CASE_SYSCALL(6, "futex"),
    CASE(lc_kernel_exit), CASE(lc_kernel_get_process_id), CASE(lc_kernel_fork), CASE(lc_kernel_clone),
    CASE(lc_kernel_execute), CASE(lc_kernel_wait_for_child), CASE(lc_kernel_create_pipe),
    CASE(lc_kernel_duplicate_fd), CASE(lc_kernel_create_session), CASE(lc_kernel_open_file),
    CASE(lc_kernel_open_file_at), CASE(lc_kernel_close_file), CASE(lc_kernel_read_bytes),
    CASE(lc_kernel_write_bytes), CASE(lc_kernel_write_all), CASE(lc_kernel_read_retry), CASE(lc_kernel_read_all),
    CASE(lc_kernel_seek_position), CASE(lc_kernel_device_control), CASE(lc_kernel_read_directory),
    CASE(lc_kernel_getsockname), CASE(lc_kernel_map_memory), CASE(lc_kernel_unmap_memory),
    CASE(lc_kernel_protect_memory), CASE(lc_kernel_set_heap_end), CASE(lc_kernel_advise_memory),
    CASE(lc_kernel_get_thread_id), CASE(lc_kernel_futex_wait), CASE(lc_kernel_futex_wake),
    CASE(lc_kernel_get_clock), CASE(lc_kernel_sleep), CASE(lc_kernel_clock_sleep), CASE(lc_kernel_reset_signal),
    CASE(lc_kernel_set_signal_mask), CASE(lc_kernel_create_signal_fd), CASE(lc_kernel_send_signal),
    CASE(lc_kernel_mount), CASE(lc_kernel_umount), CASE(lc_kernel_pivot_root), CASE(lc_kernel_chdir),
    CASE(lc_kernel_mkdirat), CASE(lc_kernel_mknodat), CASE(lc_kernel_symlinkat), CASE(lc_kernel_unlinkat),
    CASE(lc_kernel_readlinkat), CASE(lc_kernel_fchownat), CASE(lc_kernel_fchmodat), CASE(lc_kernel_faccessat2),
    CASE(lc_kernel_unshare), CASE(lc_kernel_setns), CASE(lc_kernel_sethostname), CASE(lc_kernel_prctl),
    CASE(lc_kernel_seccomp), CASE(lc_kernel_getuid), CASE(lc_kernel_setuid), CASE(lc_kernel_setgid),
    CASE(lc_kernel_setgroups), CASE(lc_kernel_io_ring_setup), CASE(lc_kernel_io_ring_enter),
    CASE(lc_kernel_setresuid), CASE(lc_kernel_setresgid),

    CASE(lc_writer_create), CASE(lc_writer_destroy), CASE(lc_writer_flush), CASE(lc_writer_put_byte),
    CASE(lc_writer_put_char), CASE(lc_writer_put_hex), CASE(lc_writer_put_line), CASE(lc_writer_put_newline),
    CASE(lc_writer_put_signed), CASE(lc_writer_put_string), CASE(lc_writer_put_unsigned),
    CASE(lc_reader_create), CASE(lc_reader_destroy), CASE(lc_reader_is_end), CASE(lc_reader_read_byte),
    CASE(lc_reader_read_bytes), CASE(lc_reader_read_line), CASE(lc_file_read_all), CASE(lc_file_write_all),
    CASE(lc_file_get_size), CASE(lc_directory_open), CASE(lc_directory_next), CASE(lc_directory_close),

    CASE(lc_host_to_network_16), CASE(lc_host_to_network_32), CASE(lc_network_to_host_16),
    CASE(lc_network_to_host_32), CASE(lc_socket_address_create), CASE(lc_socket_address_any),
    CASE(lc_socket_address6_create), CASE(lc_socket_address6_loopback), CASE(lc_socket_address6_any),
    CASE(lc_socket_create), CASE(lc_socket_close), CASE(lc_socket_set_reuse_address), CASE(lc_socket_bind),
    CASE(lc_socket_listen), CASE(lc_socket_accept), CASE(lc_socket_connect), CASE(lc_socket_send),
    CASE(lc_socket_receive), CASE(lc_socket_send_to), CASE(lc_socket_receive_from), CASE(lc_socket_shutdown),
    CASE(lc_socket_listen_on), CASE(lc_socket_connect_to), CASE(lc_socket_tcp6), CASE(lc_socket_udp6),
    CASE(lc_socket_bind6), CASE(lc_socket_listen6), CASE(lc_socket_accept6), CASE(lc_socket_connect6),
    CASE(lc_socket_open), CASE(lc_socket_accept_flags), CASE(lc_socket_get_option),
    CASE(lc_socket_set_option), CASE(lc_socket_send_to_sockaddr), CASE(lc_socket_receive_from_sockaddr),

    CASE(lc_time_now), CASE(lc_time_now_unix), CASE(lc_time_now_monotonic), CASE(lc_time_from_unix),
    CASE(lc_time_to_unix), CASE(lc_time_start_timer), CASE(lc_time_elapsed_nanoseconds),
    CASE(lc_time_elapsed_microseconds), CASE(lc_time_elapsed_milliseconds), CASE(lc_time_format),
    CASE(lc_time_format_date), CASE(lc_time_format_time), CASE(lc_time_format_iso8601),
    CASE(lc_time_sleep_milliseconds), CASE(lc_time_sleep_microseconds), CASE(lc_time_now_ns),
    CASE(lc_time_sleep_until),

    CASE(lc_signal_handle), CASE(lc_signal_block), CASE(lc_signal_unblock), CASE(lc_on_crash),
    CASE(lc_on_shutdown), CASE(lc_signal_ignore), CASE(lc_signal_reset),

    CASE(lc_print_char), CASE(lc_print_string), CASE(lc_print_line), CASE(lc_print_newline),
    CASE(lc_print_signed), CASE(lc_print_unsigned), CASE(lc_print_hex), CASE(lc_print_byte_hex),
    CASE(lc_print_double),

    CASE(lc_log_set_level), CASE(lc_log_set_output), CASE(lc_log_set_format), CASE(lc_log),
    CASE(lc_log_string), CASE(lc_log_begin), CASE(lc_log_commit), CASE(lc_log_is_enabled),

    CASE(lc_probe_kernel),
};

#define CASE_COUNT (sizeof cases / sizeof cases[0])

/* The headers whose every function has a case. */
static const char *const covered[] = {
    "lightc/syscall.h", "lightc/io.h", "lightc/socket.h", "lightc/time.h",
    "lightc/signal.h", "lightc/print.h", "lightc/log.h",
    "lightc/probe.h",
};

/* --- Running a case --- */

static void print_name(const index_case *c) {
    _test_print_text("  ");
    _test_print_text(c->name);
    _test_print_text(": ");
}

/* Runs one case in a child, and prints what went wrong; true if nothing did. */
static bool run_case(const index_case *c) {
    const char *row;
    size_t row_length;
    if (!index_row(c->name, &row, &row_length)) {
        print_name(c);
        _test_print_text("not in api/index.md\n");
        return false;
    }
    clone_safe = row_clone_safe(row, row_length);

    int32_t allowed[MAX_ALLOWED];
    size_t count = 0;
    allowed[count++] = SYS_exit_group;
    if (c->also) allowed[count++] = syscall_number(c->also, lc_string_length(c->also));
    size_t at = 0, n = lc_string_length(c->name);
    const char *line;
    size_t ln;
    while (next_line(&tsv, &at, &line, &ln)) {
        const char *f, *s, *a;
        size_t fn, sn, an;
        if (!field(line, ln, '\t', 0, &f, &fn) || fn != n || lc_bytes_compare(f, c->name, n) != 0) continue;
        if (!field(line, ln, '\t', 1, &s, &sn) || !field(line, ln, '\t', 2, &a, &an)) continue;
        if (!has_word(a, an, THIS_ARCH)) continue;
        int32_t nr = syscall_number(s, sn);
        if (nr < 0 || count == MAX_ALLOWED) {
            print_name(c);
            _test_print_text("api/syscalls.tsv lists a system call syscalls.def does not number: ");
            lc_print_string(TEST_FD, s, sn);
            _test_print_text("\n");
            return false;
        }
        allowed[count++] = nr;
    }
    build_filter(allowed, count);

    report->entered = 0;
    report->syscall = -1;
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 10, 0 };   /* struct itimerval: 10 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        c->run();
        lc_kernel_exit(0);
    }
    if (pid < 0) {
        print_name(c);
        _test_print_text("fork failed\n");
        return false;
    }
    int32_t status = 0;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    int32_t signal = status & 0x7f, code = (status >> 8) & 0xff;
    if (!report->entered) {
        print_name(c);
        _test_print_text("the case never called enter()\n");
        return false;
    }
    if (signal == 0 && code == 0) return true;
    print_name(c);
    if (signal == 0 && code == 99) {
        _test_print_text("made ");
        print_syscall(report->syscall);
        _test_print_text(", which api/syscalls.tsv does not list for it on " THIS_ARCH "\n");
    } else if (signal == 0 && code == 98) {
        _test_print_text("the seccomp filter could not be installed\n");
    } else if (signal == LC_SIGSEGV && clone_safe) {
        _test_print_text("died of SIGSEGV with the program's data read-only and no thread pointer,"
                         " though api/index.md calls it clone-safe\n");
    } else if (signal == LC_SIGALRM) {
        _test_print_text("still running after 10 s\n");
    } else if (signal != 0) {
        _test_print_text("died of signal ");
        lc_print_signed(TEST_FD, signal);
        _test_print_text("\n");
    } else {
        _test_print_text("exited with status ");
        lc_print_signed(TEST_FD, code);
        _test_print_text("\n");
    }
    return false;
}

/* Does a seccomp filter work here? A child installs one that allows every
 * call of this arch, as the filters of the cases check the arch too: an
 * emulator that handed the filter to its host process instead of refusing
 * it would see the child killed, and the suite skips. */
static bool seccomp_works(void) {
    static int answer = -1;
    if (answer >= 0) return answer;
    static bpf_insn allow_all[4] = {
        { BPF_LD_W_ABS, 0, 0, 4 },
        { BPF_JEQ_K, 1, 0, AUDIT_ARCH_THIS },
        { BPF_RET_K, 0, 0, SECCOMP_RET_KILL_PROCESS },
        { BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW },
    };
    static bpf_program all = { 4, allow_all };
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        (void)lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
        lc_kernel_exit(lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &all) == 0 ? 0 : 1);
    }
    int32_t status = 1;
    if (pid > 0) (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    answer = status == 0;
    return answer;
}

static bool setup(void) {
    if (!load_all()) return false;
    if (!report) {
        report = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        tls_guard = lc_kernel_map_memory(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (report == MAP_FAILED || tls_guard == MAP_FAILED) {
            _test_print_text("  cannot map the report and guard pages\n");
            report = NULL;
            return false;
        }
        if (!find_data_segment()) {
            _test_print_text("  cannot find the program's writable segment in /proc/self/exe\n");
            report = NULL;
            return false;
        }
    }
    return true;
}

/* Runs the case of every function of `header`. */
static void check_header(const char *header) {
    TEST_REQUIRE(setup());
    if (!seccomp_works()) TEST_SKIP("seccomp filters do not work here: qemu-user does not implement them");
    size_t ran = 0, failed = 0, hn = lc_string_length(header);
    for (size_t i = 0; i < CASE_COUNT; i++) {
        const char *row, *h;
        size_t rn, n;
        if (!index_row(cases[i].name, &row, &rn) || !row_header(row, rn, &h, &n)) continue;
        if (n != hn || lc_bytes_compare(h, header, hn) != 0) continue;
        ran++;
        if (!run_case(&cases[i])) failed++;
    }
    _test_print_text("  ");
    lc_print_unsigned(TEST_FD, ran);
    _test_print_text(" functions run under what the index says of them\n");
    TEST_ASSERT(ran > 0);
    TEST_ASSERT_EQ(failed, (size_t)0);
}

static void test_syscall_h(void)  { check_header("lightc/syscall.h"); }
static void test_io_h(void)       { check_header("lightc/io.h"); }
static void test_socket_h(void)   { check_header("lightc/socket.h"); }
static void test_time_h(void)     { check_header("lightc/time.h"); }
static void test_signal_h(void)   { check_header("lightc/signal.h"); }
static void test_print_h(void)    { check_header("lightc/print.h"); }
static void test_log_h(void)      { check_header("lightc/log.h"); }
static void test_probe_h(void)    { check_header("lightc/probe.h"); }

static void test_every_function_has_a_case(void) {
    TEST_REQUIRE(load_all());
    size_t missing = 0, unknown = 0, at = 0;
    const char *line;
    size_t ln;
    while (next_line(&index_md, &at, &line, &ln)) {
        const char *h;
        size_t hn;
        if (ln < 4 || lc_bytes_compare(line, "| `", 3) != 0 || !row_header(line, ln, &h, &hn)) continue;
        bool is_covered = false;
        for (size_t i = 0; i < sizeof covered / sizeof covered[0]; i++)
            if (same(h, hn, covered[i])) is_covered = true;
        if (!is_covered) continue;
        size_t nn = 0;
        while (3 + nn < ln && line[3 + nn] != '`') nn++;
        bool found = false;
        for (size_t i = 0; i < CASE_COUNT; i++)
            if (same(line + 3, nn, cases[i].name)) found = true;
        if (!found) {
            _test_print_text("  no case for ");
            lc_print_string(TEST_FD, line + 3, nn);
            _test_print_text(", of ");
            lc_print_string(TEST_FD, h, hn);
            _test_print_text("\n");
            missing++;
        }
    }
    for (size_t i = 0; i < CASE_COUNT; i++) {
        const char *row;
        size_t rn;
        if (!index_row(cases[i].name, &row, &rn)) {
            _test_print_text("  a case for ");
            _test_print_text(cases[i].name);
            _test_print_text(", which api/index.md does not list\n");
            unknown++;
        }
    }
    TEST_ASSERT_EQ(missing, (size_t)0);
    TEST_ASSERT_EQ(unknown, (size_t)0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    find_root(argv[0]);
    TEST_RUN(test_every_function_has_a_case);
    TEST_RUN(test_syscall_h);
    TEST_RUN(test_io_h);
    TEST_RUN(test_socket_h);
    TEST_RUN(test_time_h);
    TEST_RUN(test_signal_h);
    TEST_RUN(test_print_h);
    TEST_RUN(test_log_h);
    TEST_RUN(test_probe_h);
    return test_main();
}
