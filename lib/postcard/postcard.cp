// Origin: Pure-CP Stdlib (Zero-Alloc Postcard Embedded Wire Codec + COBS) | Created: 2026-11-22

pub fn zigzagEncodeI16(v: i16): u16 {
    let u: u16 = 0;
    unsafe {
        u = @bitCast(v);
    }
    let mask: u16 = 0;
    if (v < 0) {
        mask = 0xFFFF;
    }
    return (u << 1) ^ mask;
}

pub fn zigzagDecodeI16(z: u16): i16 {
    const half: u16 = z >> 1;
    let mask: u16 = 0;
    if ((z & 1) != 0) {
        mask = 0xFFFF;
    }
    const raw: u16 = half ^ mask;
    unsafe {
        const v: i16 = @bitCast(raw);
        return v;
    }
}

pub fn zigzagEncodeI32(v: i32): u32 {
    let u: u32 = 0;
    unsafe {
        u = @bitCast(v);
    }
    let mask: u32 = 0;
    if (v < 0) {
        mask = 0xFFFF_FFFF;
    }
    return (u << 1) ^ mask;
}

pub fn zigzagDecodeI32(z: u32): i32 {
    const half: u32 = z >> 1;
    let mask: u32 = 0;
    if ((z & 1) != 0) {
        mask = 0xFFFF_FFFF;
    }
    const raw: u32 = half ^ mask;
    unsafe {
        const v: i32 = @bitCast(raw);
        return v;
    }
}

pub fn zigzagEncodeI64(v: i64): u64 {
    let u: u64 = 0;
    unsafe {
        u = @bitCast(v);
    }
    let mask: u64 = 0;
    if (v < 0) {
        mask = 0xFFFF_FFFF_FFFF_FFFF;
    }
    return (u << 1) ^ mask;
}

pub fn zigzagDecodeI64(z: u64): i64 {
    const half: u64 = z >> 1;
    let mask: u64 = 0;
    if ((z & 1) != 0) {
        mask = 0xFFFF_FFFF_FFFF_FFFF;
    }
    const raw: u64 = half ^ mask;
    unsafe {
        const v: i64 = @bitCast(raw);
        return v;
    }
}

pub struct PostcardWriter {
    buf: []u8,
    pos: usize,

    fn init(buf: []u8): PostcardWriter {
        return PostcardWriter{
            .buf = buf,
            .pos = 0,
        };
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeU8(*_, v: u8): bool {
        if (_.remaining() < 1) {
            return false;
        }
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeI8(*_, v: i8): bool {
        let bits: u8 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeU8(bits);
    }

    fn writeBool(*_, v: bool): bool {
        if (v) {
            return _.writeU8(1);
        }
        return _.writeU8(0);
    }

    fn writeVarU64(*_, val: u64): bool {
        const orig = _.pos;
        let n: u64 = val;
        while (true) {
            if (_.remaining() < 1) {
                _.pos = orig;
                return false;
            }
            let b: u8 = (n & 0x7F) as u8;
            n = n >> 7;
            if (n != 0) {
                b = b | 0x80;
            }
            _.buf[_.pos] = b;
            _.pos += 1;
            if (n == 0) {
                return true;
            }
        }
    }

    fn writeVarU16(*_, v: u16): bool {
        return _.writeVarU64(v as u64);
    }

    fn writeVarU32(*_, v: u32): bool {
        return _.writeVarU64(v as u64);
    }

    fn writeVarI16(*_, v: i16): bool {
        return _.writeVarU16(zigzagEncodeI16(v));
    }

    fn writeVarI32(*_, v: i32): bool {
        return _.writeVarU32(zigzagEncodeI32(v));
    }

    fn writeVarI64(*_, v: i64): bool {
        return _.writeVarU64(zigzagEncodeI64(v));
    }

    fn writeF32(*_, v: f32): bool {
        if (_.remaining() < 4) {
            return false;
        }
        let bits: u32 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = (bits & 0xFF) as u8;
        _.buf[_.pos + 1] = ((bits >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((bits >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((bits >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    fn writeF64(*_, v: f64): bool {
        if (_.remaining() < 8) {
            return false;
        }
        let bits: u64 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = (bits & 0xFF) as u8;
        _.buf[_.pos + 1] = ((bits >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((bits >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((bits >> 24) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((bits >> 32) & 0xFF) as u8;
        _.buf[_.pos + 5] = ((bits >> 40) & 0xFF) as u8;
        _.buf[_.pos + 6] = ((bits >> 48) & 0xFF) as u8;
        _.buf[_.pos + 7] = ((bits >> 56) & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    fn writeBytes(*_, data: []const u8): bool {
        const orig = _.pos;
        if (!_.writeVarU64(data.len as u64)) {
            _.pos = orig;
            return false;
        }
        if (data.len > _.remaining()) {
            _.pos = orig;
            return false;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i += 1;
        }
        _.pos += data.len;
        return true;
    }

    fn writeString(*_, s: []const u8): bool {
        return _.writeBytes(s);
    }

    fn writeOptionTag(*_, is_some: bool): bool {
        return _.writeBool(is_some);
    }

    fn writeVariantIdx(*_, idx: u32): bool {
        return _.writeVarU32(idx);
    }

    fn writeSeqLen(*_, len: usize): bool {
        return _.writeVarU64(len as u64);
    }
}

pub struct PostcardReader {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): PostcardReader {
        return PostcardReader{
            .data = data,
            .pos = 0,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn readU8(*_): u8? {
        if (_.remaining() < 1) {
            return null;
        }
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readI8(*_): i8? {
        const raw = _.readU8() ?? return null;
        unsafe {
            const v: i8 = @bitCast(raw);
            return v;
        }
    }

    fn readBool(*_): bool? {
        const raw = _.readU8() ?? return null;
        if (raw == 0) return false;
        if (raw == 1) return true;
        return null;
    }

    fn readVarU16(*_): u16? {
        let out: u16 = 0;
        let shift: u8 = 0;
        let count: u8 = 0;
        while (count < 3) {
            const b = _.readU8() ?? return null;
            if (count == 2 && b > 0x03) {
                return null;
            }
            const low: u16 = (b & 0x7F) as u16;
            const s: u4 = shift as u4;
            out = out | (low << s);
            if ((b & 0x80) == 0) {
                return out;
            }
            shift += 7;
            count += 1;
        }
        return null;
    }

    fn readVarU32(*_): u32? {
        let out: u32 = 0;
        let shift: u8 = 0;
        let count: u8 = 0;
        while (count < 5) {
            const b = _.readU8() ?? return null;
            if (count == 4 && b > 0x0F) {
                return null;
            }
            const low: u32 = (b & 0x7F) as u32;
            const s: u5 = shift as u5;
            out = out | (low << s);
            if ((b & 0x80) == 0) {
                return out;
            }
            shift += 7;
            count += 1;
        }
        return null;
    }

    fn readVarU64(*_): u64? {
        let out: u64 = 0;
        let shift: u8 = 0;
        let count: u8 = 0;
        while (count < 10) {
            const b = _.readU8() ?? return null;
            if (count == 9 && b > 0x01) {
                return null;
            }
            const low: u64 = (b & 0x7F) as u64;
            const s: u6 = shift as u6;
            out = out | (low << s);
            if ((b & 0x80) == 0) {
                return out;
            }
            shift += 7;
            count += 1;
        }
        return null;
    }

    fn readVarI16(*_): i16? {
        const z = _.readVarU16() ?? return null;
        return zigzagDecodeI16(z);
    }

    fn readVarI32(*_): i32? {
        const z = _.readVarU32() ?? return null;
        return zigzagDecodeI32(z);
    }

    fn readVarI64(*_): i64? {
        const z = _.readVarU64() ?? return null;
        return zigzagDecodeI64(z);
    }

    fn readF32(*_): f32? {
        if (_.remaining() < 4) {
            return null;
        }
        const bits: u32 = (_.data[_.pos] as u32) |
            ((_.data[_.pos + 1] as u32) << 8) |
            ((_.data[_.pos + 2] as u32) << 16) |
            ((_.data[_.pos + 3] as u32) << 24);
        _.pos += 4;
        unsafe {
            const v: f32 = @bitCast(bits);
            return v;
        }
    }

    fn readF64(*_): f64? {
        if (_.remaining() < 8) {
            return null;
        }
        const bits: u64 = (_.data[_.pos] as u64) |
            ((_.data[_.pos + 1] as u64) << 8) |
            ((_.data[_.pos + 2] as u64) << 16) |
            ((_.data[_.pos + 3] as u64) << 24) |
            ((_.data[_.pos + 4] as u64) << 32) |
            ((_.data[_.pos + 5] as u64) << 40) |
            ((_.data[_.pos + 6] as u64) << 48) |
            ((_.data[_.pos + 7] as u64) << 56);
        _.pos += 8;
        unsafe {
            const v: f64 = @bitCast(bits);
            return v;
        }
    }

    fn readBytes(*_): ([]const u8)? {
        const len_u64 = _.readVarU64() ?? return null;
        if (len_u64 > (_.remaining() as u64)) {
            return null;
        }
        const n: usize = len_u64 as usize;
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }

    fn readString(*_): ([]const u8)? {
        return _.readBytes();
    }

    fn readOptionTag(*_): bool? {
        return _.readBool();
    }

    fn readVariantIdx(*_): u32? {
        return _.readVarU32();
    }

    fn readSeqLen(*_): usize? {
        const v = _.readVarU64() ?? return null;
        return v as usize;
    }
}

// COBS encode without trailing 0x00 sentinel byte.
pub fn cobsEncode(src: []const u8, dst: []u8): usize? {
    if (dst.len < 1) {
        return null;
    }
    if (src.len == 0) {
        dst[0] = 1;
        return 1;
    }

    let code_idx: usize = 0;
    let write_idx: usize = 1;
    let code: u8 = 1;
    let i: usize = 0;

    while (i < src.len) {
        const b = src[i];
        if (b == 0) {
            dst[code_idx] = code;
            code_idx = write_idx;
            if (code_idx >= dst.len) {
                return null;
            }
            write_idx += 1;
            code = 1;
        } else {
            if (write_idx >= dst.len) {
                return null;
            }
            dst[write_idx] = b;
            write_idx += 1;
            code += 1;
            if (code == 0xFF) {
                dst[code_idx] = 0xFF;
                if (i + 1 < src.len) {
                    code_idx = write_idx;
                    if (code_idx >= dst.len) {
                        return null;
                    }
                    write_idx += 1;
                    code = 1;
                } else {
                    return write_idx;
                }
            }
        }
        i += 1;
    }

    dst[code_idx] = code;
    return write_idx;
}

// COBS encode with trailing 0x00 frame delimiter byte.
pub fn cobsEncodeFramed(src: []const u8, dst: []u8): usize? {
    const n = cobsEncode(src, dst) ?? return null;
    if (n >= dst.len) {
        return null;
    }
    dst[n] = 0;
    return n + 1;
}

// COBS decode (rejects embedded 0x00 bytes and out-of-bounds code jumps).
pub fn cobsDecode(src: []const u8, dst: []u8): usize? {
    if (src.len == 0) {
        return null;
    }
    let read_idx: usize = 0;
    let write_idx: usize = 0;

    while (read_idx < src.len) {
        const code = src[read_idx];
        if (code == 0) {
            return null;
        }
        read_idx += 1;
        const copy_len: usize = (code - 1) as usize;
        if (copy_len > src.len - read_idx) {
            return null;
        }
        if (copy_len > dst.len - write_idx) {
            return null;
        }
        let j: usize = 0;
        while (j < copy_len) {
            const b = src[read_idx + j];
            if (b == 0) {
                return null;
            }
            dst[write_idx + j] = b;
            j += 1;
        }
        read_idx += copy_len;
        write_idx += copy_len;

        if (code < 0xFF && read_idx < src.len) {
            if (write_idx >= dst.len) {
                return null;
            }
            dst[write_idx] = 0;
            write_idx += 1;
        }
    }

    return write_idx;
}
