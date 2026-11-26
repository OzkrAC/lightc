#ifndef LIGHTC_SRC_KERNEL_H
#define LIGHTC_SRC_KERNEL_H

/*
 * kernel.h — the kernel ABI that src/ uses and the public headers do not
 * define: the structs the kernel reads or writes, and the constants that go
 * with them. Internal: it is not installed.
 *
 * lightc builds without kernel headers, so each definition here is a copy
 * of the kernel's, from the Linux UAPI headers where they have it, and
 * named after it where that name is free. struct linux_dirent64, the
 * socket flags and some ELF values are in no UAPI header: tests/abi_check.c
 * says where each of them comes from. abi_check.c includes this file and
 * checks it against the UAPI headers of each arch: the size of each struct
 * and the offset and size of each field, and the value of each macro, or it
 * lists the macro with the reason no UAPI header can check it.
 * ci/abilint.sh fails when a macro here is in none of its lists. So a
 * kernel value that a file in src/ needs goes here or in a public header,
 * not in the .c file.
 */

#include <lightc/types.h>

/* --- io_uring (<linux/io_uring.h>), for async.c and lightio.c --- */

/* Submission Queue Entry — 64 bytes */
typedef struct {
    uint8_t  opcode;
    uint8_t  flags;
    uint16_t ioprio;
    int32_t  fd;
    uint64_t off;
    uint64_t addr;
    uint32_t len;
    uint32_t rw_flags;
    uint64_t user_data;
    uint16_t buf_index;
    uint16_t personality;
    int32_t  splice_fd_in;
    uint64_t _pad[2];
} io_uring_sqe;
_Static_assert(sizeof(io_uring_sqe) == 64, "SQE must be 64 bytes");

/* Completion Queue Entry — 16 bytes */
typedef struct {
    uint64_t user_data;
    int32_t  res;
    uint32_t flags;
} io_uring_cqe;
_Static_assert(sizeof(io_uring_cqe) == 16, "CQE must be 16 bytes");

/* Offsets into the mmap'd SQ ring */
typedef struct {
    uint32_t head, tail, ring_mask, ring_entries;
    uint32_t flags, dropped, array, resv1;
    uint64_t resv2;
} io_sqring_offsets;

/* Offsets into the mmap'd CQ ring */
typedef struct {
    uint32_t head, tail, ring_mask, ring_entries;
    uint32_t overflow, cqes, flags, resv1;
    uint64_t resv2;
} io_cqring_offsets;

/* Parameters for io_uring_setup */
typedef struct {
    uint32_t sq_entries, cq_entries, flags;
    uint32_t sq_thread_cpu, sq_thread_idle;
    uint32_t features, wq_fd;
    uint32_t resv[3];
    io_sqring_offsets sq_off;
    io_cqring_offsets cq_off;
} io_uring_params;

/* Kernel timespec layout for IORING_OP_TIMEOUT (struct __kernel_timespec) */
typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} lio_kernel_timespec;

#define IORING_OFF_SQ_RING  0ULL
#define IORING_OFF_CQ_RING  0x8000000ULL
#define IORING_OFF_SQES     0x10000000ULL

#define IORING_OP_TIMEOUT  11
#define IORING_OP_ACCEPT   13
#define IORING_OP_READ     22
#define IORING_OP_WRITE    23

#define IORING_ENTER_GETEVENTS  (1U << 0)

/* io_uring_params.features */
#define IORING_FEAT_SINGLE_MMAP  (1U << 0)
#define IORING_FEAT_NODROP       (1U << 1)

/* The SQ ring's flags: the kernel holds completions the CQ had no room for */
#define IORING_SQ_CQ_OVERFLOW    (1U << 1)

/* mmap flag for the rings: <lightc/syscall.h> has the others */
#define MAP_POPULATE  0x008000

/* --- getdents64, for io.c --- */

/*
 * The kernel's getdents64 returns entries in this format. No UAPI header
 * defines it: fs/readdir.c does, and getdents64(2) documents it.
 */
struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[];
};

/* --- statx (<linux/stat.h>), for io.c --- */

/*
 * statx fills all 256 bytes of struct statx. lc_file_get_size reads only
 * stx_size, so the fields after it are one array here.
 */
struct statx {
    uint32_t stx_mask;
    uint32_t stx_blksize;
    uint64_t stx_attributes;
    uint32_t stx_nlink;
    uint32_t stx_uid;
    uint32_t stx_gid;
    uint16_t stx_mode;
    uint16_t _spare0;
    uint64_t stx_ino;
    uint64_t stx_size;
    uint64_t _rest[26];     /* stx_blocks to the end */
};
_Static_assert(sizeof(struct statx) == 256, "struct statx must be 256 bytes");

#define STATX_SIZE  0x00000200U

/* --- writev (<linux/uio.h>), for log.c --- */

/* One buffer of a writev. The kernel's iov_base is a void *; writev only
 * reads the buffer, so it is const here. */
struct iovec {
    const void *iov_base;
    size_t      iov_len;
};

/* --- Sockets, for socket.c --- */

/* socket and accept4 flag: the kernel's include/linux/net.h defines it as
 * O_CLOEXEC */
#define SOCK_CLOEXEC  02000000

/* send flag: fail with EPIPE instead of raising SIGPIPE (the kernel's
 * include/linux/socket.h) */
#define MSG_NOSIGNAL  0x4000

/* --- clone (<linux/sched.h>), for thread.c, which says what each is for --- */

#define CLONE_VM              0x00000100
#define CLONE_FS              0x00000200
#define CLONE_FILES           0x00000400
#define CLONE_SIGHAND         0x00000800
#define CLONE_THREAD          0x00010000
#define CLONE_SYSVSEM         0x00040000
#define CLONE_PARENT_SETTID   0x00100000
#define CLONE_CHILD_CLEARTID  0x00200000

#define CLONE_SETTLS          0x00080000

/* --- arch_prctl (<asm/prctl.h>, x86_64 only), for heap.c --- */

#if defined(__x86_64__)
#define ARCH_SET_FS 0x1002
#endif

/* --- The auxiliary vector (<linux/auxvec.h>), for memory.c --- */

#define LC_AT_NULL    0
#define LC_AT_PAGESZ  6

/* --- Signals (<asm/signal.h>), for signal.c --- */

/* SA_RESTORER flag — required on x86_64, not used on aarch64 */
#define SA_RESTORER 0x04000000

/* --- ELF64 (<linux/elf.h>), for library.c --- */

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

/* ELF header */
typedef struct {
    uint8_t    e_ident[16];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off  e_phoff;
    Elf64_Off  e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/* Program header */
typedef struct {
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

/* Dynamic section entry */
typedef struct {
    Elf64_Sxword d_tag;
    union {
        Elf64_Xword d_val;
        Elf64_Addr  d_ptr;
    } d_un;
} Elf64_Dyn;

/* Symbol table entry */
typedef struct {
    Elf64_Word  st_name;
    uint8_t     st_info;
    uint8_t     st_other;
    Elf64_Half  st_shndx;
    Elf64_Addr  st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

/* Relocation entry with addend */
typedef struct {
    Elf64_Addr    r_offset;
    Elf64_Xword   r_info;
    Elf64_Sxword  r_addend;
} Elf64_Rela;

/* ELF identification */
#define ELFMAG0    0x7f
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_DYN     3

/* Machine types */
#define EM_X86_64  62
#define EM_AARCH64 183

/* Program header types */
#define PT_NULL      0
#define PT_LOAD      1
#define PT_DYNAMIC   2
#define PT_TLS       7
#define PT_GNU_RELRO 0x6474e552

/* Program header flags */
#define PF_X 1
#define PF_W 2
#define PF_R 4

/* Dynamic tags */
#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_INIT         12
#define DT_FINI         13
#define DT_PLTREL       20
#define DT_JMPREL       23
#define DT_INIT_ARRAY   25
#define DT_FINI_ARRAY   26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_RELRSZ       35
#define DT_RELR         36
#define DT_RELRENT      37
#define DT_GNU_HASH     0x6ffffef5
#define DT_RELACOUNT    0x6ffffff9
#define DT_FLAGS        30
#define DT_VERSYM       0x6ffffff0

/* DT_FLAGS: the object uses the static TLS model */
#define DF_STATIC_TLS   0x10

/* Symbol binding/type extraction */
#define ELF64_ST_BIND(i)  ((i) >> 4)
#define ELF64_ST_TYPE(i)  ((i) & 0xf)
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STT_FUNC   2
#define STT_OBJECT 1
#define STT_TLS    6
#define STT_GNU_IFUNC 10
#define SHN_UNDEF  0
#define SHN_ABS    0xfff1

/* Relocation types (x86_64) */
#define R_X86_64_NONE      0
#define R_X86_64_64        1
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE  8
#define R_X86_64_DTPMOD64  16
#define R_X86_64_DTPOFF64  17
#define R_X86_64_TPOFF64   18
#define R_X86_64_TLSDESC   36
#define R_X86_64_IRELATIVE 37

/* Relocation types (aarch64) */
#define R_AARCH64_NONE      0
#define R_AARCH64_RELATIVE  1027
#define R_AARCH64_GLOB_DAT  1025
#define R_AARCH64_JUMP_SLOT 1026
#define R_AARCH64_ABS64     257
#define R_AARCH64_TLS_DTPMOD 1028
#define R_AARCH64_TLS_DTPREL 1029
#define R_AARCH64_TLS_TPREL  1030
#define R_AARCH64_TLSDESC    1031
#define R_AARCH64_IRELATIVE  1032

/* Relocation info extraction */
#define ELF64_R_SYM(i)   ((i) >> 32)
#define ELF64_R_TYPE(i)  ((i) & 0xffffffff)

#endif /* LIGHTC_SRC_KERNEL_H */
