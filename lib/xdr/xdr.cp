// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4506 XDR External Data Representation Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants & 4-Byte Alignment Helpers (RFC 4506)
// ---------------------------------------------------------------------------

pub const XDR_UNIT: usize = 4;

/// Returns the number of trailing zero padding bytes (`0..3`) required to align `len` to 4 bytes.
pub fn xdrPadLen(len: usize): usize {
    return (XDR_UNIT - (len % XDR_UNIT)) % XDR_UNIT;
}

/// Returns `len` rounded up to the next 4-byte XDR boundary.
pub fn xdrAlignedLen(len: usize): usize {
    return len + xdrPadLen(len);
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
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

fn writeU64Be(buf: []u8, off: usize, val: u64) {
    writeU32Be(buf, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    writeU32Be(buf, off + 4, (val & 0xFFFFFFFF) as u32);
}

// ---------------------------------------------------------------------------
// Zero-Copy XDR Reader
// ---------------------------------------------------------------------------

/// Zero-copy RFC 4506 XDR streaming reader over a big-endian 4-byte-aligned byte slice.
pub struct XdrReader {
    data: []const u8,
    pos: usize,

    pub fn init(data: []const u8): XdrReader {
        return XdrReader {
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn skip(*_, bytes: usize): bool {
        if (bytes > _.remaining()) {
            return false;
        }
        _.pos += bytes;
        return true;
    }

    fn readU32(*_): u32? {
        if (_.remaining() < 4) {
            return null;
        }
        const v = readU32Be(_.data, _.pos);
        _.pos += 4;
        return v;
    }

    fn readI32(*_): i32? {
        if (u := _.readU32()) {
            const v: i32 = @bitCast(u);
            return v;
        }
        return null;
    }

    fn readU64(*_): u64? {
        if (_.remaining() < 8) {
            return null;
        }
        const v = readU64Be(_.data, _.pos);
        _.pos += 8;
        return v;
    }

    fn readI64(*_): i64? {
        if (u := _.readU64()) {
            const v: i64 = @bitCast(u);
            return v;
        }
        return null;
    }

    /// Reads an XDR boolean (4-byte big-endian uint). Strictly rejects values other than `0` or `1`.
    fn readBool(*_): bool? {
        if (_.remaining() < 4) {
            return null;
        }
        const v = readU32Be(_.data, _.pos);
        if (v == 0) {
            _.pos += 4;
            return false;
        }
        if (v == 1) {
            _.pos += 4;
            return true;
        }
        return null;
    }

    fn readF32(*_): f32? {
        if (u := _.readU32()) {
            const v: f32 = @bitCast(u);
            return v;
        }
        return null;
    }

    fn readF64(*_): f64? {
        if (u := _.readU64()) {
            const v: f64 = @bitCast(u);
            return v;
        }
        return null;
    }

    /// Reads `len` bytes of fixed-length opaque data and validates/skips `0..3` trailing zero padding bytes.
    fn readFixedOpaque(*_, len: usize): ([]const u8)? {
        const pad = xdrPadLen(len);
        const rem = _.remaining();
        if (len > rem || pad > rem - len) {
            return null;
        }
        // RFC 4506 §4.9: all padding bytes MUST be zero
        let i: usize = 0;
        while (i < pad) {
            if (_.data[_.pos + len + i] != 0) {
                return null;
            }
            i += 1;
        }
        const slice = _.data[_.pos.._.pos + len];
        _.pos += len + pad;
        return slice;
    }

    /// Reads a variable-length opaque field (`u32` byte count followed by 4-byte-padded data).
    fn readOpaque(*_): ([]const u8)? {
        const saved = _.pos;
        const len_u32 = _.readU32() ?? return null;
        if (slice := _.readFixedOpaque(len_u32 as usize)) {
            return slice;
        }
        _.pos = saved;
        return null;
    }

    /// Reads a variable-length XDR string (identical wire format to variable-length opaque).
    fn readString(*_): ([]const u8)? {
        return _.readOpaque();
    }

    /// Reads the `u32` element count of a variable-length XDR array.
    fn readArrayLen(*_): u32? {
        return _.readU32();
    }
}

// ---------------------------------------------------------------------------
// Zero-Alloc XDR Writer
// ---------------------------------------------------------------------------

/// Zero-allocation RFC 4506 XDR streaming encoder writing directly into a caller-supplied buffer.
pub struct XdrWriter {
    buf: []u8,
    pos: usize,

    pub fn init(buf: []u8): XdrWriter {
        return XdrWriter {
            buf: buf,
            pos: 0,
        };
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn writeU32(*_, v: u32): bool {
        if (_.remaining() < 4) {
            return false;
        }
        writeU32Be(_.buf, _.pos, v);
        _.pos += 4;
        return true;
    }

    fn writeI32(*_, v: i32): bool {
        const bits: u32 = @bitCast(v);
        return _.writeU32(bits);
    }

    fn writeU64(*_, v: u64): bool {
        if (_.remaining() < 8) {
            return false;
        }
        writeU64Be(_.buf, _.pos, v);
        _.pos += 8;
        return true;
    }

    fn writeI64(*_, v: i64): bool {
        const bits: u64 = @bitCast(v);
        return _.writeU64(bits);
    }

    fn writeBool(*_, v: bool): bool {
        const encoded: u32 = if (v) 1 else 0;
        return _.writeU32(encoded);
    }

    fn writeF32(*_, v: f32): bool {
        const bits: u32 = @bitCast(v);
        return _.writeU32(bits);
    }

    fn writeF64(*_, v: f64): bool {
        const bits: u64 = @bitCast(v);
        return _.writeU64(bits);
    }

    /// Writes `bytes` followed by `0..3` zero padding bytes to maintain 4-byte XDR alignment.
    fn writeFixedOpaque(*_, bytes: []const u8): bool {
        const pad = xdrPadLen(bytes.len);
        const rem = _.remaining();
        if (bytes.len > rem || pad > rem - bytes.len) {
            return false;
        }
        let i: usize = 0;
        while (i < bytes.len) {
            _.buf[_.pos + i] = bytes[i];
            i += 1;
        }
        let p: usize = 0;
        while (p < pad) {
            _.buf[_.pos + bytes.len + p] = 0;
            p += 1;
        }
        _.pos += bytes.len + pad;
        return true;
    }

    /// Writes a variable-length opaque field (`u32` length prefix + 4-byte-aligned bytes).
    fn writeOpaque(*_, bytes: []const u8): bool {
        if (bytes.len > 4294967295) {
            return false;
        }
        const pad = xdrPadLen(bytes.len);
        const rem = _.remaining();
        if (rem < 4 || bytes.len > rem - 4 || pad > (rem - 4) - bytes.len) {
            return false;
        }
        writeU32Be(_.buf, _.pos, bytes.len as u32);
        _.pos += 4;
        return _.writeFixedOpaque(bytes);
    }

    /// Writes a variable-length XDR string.
    fn writeString(*_, s: []const u8): bool {
        return _.writeOpaque(s);
    }

    /// Writes the `u32` element count of a variable-length XDR array.
    fn writeArrayLen(*_, n: u32): bool {
        return _.writeU32(n);
    }
}
