// Reference-hardened against RustCrypto/SSH (MIT/Apache-2.0)
// Source: cp/lib/ssh_wire/wire.cp (RFC 4251 SSH-2 Wire Codec & 16-lane SIMD byte ops)
const std = @import("std");
type NameSlice = []const u8;
type SshVec16u8 = @Vector(16, u8);

/// Maximum allowed length for a single RFC 4251 wire `string` (16 MiB).
/// Prevents forged 0xFFFFFFFF length prefixes from causing integer overflow or OOM.
pub const SSH_MAX_WIRE_STRING_LEN: usize = 16 * 1024 * 1024;
/// Maximum allowed number of entries in an RFC 4251 `name-list`.
pub const SSH_MAX_NAME_LIST_COUNT: usize = 4096;
/// Maximum allowed byte length for an RFC 4251 `mpint` (64 KiB).
pub const SSH_MAX_MPINT_LEN: usize = 64 * 1024;

pub err SshError {
    UnexpectedEof,
    InvalidFormat,
    InvalidKeyType,
    KeyTypeMismatch,
    UnsupportedCipher,
    UnsupportedKdf,
    CheckintMismatch,
    InvalidMagic,
    InvalidBase64,
    InvalidSignature,
    InvalidNamespace,
    InvalidHashAlgorithm,
    PayloadTooLarge,
    BufferTooSmall,
    OutOfMemory,
}

fn sshLoadVec16(data: []const u8, off: usize): SshVec16u8 {
    const v: SshVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD byte copy for SSH wire payloads, key blobs, and signatures.
pub fn simdSshCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v = sshLoadVec16(src, i);
        const arr: [16]u8 = v;
        @memcpy(dst[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference for `simdSshCopyBytes` (differential verification).
pub fn simdSshCopyBytesScalar(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane `@Vector(16, u8)` constant-time byte equality check (no early exit on mismatch).
/// Suitable for comparing public keys, signatures, fingerprints, and authentication tags.
pub fn simdSshCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    const n = a.len;
    let i: usize = 0;
    let acc_v: SshVec16u8 = @splat(0);
    while (i + 16 <= n) {
        const va = sshLoadVec16(a, i);
        const vb = sshLoadVec16(b, i);
        acc_v = acc_v | (va ^ vb);
        i += 16;
    }
    let tail_diff: u8 = 0;
    while (i < n) : (i += 1) {
        tail_diff = tail_diff | (a[i] ^ b[i]);
    }
    const vec_diff: u8 = @reduce(.Or, acc_v);
    return (vec_diff | tail_diff) == 0;
}

/// Scalar constant-time reference for `simdSshCtEqual` (differential verification).
pub fn simdSshCtEqualScalar(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn isValidNameListChar(b: u8): bool {
    return b >= 0x21 and b <= 0x7e and b != ',';
}

pub struct SshReader {
    data: []const u8;
    pos: usize = 0;

    static fn init(data: []const u8): SshReader {
        return SshReader{
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn readByte(*_): SshError!u8 {
        if (_.pos >= _.data.len) {
            return SshError.UnexpectedEof;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn readBytes(*_, len: usize): SshError![]const u8 {
        if (_.pos > _.data.len or len > _.data.len - _.pos) {
            return SshError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    fn readU32(*_): SshError!u32 {
        const raw = try _.readBytes(4);
        return std.mem.readInt(u32, raw[0..4], .big);
    }

    fn readString(*_): SshError![]const u8 {
        const len_u32 = try _.readU32();
        const len = @as(usize, len_u32);
        if (len > SSH_MAX_WIRE_STRING_LEN) {
            return SshError.PayloadTooLarge;
        }
        return try _.readBytes(len);
    }

    /// Reads an RFC 4251 `mpint` (signed or unsigned) with strict canonical encoding checks:
    /// - Zero must be encoded as empty byte string (rejects `[0x00]`).
    /// - Rejects redundant leading `0x00` when next byte's MSB is 0.
    /// - Rejects redundant leading `0xff` when next byte's MSB is 1.
    fn readMpint(*_): SshError![]const u8 {
        const bytes = try _.readString();
        if (bytes.len > SSH_MAX_MPINT_LEN) {
            return SshError.PayloadTooLarge;
        }
        if (bytes.len == 0) {
            return bytes;
        }
        if (bytes.len == 1 and bytes[0] == 0x00) {
            return SshError.InvalidFormat;
        }
        if (bytes.len >= 2) {
            if (bytes[0] == 0x00 and (bytes[1] & 0x80) == 0) {
                return SshError.InvalidFormat;
            }
            if (bytes[0] == 0xff and (bytes[1] & 0x80) != 0) {
                return SshError.InvalidFormat;
            }
        }
        return bytes;
    }

    /// Reads an RFC 4251 non-negative `mpint` (as required for RSA/DH/DSA key components).
    /// Rejects negative values (MSB 0x80 set on first byte) and non-canonical leading `0x00`.
    fn readUnsignedMpint(*_): SshError![]const u8 {
        const bytes = try _.readMpint();
        if (bytes.len > 0 and (bytes[0] & 0x80) != 0) {
            return SshError.InvalidFormat;
        }
        return bytes;
    }

    fn readNameList(*_, alloc: std.mem.Allocator): SshError![][]const u8 {
        const raw = try _.readString();
        if (raw.len == 0) {
            let empty_list = std.ArrayList(NameSlice).empty;
            return empty_list.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
        }
        let list = std.ArrayList(NameSlice).empty;
        errdefer list.deinit(alloc);

        let it = std.mem.splitScalar(u8, raw, ',');
        while (item := it.next()) {
            if (item.len == 0) {
                return SshError.InvalidFormat;
            }
            if (list.items.len >= SSH_MAX_NAME_LIST_COUNT) {
                return SshError.PayloadTooLarge;
            }
            for (ch of item) {
                if (!isValidNameListChar(ch)) {
                    return SshError.InvalidFormat;
                }
            }
            list.append(alloc, item) catch return SshError.OutOfMemory;
        }
        return list.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
    }
}

pub struct SshWriter {
    buf: std.ArrayList(u8) = .empty;
    alloc: std.mem.Allocator;

    static fn init(alloc: std.mem.Allocator): SshWriter {
        return SshWriter{
            buf: std.ArrayList(u8).empty,
            alloc: alloc,
        };
    }

    fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    fn items(): []const u8 {
        return _.buf.items;
    }

    fn toOwnedSlice(*_): SshError![]u8 {
        return _.buf.toOwnedSlice(_.alloc) catch return SshError.OutOfMemory;
    }

    fn writeByte(*_, b: u8): SshError!void {
        _.buf.append(_.alloc, b) catch return SshError.OutOfMemory;
    }

    fn writeRaw(*_, data: []const u8): SshError!void {
        if (data.len == 0) {
            return;
        }
        const old_len = _.buf.items.len;
        _.buf.resize(_.alloc, old_len + data.len) catch return SshError.OutOfMemory;
        simdSshCopyBytes(_.buf.items[old_len .. old_len + data.len], data);
    }

    fn writeU32(*_, val: u32): SshError!void {
        let tmp = std.mem.zeroes([4]u8);
        std.mem.writeInt(u32, &tmp, val, .big);
        try _.writeRaw(&tmp);
    }

    fn writeString(*_, data: []const u8): SshError!void {
        if (data.len > SSH_MAX_WIRE_STRING_LEN) {
            return SshError.PayloadTooLarge;
        }
        try _.writeU32(@as(u32, @intCast(data.len)));
        try _.writeRaw(data);
    }

    /// Writes an unsigned big-endian integer in RFC 4251 mpint wire format.
    fn writeMpint(*_, bytes: []const u8): SshError!void {
        let start: usize = 0;
        while (start < bytes.len and bytes[start] == 0) {
            start += 1;
        }
        const trimmed = bytes[start..];
        if (trimmed.len == 0) {
            try _.writeU32(0);
            return;
        }
        if (trimmed.len > SSH_MAX_MPINT_LEN) {
            return SshError.PayloadTooLarge;
        }
        if ((trimmed[0] & 0x80) != 0) {
            if (trimmed.len + 1 > SSH_MAX_MPINT_LEN) {
                return SshError.PayloadTooLarge;
            }
            try _.writeU32(@as(u32, @intCast(trimmed.len + 1)));
            try _.writeByte(0x00);
            try _.writeRaw(trimmed);
        } else {
            try _.writeString(trimmed);
        }
    }

    /// Writes a signed two's complement big-endian integer in RFC 4251 mpint wire format.
    fn writeSignedMpint(*_, bytes: []const u8): SshError!void {
        if (bytes.len == 0) {
            try _.writeU32(0);
            return;
        }
        const is_negative = (bytes[0] & 0x80) != 0;
        if (!is_negative) {
            try _.writeMpint(bytes);
            return;
        }
        let start: usize = 0;
        while (start + 1 < bytes.len and bytes[start] == 0xff and (bytes[start + 1] & 0x80) != 0) {
            start += 1;
        }
        const trimmed = bytes[start..];
        if (trimmed.len > SSH_MAX_MPINT_LEN) {
            return SshError.PayloadTooLarge;
        }
        try _.writeString(trimmed);
    }

    fn writeNameList(*_, names: []const []const u8): SshError!void {
        if (names.len == 0) {
            try _.writeU32(0);
            return;
        }
        if (names.len > SSH_MAX_NAME_LIST_COUNT) {
            return SshError.PayloadTooLarge;
        }
        let total_len: usize = names.len - 1;
        for (name of names) {
            if (name.len == 0) {
                return SshError.InvalidFormat;
            }
            for (ch of name) {
                if (!isValidNameListChar(ch)) {
                    return SshError.InvalidFormat;
                }
            }
            total_len += name.len;
        }
        if (total_len > SSH_MAX_WIRE_STRING_LEN) {
            return SshError.PayloadTooLarge;
        }
        try _.writeU32(@as(u32, @intCast(total_len)));
        let idx: usize = 0;
        while (idx < names.len) {
            if (idx > 0) {
                try _.writeByte(',');
            }
            try _.writeRaw(names[idx]);
            idx += 1;
        }
    }
}
