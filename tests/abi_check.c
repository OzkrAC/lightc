/*
 * abi_check.c — compile-time check of lightc's kernel ABI against the Linux
 * UAPI headers.
 *
 * Compiling this file is the whole check: it has no main and is never linked.
 * Each _Static_assert compares a value or a layout of lightc's headers with
 * the one the kernel headers give for the target arch, so compile it once
 * per arch, with that arch's kernel headers installed:
 *
 *   gcc -std=c2x -ffreestanding -Wall -Wextra -Wpedantic -Werror \
 *       -Iinclude -fsyntax-only tests/abi_check.c
 *   aarch64-linux-gnu-gcc (same flags)
 *
 * It checks:
 *
 *   - Every SYS_* number and LC_ERR_* errno value, against __NR_* and E*.
 *     The lists of syscalls and errnos are generated from syscalls.def, from
 *     which tools/gensyscalls.sh also generates the SYS_* and LC_ERR_*
 *     definitions, so every number and errno value in syscalls.def is
 *     checked.
 *   - Every macro of the public headers (include/, but for the generated
 *     syscall_nr.h files, whose SYS_* names the syscall lists check) and of
 *     src/kernel.h, the kernel ABI that only the library's sources use. Each
 *     is in one of the lists below: compared with the UAPI value
 *     (UAPI_CONSTANTS, UAPI_CONSTANTS_X86_64, UAPI_RENAMED, UAPI_FUNCTIONS),
 *     or listed with the reason no UAPI header can check it (NO_UAPI_CHECK),
 *     or as lightc's own, no kernel value (LIGHTC_OWN). ci/abilint.sh fails
 *     when a macro of those headers is in none of these lists, so a new one
 *     needs a check or a reason, and when a name in them is no such macro.
 *   - Every struct lightc shares with the kernel: its size, and the offset
 *     and size of each field.
 *
 * Many lightc macros have the UAPI's names (O_*, MAP_*, SIG*, MS_*, ELF and
 * io_uring names, ...). Each such UAPI value is captured as an enum constant
 * UAPI_<name>, the UAPI macro is #undef'd, and only then are lightc's headers
 * included, so every assert compares lightc's definition with the kernel's.
 * src/kernel.h's Elf64_* typedefs would clash with <linux/elf.h>'s, so that
 * file is included with them renamed lightc_Elf64_*. SYS_* numbers are
 * compared with __NR_* directly, and LC_ERR_* values with E* directly; those
 * names do not clash.
 *
 * The UAPI headers are those of Linux 6.8 in CI, and nothing here may need a
 * newer one.
 */

#include <stddef.h>          /* offsetof */

#include <asm/errno.h>       /* E* */
#include <asm/signal.h>      /* SIG*, SA_RESTORER, struct sigaction */
#include <asm/socket.h>      /* SOL_SOCKET, SO_* */
#include <asm/unistd.h>      /* __NR_* */
#include <linux/auxvec.h>    /* AT_NULL, AT_PAGESZ */
#include <linux/elf.h>       /* Elf64_*, ELFMAG*, PT_*, DT_*, ... */
#include <linux/fcntl.h>     /* O_*, AT_* */
#include <linux/fs.h>        /* SEEK_* */
#include <linux/futex.h>     /* FUTEX_* */
#include <linux/in.h>        /* IPPROTO_*, INADDR_*, struct sockaddr_in */
#include <linux/in6.h>       /* struct sockaddr_in6 */
#include <linux/io_uring.h>  /* struct io_uring_sqe, IORING_* */
#include <linux/mman.h>      /* PROT_*, MAP_*, MADV_* */
#include <linux/mount.h>     /* MS_* */
#include <linux/sched.h>     /* CLONE_* */
#include <linux/signalfd.h>  /* SFD_*, struct signalfd_siginfo */
#include <linux/stat.h>      /* S_IF* */
#include <linux/time.h>      /* CLOCK_* */
#include <linux/time_types.h>  /* struct __kernel_timespec */
#include <linux/wait.h>      /* WNOHANG */
#if defined(__x86_64__)
#include <asm/prctl.h>       /* ARCH_SET_FS */
#endif

/* --- The macros of lightc's headers, by how each is checked --- */

/* Macros with the UAPI name and value on both arches. */
#define UAPI_CONSTANTS(X)                                                   \
    /* <lightc/syscall.h> */                                                \
    X(AT_FDCWD) X(AT_REMOVEDIR) X(AT_EACCESS) X(AT_SYMLINK_NOFOLLOW)        \
    X(O_RDONLY) X(O_WRONLY) X(O_RDWR) X(O_CREAT) X(O_TRUNC) X(O_APPEND)     \
    X(O_NONBLOCK) X(O_CLOEXEC) X(O_DIRECT) X(O_DIRECTORY) X(O_NOFOLLOW)     \
    X(PROT_NONE) X(PROT_READ) X(PROT_WRITE) X(PROT_EXEC) X(MAP_SHARED)      \
    X(MAP_PRIVATE) X(MAP_FIXED) X(MAP_ANONYMOUS) X(MADV_DONTNEED)           \
    X(SEEK_SET) X(SEEK_CUR) X(SEEK_END) X(WNOHANG) X(SIGHUP) X(SIGINT)      \
    X(SIGQUIT) X(SIGKILL) X(SIGUSR1) X(SIGUSR2) X(SIGTERM) X(SIGCHLD)       \
    X(SFD_CLOEXEC) X(SFD_NONBLOCK) X(FUTEX_WAIT) X(FUTEX_WAKE) X(MS_RDONLY) \
    X(MS_NOSUID) X(MS_NODEV) X(MS_NOEXEC) X(MS_REMOUNT) X(MS_BIND)          \
    X(MS_REC) X(MS_PRIVATE) X(MS_SLAVE) X(CLONE_NEWNS) X(CLONE_NEWUTS)      \
    X(CLONE_NEWIPC) X(CLONE_NEWUSER) X(CLONE_NEWPID) X(CLONE_NEWNET)        \
    /* src/kernel.h */                                                      \
    X(IORING_OFF_SQ_RING) X(IORING_OFF_CQ_RING) X(IORING_OFF_SQES)          \
    X(IORING_OP_TIMEOUT) X(IORING_OP_ACCEPT) X(IORING_OP_READ)              \
    X(IORING_OP_WRITE) X(IORING_ENTER_GETEVENTS) X(IORING_FEAT_SINGLE_MMAP) \
    X(MAP_POPULATE) X(CLONE_VM) X(CLONE_FS) X(CLONE_FILES) X(CLONE_SIGHAND) \
    X(CLONE_THREAD) X(CLONE_SYSVSEM) X(CLONE_PARENT_SETTID)                 \
    X(CLONE_CHILD_CLEARTID) X(CLONE_SETTLS) X(SA_RESTORER) X(ELFMAG0)       \
    X(ELFMAG1) X(ELFMAG2) X(ELFMAG3) X(ELFCLASS64) X(ELFDATA2LSB) X(ET_DYN) \
    X(EM_X86_64) X(EM_AARCH64) X(PT_NULL) X(PT_LOAD) X(PT_DYNAMIC) X(PF_X)  \
    X(PF_W) X(PF_R) X(DT_NULL) X(DT_NEEDED) X(DT_HASH) X(DT_STRTAB)         \
    X(DT_SYMTAB) X(DT_RELA) X(DT_RELASZ) X(DT_RELAENT) X(DT_STRSZ)          \
    X(DT_SYMENT) X(DT_INIT) X(DT_FINI) X(DT_RELACOUNT) X(STB_GLOBAL)        \
    X(STB_WEAK) X(STT_FUNC) X(STT_OBJECT) X(SHN_UNDEF)

/* The same, on x86_64 only: aarch64 has no arch_prctl. */
#define UAPI_CONSTANTS_X86_64(X)                                            \
    /* src/kernel.h */                                                      \
    X(ARCH_SET_FS)

/*
 * X(lightc's name, the UAPI's): macros with the value of a UAPI macro of
 * another name, or of an expression of UAPI values. getdents64 gives a
 * file's type as the kernel's S_DT(mode), bits 12 to 15 of its mode
 * (include/linux/fs_types.h), so each LC_DT_* is an S_IF* shifted right
 * by 12.
 */
#define UAPI_RENAMED(X)                                                     \
    /* <lightc/syscall.h> */                                                \
    X(LC_SIG_BLOCK, SIG_BLOCK) X(LC_SIG_UNBLOCK, SIG_UNBLOCK)               \
    X(LC_SIG_SETMASK, SIG_SETMASK) X(LC_CLOCK_REALTIME, CLOCK_REALTIME)     \
    X(LC_CLOCK_MONOTONIC, CLOCK_MONOTONIC)                                  \
    /* <lightc/signal.h> */                                                 \
    X(LC_SIGINT, SIGINT) X(LC_SIGQUIT, SIGQUIT) X(LC_SIGABRT, SIGABRT)      \
    X(LC_SIGBUS, SIGBUS) X(LC_SIGSEGV, SIGSEGV) X(LC_SIGPIPE, SIGPIPE)      \
    X(LC_SIGALRM, SIGALRM) X(LC_SIGTERM, SIGTERM)                           \
    /* <lightc/socket.h> */                                                 \
    X(LC_IPPROTO_TCP, IPPROTO_TCP) X(LC_IPPROTO_UDP, IPPROTO_UDP)           \
    X(LC_SOL_SOCKET, SOL_SOCKET) X(LC_SO_REUSEADDR, SO_REUSEADDR)           \
    X(LC_SO_REUSEPORT, SO_REUSEPORT) X(LC_INADDR_ANY, INADDR_ANY)           \
    X(LC_INADDR_LOOPBACK, INADDR_LOOPBACK)                                  \
    /* <lightc/io.h> */                                                     \
    X(LC_DT_DIR, S_IFDIR >> 12) X(LC_DT_REG, S_IFREG >> 12)                 \
    X(LC_DT_LNK, S_IFLNK >> 12)                                             \
    /* src/kernel.h */                                                      \
    X(LC_AT_NULL, AT_NULL) X(LC_AT_PAGESZ, AT_PAGESZ)

/*
 * X(name, probe): function-like macros of the UAPI, which must give the
 * UAPI's result for the probe. Each probe has the fields of st_info or
 * r_info differ, and the bits on both sides of each edge of a field set,
 * so a mask or a shift one bit off, or the two macros of a pair swapped,
 * changes the result.
 */
#define UAPI_FUNCTIONS(X)                                                   \
    /* src/kernel.h */                                                      \
    X(ELF64_ST_BIND, 0x9d) X(ELF64_ST_TYPE, 0x9d)                           \
    X(ELF64_R_SYM, 0x9234567987654321) X(ELF64_R_TYPE, 0x9234567987654321)

/*
 * Kernel values that no UAPI header can check, so nothing checks them at
 * compile time:
 *
 *   STDIN, STDOUT, STDERR  the descriptors every process starts with: no
 *                          header of the kernel names them.
 *   MAP_FAILED, SIG_DFL, SIG_IGN
 *                          pointers, which no constant expression can
 *                          compare. MAP_FAILED is libc's ((void *)-1),
 *                          which lc_kernel_map_memory returns for an error;
 *                          SIG_DFL and SIG_IGN are handlers 0 and 1, as in
 *                          asm-generic/signal-defs.h.
 *   MNT_DETACH             in the kernel's include/linux/fs.h, which is not
 *                          UAPI.
 *   F_OK, R_OK, W_OK, X_OK faccessat2's modes, which only libc's <unistd.h>
 *                          names.
 *   LC_AF_*, LC_SOCK_*, LC_SHUT_*
 *                          in the kernel's include/linux/socket.h and
 *                          include/linux/net.h, which are not UAPI.
 *   LC_DT_UNKNOWN          the d_type of a file whose type getdents64
 *                          does not know: 0 in the kernel's
 *                          include/linux/fs_types.h, which is not UAPI.
 *   DT_INIT_ARRAY, DT_FINI_ARRAY, DT_INIT_ARRAYSZ, DT_FINI_ARRAYSZ,
 *   DT_GNU_HASH, R_X86_64_*, R_AARCH64_*
 *                          ELF values that <linux/elf.h> leaves out: the
 *                          first four are the ELF gABI's, DT_GNU_HASH is
 *                          GNU's, and the relocation types are each arch's
 *                          psABI's, in the kernel's arch/x86 and arch/arm64
 *                          asm/elf.h, which are not UAPI.
 */
#define NO_UAPI_CHECK(X)                                                    \
    /* <lightc/syscall.h> */                                                \
    X(STDIN) X(STDOUT) X(STDERR) X(MAP_FAILED) X(SIG_DFL) X(SIG_IGN)        \
    X(MNT_DETACH) X(F_OK) X(R_OK) X(W_OK) X(X_OK)                           \
    /* <lightc/socket.h> */                                                 \
    X(LC_AF_INET) X(LC_AF_INET6) X(LC_SOCK_STREAM) X(LC_SOCK_DGRAM)         \
    X(LC_SHUT_READ) X(LC_SHUT_WRITE) X(LC_SHUT_BOTH)                        \
    /* <lightc/io.h> */                                                     \
    X(LC_DT_UNKNOWN)                                                        \
    /* src/kernel.h */                                                      \
    X(DT_INIT_ARRAY) X(DT_FINI_ARRAY) X(DT_INIT_ARRAYSZ) X(DT_FINI_ARRAYSZ) \
    X(DT_GNU_HASH) X(R_X86_64_NONE) X(R_X86_64_64) X(R_X86_64_GLOB_DAT)     \
    X(R_X86_64_JUMP_SLOT) X(R_X86_64_RELATIVE) X(R_AARCH64_NONE)            \
    X(R_AARCH64_RELATIVE) X(R_AARCH64_GLOB_DAT) X(R_AARCH64_JUMP_SLOT)      \
    X(R_AARCH64_ABS64)

/*
 * lightc's own sizes, limits, initializers and helpers: no kernel values.
 * LC_PAGE_SIZE is the page size lightc assumes when it is built;
 * lc_runtime_page_size gives the kernel's, from AT_PAGESZ.
 */
#define LIGHTC_OWN(X)                                                       \
    /* <lightc/types.h> */                                                  \
    X(LC_VERSION_MAJOR) X(LC_VERSION_MINOR) X(LC_VERSION_PATCH)             \
    /* <lightc/heap.h> and <lightc/memory.h> */                             \
    X(LC_STATS)                                                             \
    /* <lightc/memory.h> */                                                 \
    X(LC_PAGE_SIZE) X(LC_DEFAULT_ALIGNMENT)                                 \
    /* <lightc/lifecycle.h> */                                              \
    X(LC_MAX_ATEXIT_HANDLERS)                                               \
    /* <lightc/coroutine.h> */                                              \
    X(LC_COROUTINE_STACK_SIZE) X(LC_MAX_COROUTINES)                         \
    /* <lightc/thread.h> */                                                 \
    X(LC_DEFAULT_STACK_SIZE) X(LC_TLS_MAX_KEYS) X(LC_SPINLOCK_INIT)         \
    X(LC_MUTEX_INIT) X(LC_CONDVAR_INIT) X(LC_RWLOCK_INIT) X(LC_ONCE_INIT)   \
    /* <lightc/log.h> */                                                    \
    X(LC_LOG_BUFFER_SIZE)                                                   \
    /* <lightdata/list.h> */                                                \
    X(lc_list_entry) X(lc_list_for_each) X(lc_list_for_each_safe)

/* --- Capture every UAPI value that lightc's headers define too --- */

#define CAPTURE(name) UAPI_##name = (name),
#define CAPTURE_AS(lc, uapi) UAPI_AS_##lc = (uapi),
/*
 * An enum constant must fit in an int, and ELF64_R_SYM's and ELF64_R_TYPE's
 * results for their probe do not, so each result is kept in two: its low
 * 16 bits and the rest.
 */
#define CAPTURE_FUNCTION(name, probe)                                       \
    UAPI_LOW_##name = name(probe) & 0xffff,                                 \
    UAPI_HIGH_##name = name(probe) >> 16,
enum { UAPI_CONSTANTS(CAPTURE) };
#if defined(__x86_64__)
enum { UAPI_CONSTANTS_X86_64(CAPTURE) };
#endif
enum { UAPI_RENAMED(CAPTURE_AS) };
enum { UAPI_FUNCTIONS(CAPTURE_FUNCTION) };
#undef CAPTURE
#undef CAPTURE_AS
#undef CAPTURE_FUNCTION

/*
 * The names of UAPI_CONSTANTS, UAPI_CONSTANTS_X86_64 and UAPI_FUNCTIONS,
 * and the pointer constants SIG_DFL and SIG_IGN, which <lightc/syscall.h>
 * also defines. IORING_OP_* are enum constants in <linux/io_uring.h>, not
 * macros, so their #undef does nothing: src/kernel.h's macros then hide
 * them.
 */
#undef AT_FDCWD
#undef AT_REMOVEDIR
#undef AT_EACCESS
#undef AT_SYMLINK_NOFOLLOW
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_CREAT
#undef O_TRUNC
#undef O_APPEND
#undef O_NONBLOCK
#undef O_CLOEXEC
#undef O_DIRECT
#undef O_DIRECTORY
#undef O_NOFOLLOW
#undef PROT_NONE
#undef PROT_READ
#undef PROT_WRITE
#undef PROT_EXEC
#undef MAP_SHARED
#undef MAP_PRIVATE
#undef MAP_FIXED
#undef MAP_ANONYMOUS
#undef MADV_DONTNEED
#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#undef WNOHANG
#undef SIGHUP
#undef SIGINT
#undef SIGQUIT
#undef SIGKILL
#undef SIGUSR1
#undef SIGUSR2
#undef SIGTERM
#undef SIGCHLD
#undef SIG_DFL
#undef SIG_IGN
#undef SFD_CLOEXEC
#undef SFD_NONBLOCK
#undef FUTEX_WAIT
#undef FUTEX_WAKE
#undef MS_RDONLY
#undef MS_NOSUID
#undef MS_NODEV
#undef MS_NOEXEC
#undef MS_REMOUNT
#undef MS_BIND
#undef MS_REC
#undef MS_PRIVATE
#undef MS_SLAVE
#undef CLONE_NEWNS
#undef CLONE_NEWUTS
#undef CLONE_NEWIPC
#undef CLONE_NEWUSER
#undef CLONE_NEWPID
#undef CLONE_NEWNET
#undef IORING_OFF_SQ_RING
#undef IORING_OFF_CQ_RING
#undef IORING_OFF_SQES
#undef IORING_OP_TIMEOUT
#undef IORING_OP_ACCEPT
#undef IORING_OP_READ
#undef IORING_OP_WRITE
#undef IORING_ENTER_GETEVENTS
#undef IORING_FEAT_SINGLE_MMAP
#undef MAP_POPULATE
#undef CLONE_VM
#undef CLONE_FS
#undef CLONE_FILES
#undef CLONE_SIGHAND
#undef CLONE_THREAD
#undef CLONE_SYSVSEM
#undef CLONE_PARENT_SETTID
#undef CLONE_CHILD_CLEARTID
#undef CLONE_SETTLS
#undef SA_RESTORER
#undef ELFMAG0
#undef ELFMAG1
#undef ELFMAG2
#undef ELFMAG3
#undef ELFCLASS64
#undef ELFDATA2LSB
#undef ET_DYN
#undef EM_X86_64
#undef EM_AARCH64
#undef PT_NULL
#undef PT_LOAD
#undef PT_DYNAMIC
#undef PF_X
#undef PF_W
#undef PF_R
#undef DT_NULL
#undef DT_NEEDED
#undef DT_HASH
#undef DT_STRTAB
#undef DT_SYMTAB
#undef DT_RELA
#undef DT_RELASZ
#undef DT_RELAENT
#undef DT_STRSZ
#undef DT_SYMENT
#undef DT_INIT
#undef DT_FINI
#undef DT_RELACOUNT
#undef STB_GLOBAL
#undef STB_WEAK
#undef STT_FUNC
#undef STT_OBJECT
#undef SHN_UNDEF
#undef ARCH_SET_FS
#undef ELF64_ST_BIND
#undef ELF64_ST_TYPE
#undef ELF64_R_SYM
#undef ELF64_R_TYPE

/* src/kernel.h, with its Elf64_* typedefs renamed lightc_Elf64_* */
#define Elf64_Addr   lightc_Elf64_Addr
#define Elf64_Off    lightc_Elf64_Off
#define Elf64_Half   lightc_Elf64_Half
#define Elf64_Word   lightc_Elf64_Word
#define Elf64_Sword  lightc_Elf64_Sword
#define Elf64_Xword  lightc_Elf64_Xword
#define Elf64_Sxword lightc_Elf64_Sxword
#define Elf64_Ehdr   lightc_Elf64_Ehdr
#define Elf64_Phdr   lightc_Elf64_Phdr
#define Elf64_Dyn    lightc_Elf64_Dyn
#define Elf64_Sym    lightc_Elf64_Sym
#define Elf64_Rela   lightc_Elf64_Rela
#include "../src/kernel.h"
#undef Elf64_Addr
#undef Elf64_Off
#undef Elf64_Half
#undef Elf64_Word
#undef Elf64_Sword
#undef Elf64_Xword
#undef Elf64_Sxword
#undef Elf64_Ehdr
#undef Elf64_Phdr
#undef Elf64_Dyn
#undef Elf64_Sym
#undef Elf64_Rela

#include <lightc/io.h>
#include <lightc/signal.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>

/* --- Constants --- */

#define CHECK(name) \
    _Static_assert((name) == UAPI_##name, #name " differs from the UAPI value");
#define CHECK_AS(lc, uapi) \
    _Static_assert((lc) == UAPI_AS_##lc, #lc " differs from the UAPI " #uapi);
#define CHECK_FUNCTION(name, probe)                                         \
    _Static_assert((name(probe) & 0xffff) == UAPI_LOW_##name &&             \
                   (name(probe) >> 16) == UAPI_HIGH_##name,                 \
                   #name "(" #probe ") differs from the UAPI value");
UAPI_CONSTANTS(CHECK)
#if defined(__x86_64__)
UAPI_CONSTANTS_X86_64(CHECK)
#endif
UAPI_RENAMED(CHECK_AS)
UAPI_FUNCTIONS(CHECK_FUNCTION)

/* --- Syscall numbers --- */

/*
 * The syscalls of syscalls.def: SYSCALLS lists those both arches have, and
 * SYSCALLS_<ARCH>_ONLY those only one has. For a syscall syscalls.def gives
 * no number on an arch, an #error checks that the arch has no __NR_ for it.
 */
/* BEGIN GENERATED abi-syscalls: from syscalls.def by tools/gensyscalls.sh */
#define SYSCALLS(X)                                                         \
    X(read) X(write) X(close) X(lseek) X(mmap) X(mprotect) X(munmap) X(brk) \
    X(ioctl) X(writev) X(openat) X(getpid) X(madvise) X(clone) X(exit)      \
    X(gettid) X(futex) X(exit_group) X(getdents64) X(socket) X(connect)     \
    X(sendto) X(recvfrom) X(shutdown) X(bind) X(listen) X(getsockname)      \
    X(setsockopt) X(accept4) X(clock_gettime) X(nanosleep) X(setitimer)     \
    X(execve) X(wait4) X(pipe2) X(dup3) X(setsid) X(rt_sigaction)           \
    X(rt_sigprocmask) X(kill) X(signalfd4) X(mount) X(umount2)              \
    X(pivot_root) X(chdir) X(fstat) X(statx) X(mknodat) X(mkdirat)          \
    X(symlinkat) X(unlinkat) X(readlinkat) X(fchownat) X(fchmodat)          \
    X(fchmodat2) X(faccessat2) X(unshare) X(setns) X(sethostname) X(prctl)  \
    X(seccomp) X(getuid) X(setuid) X(setgid) X(setgroups) X(io_uring_setup) \
    X(io_uring_enter) X(io_uring_register)
#define SYSCALLS_X86_64_ONLY(X)                                             \
    X(open) X(arch_prctl)
#define SYSCALLS_AARCH64_ONLY(X)
#if defined(__aarch64__) && defined(__NR_open)
#error "syscalls.def gives open no aarch64 number, but aarch64 has __NR_open"
#endif
#if defined(__aarch64__) && defined(__NR_arch_prctl)
#error "syscalls.def gives arch_prctl no aarch64 number, but aarch64 has __NR_arch_prctl"
#endif
/* END GENERATED abi-syscalls */

#define CHECK_NR(name) \
    _Static_assert(SYS_##name == __NR_##name, "SYS_" #name " differs from __NR_" #name);
SYSCALLS(CHECK_NR)

#if defined(__x86_64__)
SYSCALLS_X86_64_ONLY(CHECK_NR)
#elif defined(__aarch64__)
SYSCALLS_AARCH64_ONLY(CHECK_NR)
#endif

/* --- Errno values --- */

/* The errnos of syscalls.def, without their leading E. */
/* BEGIN GENERATED abi-errnos: from syscalls.def by tools/gensyscalls.sh */
#define ERRNOS(X)                                                           \
    X(PERM) X(NOENT) X(SRCH) X(INTR) X(IO) X(NXIO) X(2BIG) X(NOEXEC)        \
    X(BADF) X(CHILD) X(AGAIN) X(NOMEM) X(ACCES) X(FAULT) X(NOTBLK) X(BUSY)  \
    X(EXIST) X(XDEV) X(NODEV) X(NOTDIR) X(ISDIR) X(INVAL) X(NFILE) X(MFILE) \
    X(NOTTY) X(TXTBSY) X(FBIG) X(NOSPC) X(SPIPE) X(ROFS) X(MLINK) X(PIPE)   \
    X(DOM) X(RANGE) X(DEADLK) X(NAMETOOLONG) X(NOLCK) X(NOSYS) X(NOTEMPTY)  \
    X(LOOP) X(WOULDBLOCK) X(NOMSG) X(IDRM) X(CHRNG) X(L2NSYNC) X(L3HLT)     \
    X(L3RST) X(LNRNG) X(UNATCH) X(NOCSI) X(L2HLT) X(BADE) X(BADR) X(XFULL)  \
    X(NOANO) X(BADRQC) X(BADSLT) X(DEADLOCK) X(BFONT) X(NOSTR) X(NODATA)    \
    X(TIME) X(NOSR) X(NONET) X(NOPKG) X(REMOTE) X(NOLINK) X(ADV) X(SRMNT)   \
    X(COMM) X(PROTO) X(MULTIHOP) X(DOTDOT) X(BADMSG) X(OVERFLOW) X(NOTUNIQ) \
    X(BADFD) X(REMCHG) X(LIBACC) X(LIBBAD) X(LIBSCN) X(LIBMAX) X(LIBEXEC)   \
    X(ILSEQ) X(RESTART) X(STRPIPE) X(USERS) X(NOTSOCK) X(DESTADDRREQ)       \
    X(MSGSIZE) X(PROTOTYPE) X(NOPROTOOPT) X(PROTONOSUPPORT)                 \
    X(SOCKTNOSUPPORT) X(OPNOTSUPP) X(PFNOSUPPORT) X(AFNOSUPPORT)            \
    X(ADDRINUSE) X(ADDRNOTAVAIL) X(NETDOWN) X(NETUNREACH) X(NETRESET)       \
    X(CONNABORTED) X(CONNRESET) X(NOBUFS) X(ISCONN) X(NOTCONN) X(SHUTDOWN)  \
    X(TOOMANYREFS) X(TIMEDOUT) X(CONNREFUSED) X(HOSTDOWN) X(HOSTUNREACH)    \
    X(ALREADY) X(INPROGRESS) X(STALE) X(UCLEAN) X(NOTNAM) X(NAVAIL)         \
    X(ISNAM) X(REMOTEIO) X(DQUOT) X(NOMEDIUM) X(MEDIUMTYPE) X(CANCELED)     \
    X(NOKEY) X(KEYEXPIRED) X(KEYREVOKED) X(KEYREJECTED) X(OWNERDEAD)        \
    X(NOTRECOVERABLE) X(RFKILL) X(HWPOISON)
/* END GENERATED abi-errnos */

#define CHECK_ERRNO(name) \
    _Static_assert(LC_ERR_##name == E##name, "LC_ERR_" #name " differs from E" #name);
ERRNOS(CHECK_ERRNO)

/* --- Structs the kernel reads or writes --- */

/* lightc's type t has the size of the kernel's type k. */
#define SAME_SIZE(t, k)                                                     \
    _Static_assert(sizeof(t) == sizeof(k),                                  \
                   "sizeof(" #t ") differs from sizeof(" #k ")");

/* Field f of t is at the offset of field kf of k, and as large. */
#define SAME_FIELD(t, f, k, kf)                                             \
    SAME_OFFSET(t, f, k, kf)                                                \
    _Static_assert(sizeof(((t *)0)->f) == sizeof(((k *)0)->kf),             \
                   #t "." #f " differs in size from " #k "." #kf);

/* Field f of t starts where field kf of k does: for padding. */
#define SAME_OFFSET(t, f, k, kf)                                            \
    _Static_assert(offsetof(t, f) == offsetof(k, kf),                       \
                   #t "." #f " is not at the offset of " #k "." #kf);

/* clock_gettime and nanosleep take a struct __kernel_timespec. */
SAME_SIZE(lc_timespec, struct __kernel_timespec)
SAME_FIELD(lc_timespec, seconds, struct __kernel_timespec, tv_sec)
SAME_FIELD(lc_timespec, nanoseconds, struct __kernel_timespec, tv_nsec)

/* rt_sigaction copies the whole struct in and out. */
SAME_SIZE(lc_kernel_sigaction, struct sigaction)
SAME_FIELD(lc_kernel_sigaction, handler, struct sigaction, sa_handler)
SAME_FIELD(lc_kernel_sigaction, flags, struct sigaction, sa_flags)
SAME_FIELD(lc_kernel_sigaction, restorer, struct sigaction, sa_restorer)
SAME_FIELD(lc_kernel_sigaction, mask, struct sigaction, sa_mask)

/* A read from a signalfd gives one struct signalfd_siginfo per signal. */
SAME_SIZE(lc_signal_info, struct signalfd_siginfo)
SAME_FIELD(lc_signal_info, signal, struct signalfd_siginfo, ssi_signo)
SAME_FIELD(lc_signal_info, error, struct signalfd_siginfo, ssi_errno)
SAME_FIELD(lc_signal_info, code, struct signalfd_siginfo, ssi_code)
SAME_FIELD(lc_signal_info, pid, struct signalfd_siginfo, ssi_pid)
SAME_FIELD(lc_signal_info, uid, struct signalfd_siginfo, ssi_uid)
SAME_FIELD(lc_signal_info, fd, struct signalfd_siginfo, ssi_fd)
SAME_FIELD(lc_signal_info, tid, struct signalfd_siginfo, ssi_tid)
SAME_FIELD(lc_signal_info, band, struct signalfd_siginfo, ssi_band)
SAME_FIELD(lc_signal_info, overrun, struct signalfd_siginfo, ssi_overrun)
SAME_FIELD(lc_signal_info, trapno, struct signalfd_siginfo, ssi_trapno)
SAME_FIELD(lc_signal_info, status, struct signalfd_siginfo, ssi_status)
SAME_FIELD(lc_signal_info, int_val, struct signalfd_siginfo, ssi_int)
SAME_FIELD(lc_signal_info, ptr_val, struct signalfd_siginfo, ssi_ptr)
SAME_FIELD(lc_signal_info, utime, struct signalfd_siginfo, ssi_utime)
SAME_FIELD(lc_signal_info, stime, struct signalfd_siginfo, ssi_stime)
SAME_FIELD(lc_signal_info, addr, struct signalfd_siginfo, ssi_addr)
SAME_OFFSET(lc_signal_info, _pad, struct signalfd_siginfo, ssi_addr_lsb)

/* bind, connect, accept4, sendto and recvfrom take the socket addresses. */
SAME_SIZE(lc_socket_address, struct sockaddr_in)
SAME_FIELD(lc_socket_address, family, struct sockaddr_in, sin_family)
SAME_FIELD(lc_socket_address, port, struct sockaddr_in, sin_port)
SAME_FIELD(lc_socket_address, addr, struct sockaddr_in, sin_addr)
SAME_FIELD(lc_socket_address, _pad, struct sockaddr_in, __pad)

SAME_SIZE(lc_socket_address6, struct sockaddr_in6)
SAME_FIELD(lc_socket_address6, family, struct sockaddr_in6, sin6_family)
SAME_FIELD(lc_socket_address6, port, struct sockaddr_in6, sin6_port)
SAME_FIELD(lc_socket_address6, flowinfo, struct sockaddr_in6, sin6_flowinfo)
SAME_FIELD(lc_socket_address6, addr, struct sockaddr_in6, sin6_addr)
SAME_FIELD(lc_socket_address6, scope_id, struct sockaddr_in6, sin6_scope_id)

/*
 * src/kernel.h: io_uring. The kernel's sqe ends in a union whose first
 * member is addr3, so lightc's _pad starts there. user_addr, Linux 6.5's
 * name for the last field of the ring offsets, is lightc's resv2.
 */
SAME_SIZE(io_uring_sqe, struct io_uring_sqe)
SAME_FIELD(io_uring_sqe, opcode, struct io_uring_sqe, opcode)
SAME_FIELD(io_uring_sqe, flags, struct io_uring_sqe, flags)
SAME_FIELD(io_uring_sqe, ioprio, struct io_uring_sqe, ioprio)
SAME_FIELD(io_uring_sqe, fd, struct io_uring_sqe, fd)
SAME_FIELD(io_uring_sqe, off, struct io_uring_sqe, off)
SAME_FIELD(io_uring_sqe, addr, struct io_uring_sqe, addr)
SAME_FIELD(io_uring_sqe, len, struct io_uring_sqe, len)
SAME_FIELD(io_uring_sqe, rw_flags, struct io_uring_sqe, rw_flags)
SAME_FIELD(io_uring_sqe, user_data, struct io_uring_sqe, user_data)
SAME_FIELD(io_uring_sqe, buf_index, struct io_uring_sqe, buf_index)
SAME_FIELD(io_uring_sqe, personality, struct io_uring_sqe, personality)
SAME_FIELD(io_uring_sqe, splice_fd_in, struct io_uring_sqe, splice_fd_in)
SAME_OFFSET(io_uring_sqe, _pad, struct io_uring_sqe, addr3)

SAME_SIZE(io_uring_cqe, struct io_uring_cqe)
SAME_FIELD(io_uring_cqe, user_data, struct io_uring_cqe, user_data)
SAME_FIELD(io_uring_cqe, res, struct io_uring_cqe, res)
SAME_FIELD(io_uring_cqe, flags, struct io_uring_cqe, flags)

SAME_SIZE(io_sqring_offsets, struct io_sqring_offsets)
SAME_FIELD(io_sqring_offsets, head, struct io_sqring_offsets, head)
SAME_FIELD(io_sqring_offsets, tail, struct io_sqring_offsets, tail)
SAME_FIELD(io_sqring_offsets, ring_mask, struct io_sqring_offsets, ring_mask)
SAME_FIELD(io_sqring_offsets, ring_entries, struct io_sqring_offsets, ring_entries)
SAME_FIELD(io_sqring_offsets, flags, struct io_sqring_offsets, flags)
SAME_FIELD(io_sqring_offsets, dropped, struct io_sqring_offsets, dropped)
SAME_FIELD(io_sqring_offsets, array, struct io_sqring_offsets, array)
SAME_FIELD(io_sqring_offsets, resv1, struct io_sqring_offsets, resv1)
SAME_FIELD(io_sqring_offsets, resv2, struct io_sqring_offsets, user_addr)

SAME_SIZE(io_cqring_offsets, struct io_cqring_offsets)
SAME_FIELD(io_cqring_offsets, head, struct io_cqring_offsets, head)
SAME_FIELD(io_cqring_offsets, tail, struct io_cqring_offsets, tail)
SAME_FIELD(io_cqring_offsets, ring_mask, struct io_cqring_offsets, ring_mask)
SAME_FIELD(io_cqring_offsets, ring_entries, struct io_cqring_offsets, ring_entries)
SAME_FIELD(io_cqring_offsets, overflow, struct io_cqring_offsets, overflow)
SAME_FIELD(io_cqring_offsets, cqes, struct io_cqring_offsets, cqes)
SAME_FIELD(io_cqring_offsets, flags, struct io_cqring_offsets, flags)
SAME_FIELD(io_cqring_offsets, resv1, struct io_cqring_offsets, resv1)
SAME_FIELD(io_cqring_offsets, resv2, struct io_cqring_offsets, user_addr)

SAME_SIZE(io_uring_params, struct io_uring_params)
SAME_FIELD(io_uring_params, sq_entries, struct io_uring_params, sq_entries)
SAME_FIELD(io_uring_params, cq_entries, struct io_uring_params, cq_entries)
SAME_FIELD(io_uring_params, flags, struct io_uring_params, flags)
SAME_FIELD(io_uring_params, sq_thread_cpu, struct io_uring_params, sq_thread_cpu)
SAME_FIELD(io_uring_params, sq_thread_idle, struct io_uring_params, sq_thread_idle)
SAME_FIELD(io_uring_params, features, struct io_uring_params, features)
SAME_FIELD(io_uring_params, wq_fd, struct io_uring_params, wq_fd)
SAME_FIELD(io_uring_params, resv, struct io_uring_params, resv)
SAME_FIELD(io_uring_params, sq_off, struct io_uring_params, sq_off)
SAME_FIELD(io_uring_params, cq_off, struct io_uring_params, cq_off)

/* IORING_OP_TIMEOUT reads a struct __kernel_timespec. */
SAME_SIZE(lio_kernel_timespec, struct __kernel_timespec)
SAME_FIELD(lio_kernel_timespec, tv_sec, struct __kernel_timespec, tv_sec)
SAME_FIELD(lio_kernel_timespec, tv_nsec, struct __kernel_timespec, tv_nsec)

/*
 * src/kernel.h: getdents64's records. No UAPI header defines struct
 * linux_dirent64 (fs/readdir.c does, and getdents64(2) documents it), so
 * this checks it against the offsets and sizes the kernel's has on every
 * arch.
 */
#define AT_OFFSET(t, f, offset, size)                                       \
    _Static_assert(offsetof(t, f) == (offset),                              \
                   #t "." #f " is not at offset " #offset);                 \
    _Static_assert(sizeof(((t *)0)->f) == (size),                           \
                   #t "." #f " is not " #size " bytes");
AT_OFFSET(struct linux_dirent64, d_ino, 0, 8)
AT_OFFSET(struct linux_dirent64, d_off, 8, 8)
AT_OFFSET(struct linux_dirent64, d_reclen, 16, 2)
AT_OFFSET(struct linux_dirent64, d_type, 18, 1)
_Static_assert(offsetof(struct linux_dirent64, d_name) == 19,
               "struct linux_dirent64.d_name is not at offset 19");

/* src/kernel.h: ELF, as lc_library_open reads it from a shared object. */
SAME_SIZE(lightc_Elf64_Addr, Elf64_Addr)
SAME_SIZE(lightc_Elf64_Off, Elf64_Off)
SAME_SIZE(lightc_Elf64_Half, Elf64_Half)
SAME_SIZE(lightc_Elf64_Word, Elf64_Word)
SAME_SIZE(lightc_Elf64_Sword, Elf64_Sword)
SAME_SIZE(lightc_Elf64_Xword, Elf64_Xword)
SAME_SIZE(lightc_Elf64_Sxword, Elf64_Sxword)

SAME_SIZE(lightc_Elf64_Ehdr, Elf64_Ehdr)
SAME_FIELD(lightc_Elf64_Ehdr, e_ident, Elf64_Ehdr, e_ident)
SAME_FIELD(lightc_Elf64_Ehdr, e_type, Elf64_Ehdr, e_type)
SAME_FIELD(lightc_Elf64_Ehdr, e_machine, Elf64_Ehdr, e_machine)
SAME_FIELD(lightc_Elf64_Ehdr, e_version, Elf64_Ehdr, e_version)
SAME_FIELD(lightc_Elf64_Ehdr, e_entry, Elf64_Ehdr, e_entry)
SAME_FIELD(lightc_Elf64_Ehdr, e_phoff, Elf64_Ehdr, e_phoff)
SAME_FIELD(lightc_Elf64_Ehdr, e_shoff, Elf64_Ehdr, e_shoff)
SAME_FIELD(lightc_Elf64_Ehdr, e_flags, Elf64_Ehdr, e_flags)
SAME_FIELD(lightc_Elf64_Ehdr, e_ehsize, Elf64_Ehdr, e_ehsize)
SAME_FIELD(lightc_Elf64_Ehdr, e_phentsize, Elf64_Ehdr, e_phentsize)
SAME_FIELD(lightc_Elf64_Ehdr, e_phnum, Elf64_Ehdr, e_phnum)
SAME_FIELD(lightc_Elf64_Ehdr, e_shentsize, Elf64_Ehdr, e_shentsize)
SAME_FIELD(lightc_Elf64_Ehdr, e_shnum, Elf64_Ehdr, e_shnum)
SAME_FIELD(lightc_Elf64_Ehdr, e_shstrndx, Elf64_Ehdr, e_shstrndx)

SAME_SIZE(lightc_Elf64_Phdr, Elf64_Phdr)
SAME_FIELD(lightc_Elf64_Phdr, p_type, Elf64_Phdr, p_type)
SAME_FIELD(lightc_Elf64_Phdr, p_flags, Elf64_Phdr, p_flags)
SAME_FIELD(lightc_Elf64_Phdr, p_offset, Elf64_Phdr, p_offset)
SAME_FIELD(lightc_Elf64_Phdr, p_vaddr, Elf64_Phdr, p_vaddr)
SAME_FIELD(lightc_Elf64_Phdr, p_paddr, Elf64_Phdr, p_paddr)
SAME_FIELD(lightc_Elf64_Phdr, p_filesz, Elf64_Phdr, p_filesz)
SAME_FIELD(lightc_Elf64_Phdr, p_memsz, Elf64_Phdr, p_memsz)
SAME_FIELD(lightc_Elf64_Phdr, p_align, Elf64_Phdr, p_align)

SAME_SIZE(lightc_Elf64_Dyn, Elf64_Dyn)
SAME_FIELD(lightc_Elf64_Dyn, d_tag, Elf64_Dyn, d_tag)
SAME_FIELD(lightc_Elf64_Dyn, d_un, Elf64_Dyn, d_un)
SAME_FIELD(lightc_Elf64_Dyn, d_un.d_val, Elf64_Dyn, d_un.d_val)
SAME_FIELD(lightc_Elf64_Dyn, d_un.d_ptr, Elf64_Dyn, d_un.d_ptr)

SAME_SIZE(lightc_Elf64_Sym, Elf64_Sym)
SAME_FIELD(lightc_Elf64_Sym, st_name, Elf64_Sym, st_name)
SAME_FIELD(lightc_Elf64_Sym, st_info, Elf64_Sym, st_info)
SAME_FIELD(lightc_Elf64_Sym, st_other, Elf64_Sym, st_other)
SAME_FIELD(lightc_Elf64_Sym, st_shndx, Elf64_Sym, st_shndx)
SAME_FIELD(lightc_Elf64_Sym, st_value, Elf64_Sym, st_value)
SAME_FIELD(lightc_Elf64_Sym, st_size, Elf64_Sym, st_size)

SAME_SIZE(lightc_Elf64_Rela, Elf64_Rela)
SAME_FIELD(lightc_Elf64_Rela, r_offset, Elf64_Rela, r_offset)
SAME_FIELD(lightc_Elf64_Rela, r_info, Elf64_Rela, r_info)
SAME_FIELD(lightc_Elf64_Rela, r_addend, Elf64_Rela, r_addend)
