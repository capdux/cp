// Source: Absorbed from /data/dev/bun/src/errno/freebsd_errno.cp | Synced: 2026-11-22T16:06:00Z
// ============================================================================
// lib/errno/freebsdErrno.cp — FreeBSD 98-Entry Native Errno Table & Bridge
// ============================================================================
// Pure-CP zero-allocation representation of FreeBSD `<sys/errno.h>` (0..97),
// including Capsicum capability sandbox errors (`ENOTCAPABLE`, `ECAPMODE`),
// integrity errors (`EINTEGRITY`), and bidirectional `SystemErrno` translation.

import { SystemErrno } from "./errno.cp";

pub const FREEBSD_MAX_ERRNO: u16 = 98;

fn eqAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Complete FreeBSD native `errno` enumeration (`0..97`, matching `<sys/errno.h>`).
pub enum FreebsdErrno: u16 {
    SUCCESS = 0,
    EPERM = 1,
    ENOENT = 2,
    ESRCH = 3,
    EINTR = 4,
    EIO = 5,
    ENXIO = 6,
    E2BIG = 7,
    ENOEXEC = 8,
    EBADF = 9,
    ECHILD = 10,
    EDEADLK = 11,
    ENOMEM = 12,
    EACCES = 13,
    EFAULT = 14,
    ENOTBLK = 15,
    EBUSY = 16,
    EEXIST = 17,
    EXDEV = 18,
    ENODEV = 19,
    ENOTDIR = 20,
    EISDIR = 21,
    EINVAL = 22,
    ENFILE = 23,
    EMFILE = 24,
    ENOTTY = 25,
    ETXTBSY = 26,
    EFBIG = 27,
    ENOSPC = 28,
    ESPIPE = 29,
    EROFS = 30,
    EMLINK = 31,
    EPIPE = 32,
    EDOM = 33,
    ERANGE = 34,
    EAGAIN = 35,
    EINPROGRESS = 36,
    EALREADY = 37,
    ENOTSOCK = 38,
    EDESTADDRREQ = 39,
    EMSGSIZE = 40,
    EPROTOTYPE = 41,
    ENOPROTOOPT = 42,
    EPROTONOSUPPORT = 43,
    ESOCKTNOSUPPORT = 44,
    EOPNOTSUPP = 45,
    EPFNOSUPPORT = 46,
    EAFNOSUPPORT = 47,
    EADDRINUSE = 48,
    EADDRNOTAVAIL = 49,
    ENETDOWN = 50,
    ENETUNREACH = 51,
    ENETRESET = 52,
    ECONNABORTED = 53,
    ECONNRESET = 54,
    ENOBUFS = 55,
    EISCONN = 56,
    ENOTCONN = 57,
    ESHUTDOWN = 58,
    ETOOMANYREFS = 59,
    ETIMEDOUT = 60,
    ECONNREFUSED = 61,
    ELOOP = 62,
    ENAMETOOLONG = 63,
    EHOSTDOWN = 64,
    EHOSTUNREACH = 65,
    ENOTEMPTY = 66,
    EPROCLIM = 67,
    EUSERS = 68,
    EDQUOT = 69,
    ESTALE = 70,
    EREMOTE = 71,
    EBADRPC = 72,
    ERPCMISMATCH = 73,
    EPROGUNAVAIL = 74,
    EPROGMISMATCH = 75,
    EPROCUNAVAIL = 76,
    ENOLCK = 77,
    ENOSYS = 78,
    EFTYPE = 79,
    EAUTH = 80,
    ENEEDAUTH = 81,
    EIDRM = 82,
    ENOMSG = 83,
    EOVERFLOW = 84,
    ECANCELED = 85,
    EILSEQ = 86,
    ENOATTR = 87,
    EDOOFUS = 88,
    EBADMSG = 89,
    EMULTIHOP = 90,
    ENOLINK = 91,
    EPROTO = 92,
    ENOTCAPABLE = 93,
    ECAPMODE = 94,
    ENOTRECOVERABLE = 95,
    EOWNERDEAD = 96,
    EINTEGRITY = 97,

    static fn fromCode(code: i32): FreebsdErrno? {
        if (code < 0) {
            if (code <= -98) return null;
            return @enumFromInt(@as(u16, @intCast(-code)));
        }
        if (code >= 98) return null;
        return @enumFromInt(@as(u16, @intCast(code)));
    }

    fn toCode(): i32 {
        return @as(i32, @intFromEnum(_));
    }

    fn isOk(): bool {
        return _ == .SUCCESS;
    }

    fn isRetryable(): bool {
        return _ == .EINTR || _ == .EAGAIN || _ == .EBUSY || _ == .ETIMEDOUT;
    }

    /// Returns true for FreeBSD Capsicum capability-mode sandbox errors.
    fn isCapsicumError(): bool {
        return _ == .ENOTCAPABLE || _ == .ECAPMODE;
    }

    /// Returns true for FreeBSD/BSD-specific error codes absent from Linux POSIX errno.
    fn isFreebsdSpecific(): bool {
        return switch (_) {
            .EPROCLIM, .EBADRPC, .ERPCMISMATCH, .EPROGUNAVAIL, .EPROGMISMATCH,
            .EPROCUNAVAIL, .EFTYPE, .EAUTH, .ENEEDAUTH, .ENOATTR, .EDOOFUS,
            .ENOTCAPABLE, .ECAPMODE, .EINTEGRITY => true,
            else => false,
        };
    }

    fn codeName(): []const u8 {
        const idx: usize = @as(usize, @intFromEnum(_));
        if (idx < 98) return FREEBSD_ERRNO_NAMES[idx];
        return "UNKNOWN";
    }

    static fn fromName(sym: []const u8): FreebsdErrno? {
        if (sym.len == 0) return null;
        if (eqAscii(sym, "EWOULDBLOCK")) return .EAGAIN;
        if (eqAscii(sym, "ENOTSUP")) return .EOPNOTSUPP;
        let i: usize = 0;
        while (i < 98) {
            if (eqAscii(FREEBSD_ERRNO_NAMES[i], sym)) {
                return @enumFromInt(@as(u16, @intCast(i)));
            }
            i += 1;
        }
        return null;
    }

    /// Translates a FreeBSD native errno into the canonical CP `SystemErrno` (Linux numbering).
    fn toSystemErrno(): SystemErrno {
        return switch (_) {
            .SUCCESS => .SUCCESS,
            .EPERM => .EPERM,
            .ENOENT => .ENOENT,
            .ESRCH => .ESRCH,
            .EINTR => .EINTR,
            .EIO => .EIO,
            .ENXIO => .ENXIO,
            .E2BIG => .E2BIG,
            .ENOEXEC => .ENOEXEC,
            .EBADF => .EBADF,
            .ECHILD => .ECHILD,
            .EDEADLK => .EDEADLK,
            .ENOMEM => .ENOMEM,
            .EACCES => .EACCES,
            .EFAULT => .EFAULT,
            .ENOTBLK => .ENOTBLK,
            .EBUSY => .EBUSY,
            .EEXIST => .EEXIST,
            .EXDEV => .EXDEV,
            .ENODEV => .ENODEV,
            .ENOTDIR => .ENOTDIR,
            .EISDIR => .EISDIR,
            .EINVAL => .EINVAL,
            .ENFILE => .ENFILE,
            .EMFILE => .EMFILE,
            .ENOTTY => .ENOTTY,
            .ETXTBSY => .ETXTBSY,
            .EFBIG => .EFBIG,
            .ENOSPC => .ENOSPC,
            .ESPIPE => .ESPIPE,
            .EROFS => .EROFS,
            .EMLINK => .EMLINK,
            .EPIPE => .EPIPE,
            .EDOM => .EDOM,
            .ERANGE => .ERANGE,
            .EAGAIN => .EAGAIN,
            .EINPROGRESS => .EINPROGRESS,
            .EALREADY => .EALREADY,
            .ENOTSOCK => .ENOTSOCK,
            .EDESTADDRREQ => .EDESTADDRREQ,
            .EMSGSIZE => .EMSGSIZE,
            .EPROTOTYPE => .EPROTOTYPE,
            .ENOPROTOOPT => .ENOPROTOOPT,
            .EPROTONOSUPPORT => .EPROTONOSUPPORT,
            .ESOCKTNOSUPPORT => .ESOCKTNOSUPPORT,
            .EOPNOTSUPP => .ENOTSUP,
            .EPFNOSUPPORT => .EPFNOSUPPORT,
            .EAFNOSUPPORT => .EAFNOSUPPORT,
            .EADDRINUSE => .EADDRINUSE,
            .EADDRNOTAVAIL => .EADDRNOTAVAIL,
            .ENETDOWN => .ENETDOWN,
            .ENETUNREACH => .ENETUNREACH,
            .ENETRESET => .ENETRESET,
            .ECONNABORTED => .ECONNABORTED,
            .ECONNRESET => .ECONNRESET,
            .ENOBUFS => .ENOBUFS,
            .EISCONN => .EISCONN,
            .ENOTCONN => .ENOTCONN,
            .ESHUTDOWN => .ESHUTDOWN,
            .ETOOMANYREFS => .ETOOMANYREFS,
            .ETIMEDOUT => .ETIMEDOUT,
            .ECONNREFUSED => .ECONNREFUSED,
            .ELOOP => .ELOOP,
            .ENAMETOOLONG => .ENAMETOOLONG,
            .EHOSTDOWN => .EHOSTDOWN,
            .EHOSTUNREACH => .EHOSTUNREACH,
            .ENOTEMPTY => .ENOTEMPTY,
            .EUSERS => .EUSERS,
            .EDQUOT => .EDQUOT,
            .ESTALE => .ESTALE,
            .EREMOTE => .EREMOTE,
            .ENOLCK => .ENOLCK,
            .ENOSYS => .ENOSYS,
            .EIDRM => .EIDRM,
            .ENOMSG => .ENOMSG,
            .EOVERFLOW => .EOVERFLOW,
            .ECANCELED => .ECANCELED,
            .EILSEQ => .EILSEQ,
            .EBADMSG => .EBADMSG,
            .EMULTIHOP => .EMULTIHOP,
            .ENOLINK => .ENOLINK,
            .EPROTO => .EPROTO,
            .ENOTRECOVERABLE => .ENOTRECOVERABLE,
            .EOWNERDEAD => .EOWNERDEAD,
            else => .UNKNOWN,
        };
    }

    /// Translates a canonical `SystemErrno` back to FreeBSD native numbering.
    static fn fromSystemErrno(err: SystemErrno): FreebsdErrno? {
        return switch (err) {
            .SUCCESS => .SUCCESS,
            .EPERM => .EPERM,
            .ENOENT => .ENOENT,
            .ESRCH => .ESRCH,
            .EINTR => .EINTR,
            .EIO => .EIO,
            .ENXIO => .ENXIO,
            .E2BIG => .E2BIG,
            .ENOEXEC => .ENOEXEC,
            .EBADF => .EBADF,
            .ECHILD => .ECHILD,
            .EAGAIN => .EAGAIN,
            .ENOMEM => .ENOMEM,
            .EACCES => .EACCES,
            .EFAULT => .EFAULT,
            .ENOTBLK => .ENOTBLK,
            .EBUSY => .EBUSY,
            .EEXIST => .EEXIST,
            .EXDEV => .EXDEV,
            .ENODEV => .ENODEV,
            .ENOTDIR => .ENOTDIR,
            .EISDIR => .EISDIR,
            .EINVAL => .EINVAL,
            .ENFILE => .ENFILE,
            .EMFILE => .EMFILE,
            .ENOTTY => .ENOTTY,
            .ETXTBSY => .ETXTBSY,
            .EFBIG => .EFBIG,
            .ENOSPC => .ENOSPC,
            .ESPIPE => .ESPIPE,
            .EROFS => .EROFS,
            .EMLINK => .EMLINK,
            .EPIPE => .EPIPE,
            .EDOM => .EDOM,
            .ERANGE => .ERANGE,
            .EDEADLK => .EDEADLK,
            .ENAMETOOLONG => .ENAMETOOLONG,
            .ENOLCK => .ENOLCK,
            .ENOSYS => .ENOSYS,
            .ENOTEMPTY => .ENOTEMPTY,
            .ELOOP => .ELOOP,
            .EWOULDBLOCK => .EAGAIN,
            .ENOMSG => .ENOMSG,
            .EIDRM => .EIDRM,
            .EREMOTE => .EREMOTE,
            .ENOLINK => .ENOLINK,
            .EPROTO => .EPROTO,
            .EMULTIHOP => .EMULTIHOP,
            .EBADMSG => .EBADMSG,
            .EOVERFLOW => .EOVERFLOW,
            .EILSEQ => .EILSEQ,
            .EUSERS => .EUSERS,
            .ENOTSOCK => .ENOTSOCK,
            .EDESTADDRREQ => .EDESTADDRREQ,
            .EMSGSIZE => .EMSGSIZE,
            .EPROTOTYPE => .EPROTOTYPE,
            .ENOPROTOOPT => .ENOPROTOOPT,
            .EPROTONOSUPPORT => .EPROTONOSUPPORT,
            .ESOCKTNOSUPPORT => .ESOCKTNOSUPPORT,
            .ENOTSUP => .EOPNOTSUPP,
            .EPFNOSUPPORT => .EPFNOSUPPORT,
            .EAFNOSUPPORT => .EAFNOSUPPORT,
            .EADDRINUSE => .EADDRINUSE,
            .EADDRNOTAVAIL => .EADDRNOTAVAIL,
            .ENETDOWN => .ENETDOWN,
            .ENETUNREACH => .ENETUNREACH,
            .ENETRESET => .ENETRESET,
            .ECONNABORTED => .ECONNABORTED,
            .ECONNRESET => .ECONNRESET,
            .ENOBUFS => .ENOBUFS,
            .EISCONN => .EISCONN,
            .ENOTCONN => .ENOTCONN,
            .ESHUTDOWN => .ESHUTDOWN,
            .ETOOMANYREFS => .ETOOMANYREFS,
            .ETIMEDOUT => .ETIMEDOUT,
            .ECONNREFUSED => .ECONNREFUSED,
            .EHOSTDOWN => .EHOSTDOWN,
            .EHOSTUNREACH => .EHOSTUNREACH,
            .EALREADY => .EALREADY,
            .EINPROGRESS => .EINPROGRESS,
            .ESTALE => .ESTALE,
            .EDQUOT => .EDQUOT,
            .ECANCELED => .ECANCELED,
            .EOWNERDEAD => .EOWNERDEAD,
            .ENOTRECOVERABLE => .ENOTRECOVERABLE,
            else => null,
        };
    }
}

const FREEBSD_ERRNO_NAMES: [98][]const u8 = [
    "SUCCESS", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", "E2BIG",
    "ENOEXEC", "EBADF", "ECHILD", "EDEADLK", "ENOMEM", "EACCES", "EFAULT", "ENOTBLK",
    "EBUSY", "EEXIST", "EXDEV", "ENODEV", "ENOTDIR", "EISDIR", "EINVAL", "ENFILE",
    "EMFILE", "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", "EROFS", "EMLINK",
    "EPIPE", "EDOM", "ERANGE", "EAGAIN", "EINPROGRESS", "EALREADY", "ENOTSOCK", "EDESTADDRREQ",
    "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT", "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", "EOPNOTSUPP", "EPFNOSUPPORT", "EAFNOSUPPORT",
    "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH", "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
    "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS", "ETIMEDOUT", "ECONNREFUSED", "ELOOP", "ENAMETOOLONG",
    "EHOSTDOWN", "EHOSTUNREACH", "ENOTEMPTY", "EPROCLIM", "EUSERS", "EDQUOT", "ESTALE", "EREMOTE",
    "EBADRPC", "ERPCMISMATCH", "EPROGUNAVAIL", "EPROGMISMATCH", "EPROCUNAVAIL", "ENOLCK", "ENOSYS", "EFTYPE",
    "EAUTH", "ENEEDAUTH", "EIDRM", "ENOMSG", "EOVERFLOW", "ECANCELED", "EILSEQ", "ENOATTR",
    "EDOOFUS", "EBADMSG", "EMULTIHOP", "ENOLINK", "EPROTO", "ENOTCAPABLE", "ECAPMODE", "ENOTRECOVERABLE",
    "EOWNERDEAD", "EINTEGRITY",
];
