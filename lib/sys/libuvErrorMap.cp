// Source: Absorbed from /data/dev/bun/src/sys/libuv_error_map.cp | Synced: 2026-11-22T15:42:00Z
// ============================================================================
// lib/sys/libuvErrorMap.cp — Zero-Alloc libuv / Node.js System Error Code Map
// ============================================================================
// Derived from uv.h error definitions used by Node.js and Bun when formatting
// POSIX syscall and getaddrinfo (EAI_*) diagnostics. 100% pure CP, zero heap
// allocation, deterministic lookup & Node.js-compatible error string formatting.

pub const LIBUV_ERROR_COUNT: usize = 84;
pub const LIBUV_UNKNOWN_MESSAGE: []const u8 = "unknown error";

pub enum LibuvErrorCategory: u8 {
    Filesystem = 0,
    Network = 1,
    Dns = 2,
    Permission = 3,
    Resource = 4,
    Io = 5,
    Argument = 6,
    Process = 7,
    Unknown = 8,

    fn asStr(): []const u8 {
        return switch (_) {
            .Filesystem => "filesystem",
            .Network => "network",
            .Dns => "dns",
            .Permission => "permission",
            .Resource => "resource",
            .Io => "io",
            .Argument => "argument",
            .Process => "process",
            .Unknown => "unknown",
        };
    }
}

/// Canonical libuv / Node.js error code identifiers (84 entries from uv.h).
pub enum LibuvErrno: u8 {
    E2BIG = 0,
    EACCES = 1,
    EADDRINUSE = 2,
    EADDRNOTAVAIL = 3,
    EAFNOSUPPORT = 4,
    EAGAIN = 5,
    EAI_ADDRFAMILY = 6,
    EAI_AGAIN = 7,
    EAI_BADFLAGS = 8,
    EAI_BADHINTS = 9,
    EAI_CANCELED = 10,
    EAI_FAIL = 11,
    EAI_FAMILY = 12,
    EAI_MEMORY = 13,
    EAI_NODATA = 14,
    EAI_NONAME = 15,
    EAI_OVERFLOW = 16,
    EAI_PROTOCOL = 17,
    EAI_SERVICE = 18,
    EAI_SOCKTYPE = 19,
    EALREADY = 20,
    EBADF = 21,
    EBUSY = 22,
    ECANCELED = 23,
    ECHARSET = 24,
    ECONNABORTED = 25,
    ECONNREFUSED = 26,
    ECONNRESET = 27,
    EDESTADDRREQ = 28,
    EEXIST = 29,
    EFAULT = 30,
    EFBIG = 31,
    EHOSTUNREACH = 32,
    EINTR = 33,
    EINVAL = 34,
    EIO = 35,
    EISCONN = 36,
    EISDIR = 37,
    ELOOP = 38,
    EMFILE = 39,
    EMSGSIZE = 40,
    ENAMETOOLONG = 41,
    ENETDOWN = 42,
    ENETUNREACH = 43,
    ENFILE = 44,
    ENOBUFS = 45,
    ENODEV = 46,
    ENOENT = 47,
    ENOMEM = 48,
    ENONET = 49,
    ENOPROTOOPT = 50,
    ENOSPC = 51,
    ENOSYS = 52,
    ENOTCONN = 53,
    ENOTDIR = 54,
    ENOTEMPTY = 55,
    ENOTSOCK = 56,
    ENOTSUP = 57,
    EOVERFLOW = 58,
    EPERM = 59,
    EPIPE = 60,
    EPROTO = 61,
    EPROTONOSUPPORT = 62,
    EPROTOTYPE = 63,
    ERANGE = 64,
    EROFS = 65,
    ESHUTDOWN = 66,
    ESPIPE = 67,
    ESRCH = 68,
    ETIMEDOUT = 69,
    ETXTBSY = 70,
    EXDEV = 71,
    UNKNOWN = 72,
    EOF = 73,
    ENXIO = 74,
    EMLINK = 75,
    EHOSTDOWN = 76,
    EREMOTEIO = 77,
    ENOTTY = 78,
    EFTYPE = 79,
    EILSEQ = 80,
    ESOCKTNOSUPPORT = 81,
    ENODATA = 82,
    EUNATCH = 83,

    fn codeName(): []const u8 {
        return LibuvErrorMap.codeName(_);
    }

    fn message(): []const u8 {
        return LibuvErrorMap.messageFor(_);
    }

    fn category(): LibuvErrorCategory {
        return LibuvErrorMap.categoryFor(_);
    }

    fn isDnsError(): bool {
        return LibuvErrorMap.isDnsError(_);
    }

    fn isRetryable(): bool {
        return LibuvErrorMap.isRetryable(_);
    }
}

pub struct LibuvErrorEntry {
    code: LibuvErrno,
    name: []const u8,
    message: []const u8,
    category: LibuvErrorCategory,
    retryable: bool,
}

fn eqAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 97 && c <= 122) return c - 32;
    return c;
}

fn eqIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toUpperAscii(a[i]) != toUpperAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < src.len && p < out.len) {
        out[p] = src[i];
        p += 1;
        i += 1;
    }
    return p;
}

const LIBUV_ENTRIES: [84]LibuvErrorEntry = [
    LibuvErrorEntry{ code: .E2BIG, name: "E2BIG", message: "argument list too long", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .EACCES, name: "EACCES", message: "permission denied", category: .Permission, retryable: false },
    LibuvErrorEntry{ code: .EADDRINUSE, name: "EADDRINUSE", message: "address already in use", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EADDRNOTAVAIL, name: "EADDRNOTAVAIL", message: "address not available", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EAFNOSUPPORT, name: "EAFNOSUPPORT", message: "address family not supported", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EAGAIN, name: "EAGAIN", message: "resource temporarily unavailable", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .EAI_ADDRFAMILY, name: "EAI_ADDRFAMILY", message: "address family not supported", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_AGAIN, name: "EAI_AGAIN", message: "temporary failure", category: .Dns, retryable: true },
    LibuvErrorEntry{ code: .EAI_BADFLAGS, name: "EAI_BADFLAGS", message: "bad ai_flags value", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_BADHINTS, name: "EAI_BADHINTS", message: "invalid value for hints", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_CANCELED, name: "EAI_CANCELED", message: "request canceled", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_FAIL, name: "EAI_FAIL", message: "permanent failure", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_FAMILY, name: "EAI_FAMILY", message: "ai_family not supported", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_MEMORY, name: "EAI_MEMORY", message: "out of memory", category: .Dns, retryable: true },
    LibuvErrorEntry{ code: .EAI_NODATA, name: "EAI_NODATA", message: "no address", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_NONAME, name: "EAI_NONAME", message: "unknown node or service", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_OVERFLOW, name: "EAI_OVERFLOW", message: "argument buffer overflow", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_PROTOCOL, name: "EAI_PROTOCOL", message: "resolved protocol is unknown", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_SERVICE, name: "EAI_SERVICE", message: "service not available for socket type", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EAI_SOCKTYPE, name: "EAI_SOCKTYPE", message: "socket type not supported", category: .Dns, retryable: false },
    LibuvErrorEntry{ code: .EALREADY, name: "EALREADY", message: "connection already in progress", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .EBADF, name: "EBADF", message: "bad file descriptor", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EBUSY, name: "EBUSY", message: "resource busy or locked", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .ECANCELED, name: "ECANCELED", message: "operation canceled", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .ECHARSET, name: "ECHARSET", message: "invalid Unicode character", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .ECONNABORTED, name: "ECONNABORTED", message: "software caused connection abort", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ECONNREFUSED, name: "ECONNREFUSED", message: "connection refused", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ECONNRESET, name: "ECONNRESET", message: "connection reset by peer", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .EDESTADDRREQ, name: "EDESTADDRREQ", message: "destination address required", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EEXIST, name: "EEXIST", message: "file already exists", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .EFAULT, name: "EFAULT", message: "bad address in system call argument", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .EFBIG, name: "EFBIG", message: "file too large", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .EHOSTUNREACH, name: "EHOSTUNREACH", message: "host is unreachable", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .EINTR, name: "EINTR", message: "interrupted system call", category: .Io, retryable: true },
    LibuvErrorEntry{ code: .EINVAL, name: "EINVAL", message: "invalid argument", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .EIO, name: "EIO", message: "i/o error", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EISCONN, name: "EISCONN", message: "socket is already connected", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EISDIR, name: "EISDIR", message: "illegal operation on a directory", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ELOOP, name: "ELOOP", message: "too many symbolic links encountered", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .EMFILE, name: "EMFILE", message: "too many open files", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .EMSGSIZE, name: "EMSGSIZE", message: "message too long", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ENAMETOOLONG, name: "ENAMETOOLONG", message: "name too long", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ENETDOWN, name: "ENETDOWN", message: "network is down", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ENETUNREACH, name: "ENETUNREACH", message: "network is unreachable", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ENFILE, name: "ENFILE", message: "file table overflow", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .ENOBUFS, name: "ENOBUFS", message: "no buffer space available", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .ENODEV, name: "ENODEV", message: "no such device", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .ENOENT, name: "ENOENT", message: "no such file or directory", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ENOMEM, name: "ENOMEM", message: "not enough memory", category: .Resource, retryable: true },
    LibuvErrorEntry{ code: .ENONET, name: "ENONET", message: "machine is not on the network", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ENOPROTOOPT, name: "ENOPROTOOPT", message: "protocol not available", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ENOSPC, name: "ENOSPC", message: "no space left on device", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ENOSYS, name: "ENOSYS", message: "function not implemented", category: .Process, retryable: false },
    LibuvErrorEntry{ code: .ENOTCONN, name: "ENOTCONN", message: "socket is not connected", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ENOTDIR, name: "ENOTDIR", message: "not a directory", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ENOTEMPTY, name: "ENOTEMPTY", message: "directory not empty", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ENOTSOCK, name: "ENOTSOCK", message: "socket operation on non-socket", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ENOTSUP, name: "ENOTSUP", message: "operation not supported on socket", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EOVERFLOW, name: "EOVERFLOW", message: "value too large for defined data type", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .EPERM, name: "EPERM", message: "operation not permitted", category: .Permission, retryable: false },
    LibuvErrorEntry{ code: .EPIPE, name: "EPIPE", message: "broken pipe", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EPROTO, name: "EPROTO", message: "protocol error", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EPROTONOSUPPORT, name: "EPROTONOSUPPORT", message: "protocol not supported", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .EPROTOTYPE, name: "EPROTOTYPE", message: "protocol wrong type for socket", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ERANGE, name: "ERANGE", message: "result too large", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .EROFS, name: "EROFS", message: "read-only file system", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .ESHUTDOWN, name: "ESHUTDOWN", message: "cannot send after transport endpoint shutdown", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ESPIPE, name: "ESPIPE", message: "invalid seek", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .ESRCH, name: "ESRCH", message: "no such process", category: .Process, retryable: false },
    LibuvErrorEntry{ code: .ETIMEDOUT, name: "ETIMEDOUT", message: "connection timed out", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .ETXTBSY, name: "ETXTBSY", message: "text file is busy", category: .Filesystem, retryable: true },
    LibuvErrorEntry{ code: .EXDEV, name: "EXDEV", message: "cross-device link not permitted", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .UNKNOWN, name: "UNKNOWN", message: "unknown error", category: .Unknown, retryable: false },
    LibuvErrorEntry{ code: .EOF, name: "EOF", message: "end of file", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .ENXIO, name: "ENXIO", message: "no such device or address", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EMLINK, name: "EMLINK", message: "too many links", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .EHOSTDOWN, name: "EHOSTDOWN", message: "host is down", category: .Network, retryable: true },
    LibuvErrorEntry{ code: .EREMOTEIO, name: "EREMOTEIO", message: "remote I/O error", category: .Io, retryable: true },
    LibuvErrorEntry{ code: .ENOTTY, name: "ENOTTY", message: "inappropriate ioctl for device", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EFTYPE, name: "EFTYPE", message: "inappropriate file type or format", category: .Filesystem, retryable: false },
    LibuvErrorEntry{ code: .EILSEQ, name: "EILSEQ", message: "illegal byte sequence", category: .Argument, retryable: false },
    LibuvErrorEntry{ code: .ESOCKTNOSUPPORT, name: "ESOCKTNOSUPPORT", message: "socket type not supported", category: .Network, retryable: false },
    LibuvErrorEntry{ code: .ENODATA, name: "ENODATA", message: "no data available", category: .Io, retryable: false },
    LibuvErrorEntry{ code: .EUNATCH, name: "EUNATCH", message: "protocol driver not attached", category: .Network, retryable: false },
];

pub struct LibuvErrorMap {
    static fn entryCount(): usize {
        return LIBUV_ERROR_COUNT;
    }

    static fn entryAt(index: usize): LibuvErrorEntry? {
        if (index >= LIBUV_ERROR_COUNT) return null;
        return LIBUV_ENTRIES[index];
    }

    static fn entryFor(code: LibuvErrno): LibuvErrorEntry {
        const idx: usize = @intFromEnum(code);
        if (idx < LIBUV_ERROR_COUNT) {
            return LIBUV_ENTRIES[idx];
        }
        return LIBUV_ENTRIES[@intFromEnum(LibuvErrno.UNKNOWN)];
    }

    static fn codeName(code: LibuvErrno): []const u8 {
        return LibuvErrorMap.entryFor(code).name;
    }

    static fn messageFor(code: LibuvErrno): []const u8 {
        return LibuvErrorMap.entryFor(code).message;
    }

    static fn categoryFor(code: LibuvErrno): LibuvErrorCategory {
        return LibuvErrorMap.entryFor(code).category;
    }

    static fn isDnsError(code: LibuvErrno): bool {
        return LibuvErrorMap.entryFor(code).category == .Dns;
    }

    static fn isRetryable(code: LibuvErrno): bool {
        return LibuvErrorMap.entryFor(code).retryable;
    }

    static fn lookupByName(name: []const u8): LibuvErrorEntry? {
        if (name.len == 0) return null;
        let i: usize = 0;
        while (i < LIBUV_ERROR_COUNT) {
            if (eqAscii(LIBUV_ENTRIES[i].name, name)) {
                return LIBUV_ENTRIES[i];
            }
            i += 1;
        }
        return null;
    }

    static fn lookupByNameIgnoreCase(name: []const u8): LibuvErrorEntry? {
        if (name.len == 0) return null;
        let i: usize = 0;
        while (i < LIBUV_ERROR_COUNT) {
            if (eqIgnoreCaseAscii(LIBUV_ENTRIES[i].name, name)) {
                return LIBUV_ENTRIES[i];
            }
            i += 1;
        }
        return null;
    }

    static fn fromCodeName(name: []const u8): LibuvErrno? {
        if (entry := LibuvErrorMap.lookupByName(name)) {
            return entry.code;
        }
        return null;
    }

    static fn messageByName(name: []const u8): []const u8 {
        if (entry := LibuvErrorMap.lookupByName(name)) {
            return entry.message;
        }
        return LIBUV_UNKNOWN_MESSAGE;
    }

    /// Formats a Node.js / Bun style system error diagnostic into `out`:
    /// - `CODE: message` (when syscall and path are empty)
    /// - `CODE: message, <syscall>` (when syscall is non-empty and path is empty)
    /// - `CODE: message, <syscall> '<path>'` (when both syscall and path are non-empty)
    /// - `CODE: message, '<path>'` (when syscall is empty and path is non-empty)
    static fn formatError(code: LibuvErrno, syscall: []const u8, path: []const u8, out: []u8): []const u8 {
        const entry = LibuvErrorMap.entryFor(code);
        let p: usize = 0;
        p = appendBytes(out, p, entry.name);
        p = appendBytes(out, p, ": ");
        p = appendBytes(out, p, entry.message);
        if (syscall.len > 0 && path.len > 0) {
            p = appendBytes(out, p, ", ");
            p = appendBytes(out, p, syscall);
            p = appendBytes(out, p, " '");
            p = appendBytes(out, p, path);
            p = appendBytes(out, p, "'");
        } else if (syscall.len > 0) {
            p = appendBytes(out, p, ", ");
            p = appendBytes(out, p, syscall);
        } else if (path.len > 0) {
            p = appendBytes(out, p, ", '");
            p = appendBytes(out, p, path);
            p = appendBytes(out, p, "'");
        }
        return out[0..p];
    }
}
