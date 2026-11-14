#ifndef LIGHTC_RESULT_H
#define LIGHTC_RESULT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Result types for error handling.
 *
 * Both structs are exactly 16 bytes, returned in two registers on
 * x86_64 (rax+rdx) and aarch64 (x0+x1).  Zero overhead versus
 * returning a single value.
 *
 * Convention: error == 0 means success, error > 0 is a Linux errno.
 * Library-specific error codes start at 4096.
 */

typedef struct {
    int64_t value;
    int32_t error;
} lc_result;

typedef struct {
    void   *value;
    int32_t error;
} lc_result_ptr;

/* --- Error codes --- */

/*
 * LC_ERR_<name> is Linux's E<name>, with its value, for every errno of
 * asm-generic/errno-base.h and asm-generic/errno.h, which x86_64 and aarch64
 * use unchanged. As in the kernel, LC_ERR_WOULDBLOCK is LC_ERR_AGAIN and
 * LC_ERR_DEADLOCK is LC_ERR_DEADLK.
 *
 * The errno lines here and in lc_error_name are generated from syscalls.def:
 * edit that file, not them, then run sh tools/gensyscalls.sh.
 */
enum {
    LC_OK                 = 0,

    /* BEGIN GENERATED errno-enum: from syscalls.def by tools/gensyscalls.sh */
    LC_ERR_PERM           = 1,   /* EPERM: operation not permitted */
    LC_ERR_NOENT          = 2,   /* ENOENT: no such file or directory */
    LC_ERR_SRCH           = 3,   /* ESRCH: no such process */
    LC_ERR_INTR           = 4,   /* EINTR: interrupted system call */
    LC_ERR_IO             = 5,   /* EIO: I/O error */
    LC_ERR_NXIO           = 6,   /* ENXIO: no such device or address */
    LC_ERR_2BIG           = 7,   /* E2BIG: argument list too long */
    LC_ERR_NOEXEC         = 8,   /* ENOEXEC: exec format error */
    LC_ERR_BADF           = 9,   /* EBADF: bad file number */
    LC_ERR_CHILD          = 10,  /* ECHILD: no child processes */
    LC_ERR_AGAIN          = 11,  /* EAGAIN: try again */
    LC_ERR_NOMEM          = 12,  /* ENOMEM: out of memory */
    LC_ERR_ACCES          = 13,  /* EACCES: permission denied */
    LC_ERR_FAULT          = 14,  /* EFAULT: bad address */
    LC_ERR_NOTBLK         = 15,  /* ENOTBLK: block device required */
    LC_ERR_BUSY           = 16,  /* EBUSY: device or resource busy */
    LC_ERR_EXIST          = 17,  /* EEXIST: file exists */
    LC_ERR_XDEV           = 18,  /* EXDEV: cross-device link */
    LC_ERR_NODEV          = 19,  /* ENODEV: no such device */
    LC_ERR_NOTDIR         = 20,  /* ENOTDIR: not a directory */
    LC_ERR_ISDIR          = 21,  /* EISDIR: is a directory */
    LC_ERR_INVAL          = 22,  /* EINVAL: invalid argument */
    LC_ERR_NFILE          = 23,  /* ENFILE: file table overflow */
    LC_ERR_MFILE          = 24,  /* EMFILE: too many open files */
    LC_ERR_NOTTY          = 25,  /* ENOTTY: not a typewriter */
    LC_ERR_TXTBSY         = 26,  /* ETXTBSY: text file busy */
    LC_ERR_FBIG           = 27,  /* EFBIG: file too large */
    LC_ERR_NOSPC          = 28,  /* ENOSPC: no space left on device */
    LC_ERR_SPIPE          = 29,  /* ESPIPE: illegal seek */
    LC_ERR_ROFS           = 30,  /* EROFS: read-only file system */
    LC_ERR_MLINK          = 31,  /* EMLINK: too many links */
    LC_ERR_PIPE           = 32,  /* EPIPE: broken pipe */
    LC_ERR_DOM            = 33,  /* EDOM: math argument out of domain of func */
    LC_ERR_RANGE          = 34,  /* ERANGE: math result not representable */
    LC_ERR_DEADLK         = 35,  /* EDEADLK: resource deadlock would occur */
    LC_ERR_NAMETOOLONG    = 36,  /* ENAMETOOLONG: file name too long */
    LC_ERR_NOLCK          = 37,  /* ENOLCK: no record locks available */
    LC_ERR_NOSYS          = 38,  /* ENOSYS: invalid system call number */
    LC_ERR_NOTEMPTY       = 39,  /* ENOTEMPTY: directory not empty */
    LC_ERR_LOOP           = 40,  /* ELOOP: too many symbolic links encountered */
    LC_ERR_WOULDBLOCK     = 11,  /* EWOULDBLOCK: same as EAGAIN */
    LC_ERR_NOMSG          = 42,  /* ENOMSG: no message of desired type */
    LC_ERR_IDRM           = 43,  /* EIDRM: identifier removed */
    LC_ERR_CHRNG          = 44,  /* ECHRNG: channel number out of range */
    LC_ERR_L2NSYNC        = 45,  /* EL2NSYNC: level 2 not synchronized */
    LC_ERR_L3HLT          = 46,  /* EL3HLT: level 3 halted */
    LC_ERR_L3RST          = 47,  /* EL3RST: level 3 reset */
    LC_ERR_LNRNG          = 48,  /* ELNRNG: link number out of range */
    LC_ERR_UNATCH         = 49,  /* EUNATCH: protocol driver not attached */
    LC_ERR_NOCSI          = 50,  /* ENOCSI: no CSI structure available */
    LC_ERR_L2HLT          = 51,  /* EL2HLT: level 2 halted */
    LC_ERR_BADE           = 52,  /* EBADE: invalid exchange */
    LC_ERR_BADR           = 53,  /* EBADR: invalid request descriptor */
    LC_ERR_XFULL          = 54,  /* EXFULL: exchange full */
    LC_ERR_NOANO          = 55,  /* ENOANO: no anode */
    LC_ERR_BADRQC         = 56,  /* EBADRQC: invalid request code */
    LC_ERR_BADSLT         = 57,  /* EBADSLT: invalid slot */
    LC_ERR_DEADLOCK       = 35,  /* EDEADLOCK: same as EDEADLK */
    LC_ERR_BFONT          = 59,  /* EBFONT: bad font file format */
    LC_ERR_NOSTR          = 60,  /* ENOSTR: device not a stream */
    LC_ERR_NODATA         = 61,  /* ENODATA: no data available */
    LC_ERR_TIME           = 62,  /* ETIME: timer expired */
    LC_ERR_NOSR           = 63,  /* ENOSR: out of streams resources */
    LC_ERR_NONET          = 64,  /* ENONET: machine is not on the network */
    LC_ERR_NOPKG          = 65,  /* ENOPKG: package not installed */
    LC_ERR_REMOTE         = 66,  /* EREMOTE: object is remote */
    LC_ERR_NOLINK         = 67,  /* ENOLINK: link has been severed */
    LC_ERR_ADV            = 68,  /* EADV: advertise error */
    LC_ERR_SRMNT          = 69,  /* ESRMNT: srmount error */
    LC_ERR_COMM           = 70,  /* ECOMM: communication error on send */
    LC_ERR_PROTO          = 71,  /* EPROTO: protocol error */
    LC_ERR_MULTIHOP       = 72,  /* EMULTIHOP: multihop attempted */
    LC_ERR_DOTDOT         = 73,  /* EDOTDOT: RFS specific error */
    LC_ERR_BADMSG         = 74,  /* EBADMSG: not a data message */
    LC_ERR_OVERFLOW       = 75,  /* EOVERFLOW: value too large for defined data type */
    LC_ERR_NOTUNIQ        = 76,  /* ENOTUNIQ: name not unique on network */
    LC_ERR_BADFD          = 77,  /* EBADFD: file descriptor in bad state */
    LC_ERR_REMCHG         = 78,  /* EREMCHG: remote address changed */
    LC_ERR_LIBACC         = 79,  /* ELIBACC: can not access a needed shared library */
    LC_ERR_LIBBAD         = 80,  /* ELIBBAD: accessing a corrupted shared library */
    LC_ERR_LIBSCN         = 81,  /* ELIBSCN: .lib section in a.out corrupted */
    LC_ERR_LIBMAX         = 82,  /* ELIBMAX: attempting to link in too many shared libraries */
    LC_ERR_LIBEXEC        = 83,  /* ELIBEXEC: cannot exec a shared library directly */
    LC_ERR_ILSEQ          = 84,  /* EILSEQ: illegal byte sequence */
    LC_ERR_RESTART        = 85,  /* ERESTART: interrupted system call should be restarted */
    LC_ERR_STRPIPE        = 86,  /* ESTRPIPE: streams pipe error */
    LC_ERR_USERS          = 87,  /* EUSERS: too many users */
    LC_ERR_NOTSOCK        = 88,  /* ENOTSOCK: socket operation on non-socket */
    LC_ERR_DESTADDRREQ    = 89,  /* EDESTADDRREQ: destination address required */
    LC_ERR_MSGSIZE        = 90,  /* EMSGSIZE: message too long */
    LC_ERR_PROTOTYPE      = 91,  /* EPROTOTYPE: protocol wrong type for socket */
    LC_ERR_NOPROTOOPT     = 92,  /* ENOPROTOOPT: protocol not available */
    LC_ERR_PROTONOSUPPORT = 93,  /* EPROTONOSUPPORT: protocol not supported */
    LC_ERR_SOCKTNOSUPPORT = 94,  /* ESOCKTNOSUPPORT: socket type not supported */
    LC_ERR_OPNOTSUPP      = 95,  /* EOPNOTSUPP: operation not supported on transport endpoint */
    LC_ERR_PFNOSUPPORT    = 96,  /* EPFNOSUPPORT: protocol family not supported */
    LC_ERR_AFNOSUPPORT    = 97,  /* EAFNOSUPPORT: address family not supported by protocol */
    LC_ERR_ADDRINUSE      = 98,  /* EADDRINUSE: address already in use */
    LC_ERR_ADDRNOTAVAIL   = 99,  /* EADDRNOTAVAIL: cannot assign requested address */
    LC_ERR_NETDOWN        = 100, /* ENETDOWN: network is down */
    LC_ERR_NETUNREACH     = 101, /* ENETUNREACH: network is unreachable */
    LC_ERR_NETRESET       = 102, /* ENETRESET: network dropped connection because of reset */
    LC_ERR_CONNABORTED    = 103, /* ECONNABORTED: software caused connection abort */
    LC_ERR_CONNRESET      = 104, /* ECONNRESET: connection reset by peer */
    LC_ERR_NOBUFS         = 105, /* ENOBUFS: no buffer space available */
    LC_ERR_ISCONN         = 106, /* EISCONN: transport endpoint is already connected */
    LC_ERR_NOTCONN        = 107, /* ENOTCONN: transport endpoint is not connected */
    LC_ERR_SHUTDOWN       = 108, /* ESHUTDOWN: cannot send after transport endpoint shutdown */
    LC_ERR_TOOMANYREFS    = 109, /* ETOOMANYREFS: too many references: cannot splice */
    LC_ERR_TIMEDOUT       = 110, /* ETIMEDOUT: connection timed out */
    LC_ERR_CONNREFUSED    = 111, /* ECONNREFUSED: connection refused */
    LC_ERR_HOSTDOWN       = 112, /* EHOSTDOWN: host is down */
    LC_ERR_HOSTUNREACH    = 113, /* EHOSTUNREACH: no route to host */
    LC_ERR_ALREADY        = 114, /* EALREADY: operation already in progress */
    LC_ERR_INPROGRESS     = 115, /* EINPROGRESS: operation now in progress */
    LC_ERR_STALE          = 116, /* ESTALE: stale file handle */
    LC_ERR_UCLEAN         = 117, /* EUCLEAN: structure needs cleaning */
    LC_ERR_NOTNAM         = 118, /* ENOTNAM: not a XENIX named type file */
    LC_ERR_NAVAIL         = 119, /* ENAVAIL: no XENIX semaphores available */
    LC_ERR_ISNAM          = 120, /* EISNAM: is a named type file */
    LC_ERR_REMOTEIO       = 121, /* EREMOTEIO: remote I/O error */
    LC_ERR_DQUOT          = 122, /* EDQUOT: quota exceeded */
    LC_ERR_NOMEDIUM       = 123, /* ENOMEDIUM: no medium found */
    LC_ERR_MEDIUMTYPE     = 124, /* EMEDIUMTYPE: wrong medium type */
    LC_ERR_CANCELED       = 125, /* ECANCELED: operation canceled */
    LC_ERR_NOKEY          = 126, /* ENOKEY: required key not available */
    LC_ERR_KEYEXPIRED     = 127, /* EKEYEXPIRED: key has expired */
    LC_ERR_KEYREVOKED     = 128, /* EKEYREVOKED: key has been revoked */
    LC_ERR_KEYREJECTED    = 129, /* EKEYREJECTED: key was rejected by service */
    LC_ERR_OWNERDEAD      = 130, /* EOWNERDEAD: owner died */
    LC_ERR_NOTRECOVERABLE = 131, /* ENOTRECOVERABLE: state not recoverable */
    LC_ERR_RFKILL         = 132, /* ERFKILL: operation not possible due to RF-kill */
    LC_ERR_HWPOISON       = 133, /* EHWPOISON: memory page has hardware error */
    /* END GENERATED errno-enum */

    /* Library-specific (4096+) */
    LC_ERR_BAD_ELF_MAGIC  = 4096,
    LC_ERR_NOT_64BIT      = 4097,
    LC_ERR_BAD_MACHINE    = 4098,
    LC_ERR_NO_LOAD_SEG    = 4099,
    LC_ERR_NO_DYNAMIC     = 4100,
    LC_ERR_RELOC_FAILED   = 4101,
    LC_ERR_FULL           = 4102,  /* capacity exceeded (atexit table, etc.) */
    LC_ERR_MALFORMED_ELF  = 4103,  /* structurally invalid / out-of-bounds ELF */
};

/* --- Constructors --- */

[[gnu::const]]
static inline lc_result lc_ok(int64_t value) {
    return (lc_result){ .value = value, .error = LC_OK };
}

[[gnu::const]]
static inline lc_result lc_err(int32_t error) {
    return (lc_result){ .value = 0, .error = error };
}

[[gnu::const]]
static inline lc_result_ptr lc_ok_ptr(void *ptr) {
    return (lc_result_ptr){ .value = ptr, .error = LC_OK };
}

[[gnu::const]]
static inline lc_result_ptr lc_err_ptr(int32_t error) {
    return (lc_result_ptr){ .value = NULL, .error = error };
}

/* Convert from lc_sysret (negative-errno convention) to lc_result. */
[[gnu::const]]
static inline lc_result lc_result_from_sysret(int64_t ret) {
    if (ret < 0) return (lc_result){ .value = 0, .error = (int32_t)(-ret) };
    return (lc_result){ .value = ret, .error = LC_OK };
}

/* Convert from lc_sysret to lc_result_ptr (for mmap-style calls). */
[[gnu::const]]
static inline lc_result_ptr lc_result_ptr_from_sysret(int64_t ret) {
    if (ret < 0) return (lc_result_ptr){ .value = NULL, .error = (int32_t)(-ret) };
    return (lc_result_ptr){ .value = (void *)ret, .error = LC_OK };
}

/* --- Queries --- */

[[gnu::const]]
static inline bool lc_is_ok(lc_result r) { return r.error == 0; }

[[gnu::const]]
static inline bool lc_is_err(lc_result r) { return r.error != 0; }

[[gnu::const]]
static inline bool lc_ptr_is_ok(lc_result_ptr r) { return r.error == 0; }

[[gnu::const]]
static inline bool lc_ptr_is_err(lc_result_ptr r) { return r.error != 0; }

/* --- Error name lookup --- */

/*
 * Returns a short string for the error code: the errno's name, such as
 * "ENOMEM", for an errno, the name after LC_ERR_ for a library code, "OK"
 * for LC_OK and "UNKNOWN" for any other value. A value with two errno names
 * gives the first: "EAGAIN" for 11, "EDEADLK" for 35.
 */
[[gnu::const]]
static inline const char *lc_error_name(int32_t error) {
    switch (error) {
    case LC_OK:                 return "OK";
    /* BEGIN GENERATED errno-names: from syscalls.def by tools/gensyscalls.sh */
    case LC_ERR_PERM:           return "EPERM";
    case LC_ERR_NOENT:          return "ENOENT";
    case LC_ERR_SRCH:           return "ESRCH";
    case LC_ERR_INTR:           return "EINTR";
    case LC_ERR_IO:             return "EIO";
    case LC_ERR_NXIO:           return "ENXIO";
    case LC_ERR_2BIG:           return "E2BIG";
    case LC_ERR_NOEXEC:         return "ENOEXEC";
    case LC_ERR_BADF:           return "EBADF";
    case LC_ERR_CHILD:          return "ECHILD";
    case LC_ERR_AGAIN:          return "EAGAIN";
    case LC_ERR_NOMEM:          return "ENOMEM";
    case LC_ERR_ACCES:          return "EACCES";
    case LC_ERR_FAULT:          return "EFAULT";
    case LC_ERR_NOTBLK:         return "ENOTBLK";
    case LC_ERR_BUSY:           return "EBUSY";
    case LC_ERR_EXIST:          return "EEXIST";
    case LC_ERR_XDEV:           return "EXDEV";
    case LC_ERR_NODEV:          return "ENODEV";
    case LC_ERR_NOTDIR:         return "ENOTDIR";
    case LC_ERR_ISDIR:          return "EISDIR";
    case LC_ERR_INVAL:          return "EINVAL";
    case LC_ERR_NFILE:          return "ENFILE";
    case LC_ERR_MFILE:          return "EMFILE";
    case LC_ERR_NOTTY:          return "ENOTTY";
    case LC_ERR_TXTBSY:         return "ETXTBSY";
    case LC_ERR_FBIG:           return "EFBIG";
    case LC_ERR_NOSPC:          return "ENOSPC";
    case LC_ERR_SPIPE:          return "ESPIPE";
    case LC_ERR_ROFS:           return "EROFS";
    case LC_ERR_MLINK:          return "EMLINK";
    case LC_ERR_PIPE:           return "EPIPE";
    case LC_ERR_DOM:            return "EDOM";
    case LC_ERR_RANGE:          return "ERANGE";
    case LC_ERR_DEADLK:         return "EDEADLK";
    case LC_ERR_NAMETOOLONG:    return "ENAMETOOLONG";
    case LC_ERR_NOLCK:          return "ENOLCK";
    case LC_ERR_NOSYS:          return "ENOSYS";
    case LC_ERR_NOTEMPTY:       return "ENOTEMPTY";
    case LC_ERR_LOOP:           return "ELOOP";
    case LC_ERR_NOMSG:          return "ENOMSG";
    case LC_ERR_IDRM:           return "EIDRM";
    case LC_ERR_CHRNG:          return "ECHRNG";
    case LC_ERR_L2NSYNC:        return "EL2NSYNC";
    case LC_ERR_L3HLT:          return "EL3HLT";
    case LC_ERR_L3RST:          return "EL3RST";
    case LC_ERR_LNRNG:          return "ELNRNG";
    case LC_ERR_UNATCH:         return "EUNATCH";
    case LC_ERR_NOCSI:          return "ENOCSI";
    case LC_ERR_L2HLT:          return "EL2HLT";
    case LC_ERR_BADE:           return "EBADE";
    case LC_ERR_BADR:           return "EBADR";
    case LC_ERR_XFULL:          return "EXFULL";
    case LC_ERR_NOANO:          return "ENOANO";
    case LC_ERR_BADRQC:         return "EBADRQC";
    case LC_ERR_BADSLT:         return "EBADSLT";
    case LC_ERR_BFONT:          return "EBFONT";
    case LC_ERR_NOSTR:          return "ENOSTR";
    case LC_ERR_NODATA:         return "ENODATA";
    case LC_ERR_TIME:           return "ETIME";
    case LC_ERR_NOSR:           return "ENOSR";
    case LC_ERR_NONET:          return "ENONET";
    case LC_ERR_NOPKG:          return "ENOPKG";
    case LC_ERR_REMOTE:         return "EREMOTE";
    case LC_ERR_NOLINK:         return "ENOLINK";
    case LC_ERR_ADV:            return "EADV";
    case LC_ERR_SRMNT:          return "ESRMNT";
    case LC_ERR_COMM:           return "ECOMM";
    case LC_ERR_PROTO:          return "EPROTO";
    case LC_ERR_MULTIHOP:       return "EMULTIHOP";
    case LC_ERR_DOTDOT:         return "EDOTDOT";
    case LC_ERR_BADMSG:         return "EBADMSG";
    case LC_ERR_OVERFLOW:       return "EOVERFLOW";
    case LC_ERR_NOTUNIQ:        return "ENOTUNIQ";
    case LC_ERR_BADFD:          return "EBADFD";
    case LC_ERR_REMCHG:         return "EREMCHG";
    case LC_ERR_LIBACC:         return "ELIBACC";
    case LC_ERR_LIBBAD:         return "ELIBBAD";
    case LC_ERR_LIBSCN:         return "ELIBSCN";
    case LC_ERR_LIBMAX:         return "ELIBMAX";
    case LC_ERR_LIBEXEC:        return "ELIBEXEC";
    case LC_ERR_ILSEQ:          return "EILSEQ";
    case LC_ERR_RESTART:        return "ERESTART";
    case LC_ERR_STRPIPE:        return "ESTRPIPE";
    case LC_ERR_USERS:          return "EUSERS";
    case LC_ERR_NOTSOCK:        return "ENOTSOCK";
    case LC_ERR_DESTADDRREQ:    return "EDESTADDRREQ";
    case LC_ERR_MSGSIZE:        return "EMSGSIZE";
    case LC_ERR_PROTOTYPE:      return "EPROTOTYPE";
    case LC_ERR_NOPROTOOPT:     return "ENOPROTOOPT";
    case LC_ERR_PROTONOSUPPORT: return "EPROTONOSUPPORT";
    case LC_ERR_SOCKTNOSUPPORT: return "ESOCKTNOSUPPORT";
    case LC_ERR_OPNOTSUPP:      return "EOPNOTSUPP";
    case LC_ERR_PFNOSUPPORT:    return "EPFNOSUPPORT";
    case LC_ERR_AFNOSUPPORT:    return "EAFNOSUPPORT";
    case LC_ERR_ADDRINUSE:      return "EADDRINUSE";
    case LC_ERR_ADDRNOTAVAIL:   return "EADDRNOTAVAIL";
    case LC_ERR_NETDOWN:        return "ENETDOWN";
    case LC_ERR_NETUNREACH:     return "ENETUNREACH";
    case LC_ERR_NETRESET:       return "ENETRESET";
    case LC_ERR_CONNABORTED:    return "ECONNABORTED";
    case LC_ERR_CONNRESET:      return "ECONNRESET";
    case LC_ERR_NOBUFS:         return "ENOBUFS";
    case LC_ERR_ISCONN:         return "EISCONN";
    case LC_ERR_NOTCONN:        return "ENOTCONN";
    case LC_ERR_SHUTDOWN:       return "ESHUTDOWN";
    case LC_ERR_TOOMANYREFS:    return "ETOOMANYREFS";
    case LC_ERR_TIMEDOUT:       return "ETIMEDOUT";
    case LC_ERR_CONNREFUSED:    return "ECONNREFUSED";
    case LC_ERR_HOSTDOWN:       return "EHOSTDOWN";
    case LC_ERR_HOSTUNREACH:    return "EHOSTUNREACH";
    case LC_ERR_ALREADY:        return "EALREADY";
    case LC_ERR_INPROGRESS:     return "EINPROGRESS";
    case LC_ERR_STALE:          return "ESTALE";
    case LC_ERR_UCLEAN:         return "EUCLEAN";
    case LC_ERR_NOTNAM:         return "ENOTNAM";
    case LC_ERR_NAVAIL:         return "ENAVAIL";
    case LC_ERR_ISNAM:          return "EISNAM";
    case LC_ERR_REMOTEIO:       return "EREMOTEIO";
    case LC_ERR_DQUOT:          return "EDQUOT";
    case LC_ERR_NOMEDIUM:       return "ENOMEDIUM";
    case LC_ERR_MEDIUMTYPE:     return "EMEDIUMTYPE";
    case LC_ERR_CANCELED:       return "ECANCELED";
    case LC_ERR_NOKEY:          return "ENOKEY";
    case LC_ERR_KEYEXPIRED:     return "EKEYEXPIRED";
    case LC_ERR_KEYREVOKED:     return "EKEYREVOKED";
    case LC_ERR_KEYREJECTED:    return "EKEYREJECTED";
    case LC_ERR_OWNERDEAD:      return "EOWNERDEAD";
    case LC_ERR_NOTRECOVERABLE: return "ENOTRECOVERABLE";
    case LC_ERR_RFKILL:         return "ERFKILL";
    case LC_ERR_HWPOISON:       return "EHWPOISON";
    /* END GENERATED errno-names */
    case LC_ERR_BAD_ELF_MAGIC:  return "BAD_ELF_MAGIC";
    case LC_ERR_NOT_64BIT:      return "NOT_64BIT";
    case LC_ERR_BAD_MACHINE:    return "BAD_MACHINE";
    case LC_ERR_NO_LOAD_SEG:    return "NO_LOAD_SEG";
    case LC_ERR_NO_DYNAMIC:     return "NO_DYNAMIC";
    case LC_ERR_RELOC_FAILED:   return "RELOC_FAILED";
    case LC_ERR_FULL:           return "FULL";
    case LC_ERR_MALFORMED_ELF:  return "MALFORMED_ELF";
    default:                    return "UNKNOWN";
    }
}

#endif /* LIGHTC_RESULT_H */
