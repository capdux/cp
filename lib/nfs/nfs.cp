// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5531 ONC RPC v2 & RFC 1813 NFSv3 Wire Parser)

// ---------------------------------------------------------------------------
// Protocol Constants (RFC 5531 ONC RPC v2 & RFC 1813 NFSv3)
// ---------------------------------------------------------------------------

pub const NFS_PORT: u16 = 2049;
pub const RPC_VERSION: u32 = 2;
pub const RPC_CALL: u32 = 0;
pub const RPC_REPLY: u32 = 1;
pub const RPC_MAX_AUTH_LEN: usize = 400;
pub const RPC_CALL_NONE_HEADER_SIZE: usize = 40;

pub const RPC_PROG_PORTMAP: u32 = 100000;
pub const RPC_PROG_NFS: u32 = 100003;
pub const RPC_PROG_MOUNT: u32 = 100005;
pub const NFS_V3: u32 = 3;
pub const NFS_V4: u32 = 4;
pub const NFS3_FHSIZE: usize = 64;
pub const NFS3_FATTR_SIZE: usize = 84;

// Auth Flavors
pub const AUTH_NONE: u32 = 0;
pub const AUTH_SYS: u32 = 1;
pub const AUTH_SHORT: u32 = 2;
pub const AUTH_GSS: u32 = 6;

// Reply & Accept Status
pub const MSG_ACCEPTED: u32 = 0;
pub const MSG_DENIED: u32 = 1;
pub const ACCEPT_SUCCESS: u32 = 0;
pub const ACCEPT_PROG_UNAVAIL: u32 = 1;
pub const ACCEPT_PROG_MISMATCH: u32 = 2;
pub const ACCEPT_PROC_UNAVAIL: u32 = 3;
pub const ACCEPT_GARBAGE_ARGS: u32 = 4;

// NFSv3 Procedure Numbers
pub const NFS3_NULL: u32 = 0;
pub const NFS3_GETATTR: u32 = 1;
pub const NFS3_SETATTR: u32 = 2;
pub const NFS3_LOOKUP: u32 = 3;
pub const NFS3_ACCESS: u32 = 4;
pub const NFS3_READLINK: u32 = 5;
pub const NFS3_READ: u32 = 6;
pub const NFS3_WRITE: u32 = 7;
pub const NFS3_CREATE: u32 = 8;
pub const NFS3_MKDIR: u32 = 9;
pub const NFS3_REMOVE: u32 = 12;
pub const NFS3_READDIR: u32 = 16;
pub const NFS3_FSSTAT: u32 = 18;
pub const NFS3_COMMIT: u32 = 21;

// NFSv3 Status Codes
pub const NFS3_OK: u32 = 0;
pub const NFS3ERR_PERM: u32 = 1;
pub const NFS3ERR_NOENT: u32 = 2;
pub const NFS3ERR_IO: u32 = 5;
pub const NFS3ERR_ACCES: u32 = 13;
pub const NFS3ERR_EXIST: u32 = 17;
pub const NFS3ERR_NOTDIR: u32 = 20;
pub const NFS3ERR_ISDIR: u32 = 21;
pub const NFS3ERR_STALE: u32 = 70;

// NFSv3 File Types
pub const FTYPE_REG: u32 = 1;
pub const FTYPE_DIR: u32 = 2;
pub const FTYPE_BLK: u32 = 3;
pub const FTYPE_CHR: u32 = 4;
pub const FTYPE_LNK: u32 = 5;
pub const FTYPE_SOCK: u32 = 6;
pub const FTYPE_FIFO: u32 = 7;

// ---------------------------------------------------------------------------
// Internal Big-Endian & XDR Alignment Helpers
// ---------------------------------------------------------------------------

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Be(buf: []const u8, off: usize): u64 {
    const hi = readU32Be(buf, off) as u64;
    const lo = readU32Be(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn writeU32Be(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn pad4(len: usize): usize {
    return (4 - (len % 4)) % 4;
}

// ---------------------------------------------------------------------------
// TCP Record Marking (RFC 5531 §11)
// ---------------------------------------------------------------------------

/// Zero-copy view of a single ONC RPC TCP Record Marking fragment.
pub struct RpcRecordFrag {
    is_last: bool,
    payload: []const u8,
    consumed: usize,
}

/// Parses a 4-byte RFC 5531 TCP Record Mark header and its fragment payload.
pub fn parseRpcRecordFrag(data: []const u8): RpcRecordFrag? {
    if (data.len < 4) {
        return null;
    }
    const rm = readU32Be(data, 0);
    const is_last = (rm & 0x80000000) != 0;
    const frag_len = (rm & 0x7FFFFFFF) as usize;
    if (frag_len > data.len - 4) {
        return null;
    }
    return RpcRecordFrag {
        is_last: is_last,
        payload: data[4..4 + frag_len],
        consumed: 4 + frag_len,
    };
}

/// Encodes a 4-byte RFC 5531 TCP Record Mark header into `out`.
pub fn encodeRpcRecordMark(is_last: bool, payload_len: usize, out: []u8): usize? {
    if (out.len < 4 || payload_len > 0x7FFFFFFF) {
        return null;
    }
    let rm = payload_len as u32;
    if (is_last) {
        rm = rm | 0x80000000;
    }
    writeU32Be(out, 0, rm);
    return 4;
}

// ---------------------------------------------------------------------------
// ONC RPC v2 Auth, Call & Reply Models
// ---------------------------------------------------------------------------

/// Zero-copy ONC RPC `opaque_auth` credential or verifier.
pub struct RpcAuth {
    flavor: u32,
    body: []const u8,
}

struct ParsedAuth {
    auth: RpcAuth,
    next_off: usize,
}

fn parseRpcAuthAt(data: []const u8, off: usize): ParsedAuth? {
    if (off > data.len || data.len - off < 8) {
        return null;
    }
    const flavor = readU32Be(data, off);
    const body_len = readU32Be(data, off + 4) as usize;
    if (body_len > RPC_MAX_AUTH_LEN) {
        return null;
    }
    const pad = pad4(body_len);
    const rem = data.len - (off + 8);
    if (body_len > rem || pad > rem - body_len) {
        return null;
    }
    let i: usize = 0;
    while (i < pad) {
        if (data[off + 8 + body_len + i] != 0) {
            return null;
        }
        i += 1;
    }
    return ParsedAuth {
        auth: RpcAuth {
            flavor: flavor,
            body: data[off + 8..off + 8 + body_len],
        },
        next_off: off + 8 + body_len + pad,
    };
}

/// Zero-copy view of a parsed ONC RPC v2 Call message.
pub struct RpcCall {
    xid: u32,
    rpcvers: u32,
    prog: u32,
    vers: u32,
    proc: u32,
    cred: RpcAuth,
    verf: RpcAuth,
    params: []const u8,
}

/// Zero-copy view of a parsed ONC RPC v2 Reply message.
pub struct RpcReply {
    xid: u32,
    reply_stat: u32,
    verf: RpcAuth,
    accept_stat: u32,
    reject_stat: u32,
    results: []const u8,

    /// Returns true if `reply_stat == MSG_ACCEPTED` and `accept_stat == ACCEPT_SUCCESS`.
    fn isSuccess(): bool {
        return _.reply_stat == MSG_ACCEPTED && _.accept_stat == ACCEPT_SUCCESS;
    }
}

/// Parses an ONC RPC v2 Call message.
/// Strictly validates `msg_type == RPC_CALL`, `rpcvers == 2`, and `cred`/`verf` lengths <= 400 bytes.
pub fn parseRpcCall(data: []const u8): RpcCall? {
    if (data.len < 24) {
        return null;
    }
    const xid = readU32Be(data, 0);
    const msg_type = readU32Be(data, 4);
    if (msg_type != RPC_CALL) {
        return null;
    }
    const rpcvers = readU32Be(data, 8);
    if (rpcvers != RPC_VERSION) {
        return null;
    }
    const prog = readU32Be(data, 12);
    const vers = readU32Be(data, 16);
    const proc = readU32Be(data, 20);

    const parsed_cred = parseRpcAuthAt(data, 24) ?? return null;
    const parsed_verf = parseRpcAuthAt(data, parsed_cred.next_off) ?? return null;

    return RpcCall {
        xid: xid,
        rpcvers: rpcvers,
        prog: prog,
        vers: vers,
        proc: proc,
        cred: parsed_cred.auth,
        verf: parsed_verf.auth,
        params: data[parsed_verf.next_off..],
    };
}

/// Parses an ONC RPC v2 Reply message (`MSG_ACCEPTED` or `MSG_DENIED`).
pub fn parseRpcReply(data: []const u8): RpcReply? {
    if (data.len < 12) {
        return null;
    }
    const xid = readU32Be(data, 0);
    const msg_type = readU32Be(data, 4);
    if (msg_type != RPC_REPLY) {
        return null;
    }
    const reply_stat = readU32Be(data, 8);

    if (reply_stat == MSG_ACCEPTED) {
        const parsed_verf = parseRpcAuthAt(data, 12) ?? return null;
        if (data.len - parsed_verf.next_off < 4) {
            return null;
        }
        const accept_stat = readU32Be(data, parsed_verf.next_off);
        return RpcReply {
            xid: xid,
            reply_stat: reply_stat,
            verf: parsed_verf.auth,
            accept_stat: accept_stat,
            reject_stat: 0,
            results: data[parsed_verf.next_off + 4..],
        };
    } else if (reply_stat == MSG_DENIED) {
        if (data.len < 16) {
            return null;
        }
        const reject_stat = readU32Be(data, 12);
        return RpcReply {
            xid: xid,
            reply_stat: reply_stat,
            verf: RpcAuth { flavor: AUTH_NONE, body: "" },
            accept_stat: 0,
            reject_stat: reject_stat,
            results: data[16..],
        };
    }

    return null;
}

/// Encodes a 40-byte ONC RPC v2 Call header with `AUTH_NONE` cred and verf into `out`.
pub fn encodeRpcCallHeader(
    xid: u32,
    prog: u32,
    vers: u32,
    proc: u32,
    out: []u8,
): usize? {
    if (out.len < RPC_CALL_NONE_HEADER_SIZE) {
        return null;
    }
    writeU32Be(out, 0, xid);
    writeU32Be(out, 4, RPC_CALL);
    writeU32Be(out, 8, RPC_VERSION);
    writeU32Be(out, 12, prog);
    writeU32Be(out, 16, vers);
    writeU32Be(out, 20, proc);
    // cred: AUTH_NONE, len=0
    writeU32Be(out, 24, AUTH_NONE);
    writeU32Be(out, 28, 0);
    // verf: AUTH_NONE, len=0
    writeU32Be(out, 32, AUTH_NONE);
    writeU32Be(out, 36, 0);
    return RPC_CALL_NONE_HEADER_SIZE;
}

// ---------------------------------------------------------------------------
// RFC 1813 NFSv3 File Handle & 84-Byte fattr3
// ---------------------------------------------------------------------------

/// Decoded 84-byte RFC 1813 NFSv3 `fattr3` file attributes structure.
pub struct Nfs3Fattr {
    ftype: u32,
    mode: u32,
    nlink: u32,
    uid: u32,
    gid: u32,
    size: u64,
    used: u64,
    rdev_major: u32,
    rdev_minor: u32,
    fsid: u64,
    fileid: u64,
    atime_sec: u32,
    atime_nsec: u32,
    mtime_sec: u32,
    mtime_nsec: u32,
    ctime_sec: u32,
    ctime_nsec: u32,
}

/// Parses an XDR-encoded NFSv3 `nfs_fh3` file handle (`len <= 64`, 4-byte padded).
pub fn parseNfs3FileHandle(data: []const u8): ([]const u8)? {
    if (data.len < 4) {
        return null;
    }
    const fh_len = readU32Be(data, 0) as usize;
    if (fh_len > NFS3_FHSIZE) {
        return null;
    }
    const pad = pad4(fh_len);
    const rem = data.len - 4;
    if (fh_len > rem || pad > rem - fh_len) {
        return null;
    }
    let i: usize = 0;
    while (i < pad) {
        if (data[4 + fh_len + i] != 0) {
            return null;
        }
        i += 1;
    }
    return data[4..4 + fh_len];
}

/// Parses an 84-byte RFC 1813 NFSv3 `fattr3` structure from `data`.
pub fn parseNfs3Fattr(data: []const u8): Nfs3Fattr? {
    if (data.len < NFS3_FATTR_SIZE) {
        return null;
    }
    return Nfs3Fattr {
        ftype: readU32Be(data, 0),
        mode: readU32Be(data, 4),
        nlink: readU32Be(data, 8),
        uid: readU32Be(data, 12),
        gid: readU32Be(data, 16),
        size: readU64Be(data, 20),
        used: readU64Be(data, 28),
        rdev_major: readU32Be(data, 36),
        rdev_minor: readU32Be(data, 40),
        fsid: readU64Be(data, 44),
        fileid: readU64Be(data, 52),
        atime_sec: readU32Be(data, 60),
        atime_nsec: readU32Be(data, 64),
        mtime_sec: readU32Be(data, 68),
        mtime_nsec: readU32Be(data, 72),
        ctime_sec: readU32Be(data, 76),
        ctime_nsec: readU32Be(data, 80),
    };
}
