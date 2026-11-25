// @cap/msgpack — Pure CP, zero-dependency, zero-heap-allocation MessagePack implementation
// Strictly follows the official MessagePack specification & Rust `rmp` crate semantics.
// Features:
//   - Big-endian wire encoding/decoding with zero heap allocation and zero `unsafe`
//   - Compact integer encoding (positive/negative fixint, uint8..64, int8..64)
//   - IEEE-754 float32 & float64, nil, bool
//   - Zero-copy str (fixstr/str8/str16/str32), bin (bin8/bin16/bin32), ext (fixext1..16 & ext8/16/32)
//   - Array & Map headers (fixarray/array16/array32, fixmap/map16/map32)
//   - Type inspection (`peekType`) & depth-guarded recursive `skipValue`

pub const MAX_SKIP_DEPTH: usize = 128;

pub err MsgPackError {
    BufferTooSmall,
    UnexpectedEof,
    TypeMismatch,
    InvalidMarker,
    Overflow,
}

pub enum MsgPackType {
    Nil,
    Bool,
    Int,
    Uint,
    Float32,
    Float64,
    Str,
    Bin,
    Array,
    Map,
    Ext,
}

pub struct MsgPackExt {
    type_code: i8;
    data: []const u8;
}

const MARKER_NIL: u8 = 0xC0;
const MARKER_UNUSED: u8 = 0xC1;
const MARKER_FALSE: u8 = 0xC2;
const MARKER_TRUE: u8 = 0xC3;
const MARKER_BIN8: u8 = 0xC4;
const MARKER_BIN16: u8 = 0xC5;
const MARKER_BIN32: u8 = 0xC6;
const MARKER_EXT8: u8 = 0xC7;
const MARKER_EXT16: u8 = 0xC8;
const MARKER_EXT32: u8 = 0xC9;
const MARKER_F32: u8 = 0xCA;
const MARKER_F64: u8 = 0xCB;
const MARKER_U8: u8 = 0xCC;
const MARKER_U16: u8 = 0xCD;
const MARKER_U32: u8 = 0xCE;
const MARKER_U64: u8 = 0xCF;
const MARKER_I8: u8 = 0xD0;
const MARKER_I16: u8 = 0xD1;
const MARKER_I32: u8 = 0xD2;
const MARKER_I64: u8 = 0xD3;
const MARKER_FIXEXT1: u8 = 0xD4;
const MARKER_FIXEXT2: u8 = 0xD5;
const MARKER_FIXEXT4: u8 = 0xD6;
const MARKER_FIXEXT8: u8 = 0xD7;
const MARKER_FIXEXT16: u8 = 0xD8;
const MARKER_STR8: u8 = 0xD9;
const MARKER_STR16: u8 = 0xDA;
const MARKER_STR32: u8 = 0xDB;
const MARKER_ARRAY16: u8 = 0xDC;
const MARKER_ARRAY32: u8 = 0xDD;
const MARKER_MAP16: u8 = 0xDE;
const MARKER_MAP32: u8 = 0xDF;

const MAX_U32_USIZE: usize = 0xFFFFFFFF;
const MAX_I64_AS_U64: u64 = 0x7FFFFFFFFFFFFFFF;

fn writeU16Be(buf: []u8, offset: usize, v: u16) {
    buf[offset] = ((v >> 8) & 0xFF) as u8;
    buf[offset + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, offset: usize, v: u32) {
    buf[offset] = ((v >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((v >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((v >> 8) & 0xFF) as u8;
    buf[offset + 3] = (v & 0xFF) as u8;
}

fn writeU64Be(buf: []u8, offset: usize, v: u64) {
    buf[offset] = ((v >> 56) & 0xFF) as u8;
    buf[offset + 1] = ((v >> 48) & 0xFF) as u8;
    buf[offset + 2] = ((v >> 40) & 0xFF) as u8;
    buf[offset + 3] = ((v >> 32) & 0xFF) as u8;
    buf[offset + 4] = ((v >> 24) & 0xFF) as u8;
    buf[offset + 5] = ((v >> 16) & 0xFF) as u8;
    buf[offset + 6] = ((v >> 8) & 0xFF) as u8;
    buf[offset + 7] = (v & 0xFF) as u8;
}

fn readU16Be(buf: []const u8, offset: usize): u16 {
    const b0: u16 = u16(buf[offset]);
    const b1: u16 = u16(buf[offset + 1]);
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, offset: usize): u32 {
    const b0: u32 = u32(buf[offset]);
    const b1: u32 = u32(buf[offset + 1]);
    const b2: u32 = u32(buf[offset + 2]);
    const b3: u32 = u32(buf[offset + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Be(buf: []const u8, offset: usize): u64 {
    const b0: u64 = u64(buf[offset]);
    const b1: u64 = u64(buf[offset + 1]);
    const b2: u64 = u64(buf[offset + 2]);
    const b3: u64 = u64(buf[offset + 3]);
    const b4: u64 = u64(buf[offset + 4]);
    const b5: u64 = u64(buf[offset + 5]);
    const b6: u64 = u64(buf[offset + 6]);
    const b7: u64 = u64(buf[offset + 7]);
    return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) |
           (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
}

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

pub struct MsgPackEncoder {
    buf: []u8;
    pos: usize;

    static fn init(buf: []u8): MsgPackEncoder {
        return MsgPackEncoder{
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

    fn reset(*_) {
        _.pos = 0;
    }

    fn ensureSpace(needed: usize): void ! MsgPackError {
        if (_.buf.len - _.pos < needed) {
            return MsgPackError.BufferTooSmall;
        }
    }

    fn writeNil(*_): void ! MsgPackError {
        _.ensureSpace(1)?;
        _.buf[_.pos] = MARKER_NIL;
        _.pos += 1;
    }

    fn writeBool(*_, val: bool): void ! MsgPackError {
        _.ensureSpace(1)?;
        if (val) {
            _.buf[_.pos] = MARKER_TRUE;
        } else {
            _.buf[_.pos] = MARKER_FALSE;
        }
        _.pos += 1;
    }

    fn writeUint(*_, val: u64): void ! MsgPackError {
        if (val <= 0x7F) {
            _.ensureSpace(1)?;
            _.buf[_.pos] = (val & 0x7F) as u8;
            _.pos += 1;
            return;
        }
        if (val <= 0xFF) {
            _.ensureSpace(2)?;
            _.buf[_.pos] = MARKER_U8;
            _.buf[_.pos + 1] = (val & 0xFF) as u8;
            _.pos += 2;
            return;
        }
        if (val <= 0xFFFF) {
            _.ensureSpace(3)?;
            _.buf[_.pos] = MARKER_U16;
            writeU16Be(_.buf, _.pos + 1, (val & 0xFFFF) as u16);
            _.pos += 3;
            return;
        }
        if (val <= 0xFFFFFFFF) {
            _.ensureSpace(5)?;
            _.buf[_.pos] = MARKER_U32;
            writeU32Be(_.buf, _.pos + 1, (val & 0xFFFFFFFF) as u32);
            _.pos += 5;
            return;
        }
        _.ensureSpace(9)?;
        _.buf[_.pos] = MARKER_U64;
        writeU64Be(_.buf, _.pos + 1, val);
        _.pos += 9;
    }

    fn writeInt(*_, val: i64): void ! MsgPackError {
        if (val >= 0) {
            const uval: u64 = @as(u64, @bitCast(val));
            return _.writeUint(uval);
        }
        if (val >= -32) {
            _.ensureSpace(1)?;
            const raw8: i8 = val as i8;
            _.buf[_.pos] = @as(u8, @bitCast(raw8));
            _.pos += 1;
            return;
        }
        if (val >= -128) {
            _.ensureSpace(2)?;
            const raw8: i8 = val as i8;
            _.buf[_.pos] = MARKER_I8;
            _.buf[_.pos + 1] = @as(u8, @bitCast(raw8));
            _.pos += 2;
            return;
        }
        if (val >= -32768) {
            _.ensureSpace(3)?;
            const raw16: i16 = val as i16;
            _.buf[_.pos] = MARKER_I16;
            writeU16Be(_.buf, _.pos + 1, @as(u16, @bitCast(raw16)));
            _.pos += 3;
            return;
        }
        if (val >= -2147483648) {
            _.ensureSpace(5)?;
            const raw32: i32 = val as i32;
            _.buf[_.pos] = MARKER_I32;
            writeU32Be(_.buf, _.pos + 1, @as(u32, @bitCast(raw32)));
            _.pos += 5;
            return;
        }
        _.ensureSpace(9)?;
        _.buf[_.pos] = MARKER_I64;
        writeU64Be(_.buf, _.pos + 1, @as(u64, @bitCast(val)));
        _.pos += 9;
    }

    fn writeF32(*_, val: f32): void ! MsgPackError {
        _.ensureSpace(5)?;
        _.buf[_.pos] = MARKER_F32;
        writeU32Be(_.buf, _.pos + 1, @as(u32, @bitCast(val)));
        _.pos += 5;
    }

    fn writeF64(*_, val: f64): void ! MsgPackError {
        _.ensureSpace(9)?;
        _.buf[_.pos] = MARKER_F64;
        writeU64Be(_.buf, _.pos + 1, @as(u64, @bitCast(val)));
        _.pos += 9;
    }

    fn writeStr(*_, s: []const u8): void ! MsgPackError {
        const n = s.len;
        if (n > MAX_U32_USIZE) {
            return MsgPackError.Overflow;
        }
        if (n <= 31) {
            if (_.buf.len - _.pos < 1 || _.buf.len - _.pos - 1 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = (0xA0 | (n as u8)) as u8;
            copyBytes(_.buf, _.pos + 1, s);
            _.pos += 1 + n;
            return;
        }
        if (n <= 0xFF) {
            if (_.buf.len - _.pos < 2 || _.buf.len - _.pos - 2 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_STR8;
            _.buf[_.pos + 1] = n as u8;
            copyBytes(_.buf, _.pos + 2, s);
            _.pos += 2 + n;
            return;
        }
        if (n <= 0xFFFF) {
            if (_.buf.len - _.pos < 3 || _.buf.len - _.pos - 3 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_STR16;
            writeU16Be(_.buf, _.pos + 1, n as u16);
            copyBytes(_.buf, _.pos + 3, s);
            _.pos += 3 + n;
            return;
        }
        if (_.buf.len - _.pos < 5 || _.buf.len - _.pos - 5 < n) {
            return MsgPackError.BufferTooSmall;
        }
        _.buf[_.pos] = MARKER_STR32;
        writeU32Be(_.buf, _.pos + 1, n as u32);
        copyBytes(_.buf, _.pos + 5, s);
        _.pos += 5 + n;
    }

    fn writeBin(*_, data: []const u8): void ! MsgPackError {
        const n = data.len;
        if (n > MAX_U32_USIZE) {
            return MsgPackError.Overflow;
        }
        if (n <= 0xFF) {
            if (_.buf.len - _.pos < 2 || _.buf.len - _.pos - 2 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_BIN8;
            _.buf[_.pos + 1] = n as u8;
            copyBytes(_.buf, _.pos + 2, data);
            _.pos += 2 + n;
            return;
        }
        if (n <= 0xFFFF) {
            if (_.buf.len - _.pos < 3 || _.buf.len - _.pos - 3 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_BIN16;
            writeU16Be(_.buf, _.pos + 1, n as u16);
            copyBytes(_.buf, _.pos + 3, data);
            _.pos += 3 + n;
            return;
        }
        if (_.buf.len - _.pos < 5 || _.buf.len - _.pos - 5 < n) {
            return MsgPackError.BufferTooSmall;
        }
        _.buf[_.pos] = MARKER_BIN32;
        writeU32Be(_.buf, _.pos + 1, n as u32);
        copyBytes(_.buf, _.pos + 5, data);
        _.pos += 5 + n;
    }

    fn writeArrayHeader(*_, count: u32): void ! MsgPackError {
        if (count <= 15) {
            _.ensureSpace(1)?;
            _.buf[_.pos] = (0x90 | (count as u8)) as u8;
            _.pos += 1;
            return;
        }
        if (count <= 0xFFFF) {
            _.ensureSpace(3)?;
            _.buf[_.pos] = MARKER_ARRAY16;
            writeU16Be(_.buf, _.pos + 1, count as u16);
            _.pos += 3;
            return;
        }
        _.ensureSpace(5)?;
        _.buf[_.pos] = MARKER_ARRAY32;
        writeU32Be(_.buf, _.pos + 1, count);
        _.pos += 5;
    }

    fn writeMapHeader(*_, count: u32): void ! MsgPackError {
        if (count <= 15) {
            _.ensureSpace(1)?;
            _.buf[_.pos] = (0x80 | (count as u8)) as u8;
            _.pos += 1;
            return;
        }
        if (count <= 0xFFFF) {
            _.ensureSpace(3)?;
            _.buf[_.pos] = MARKER_MAP16;
            writeU16Be(_.buf, _.pos + 1, count as u16);
            _.pos += 3;
            return;
        }
        _.ensureSpace(5)?;
        _.buf[_.pos] = MARKER_MAP32;
        writeU32Be(_.buf, _.pos + 1, count);
        _.pos += 5;
    }

    fn writeExt(*_, type_code: i8, data: []const u8): void ! MsgPackError {
        const n = data.len;
        if (n > MAX_U32_USIZE) {
            return MsgPackError.Overflow;
        }
        const type_byte: u8 = @as(u8, @bitCast(type_code));

        if (n == 1) {
            _.ensureSpace(3)?;
            _.buf[_.pos] = MARKER_FIXEXT1;
            _.buf[_.pos + 1] = type_byte;
            _.buf[_.pos + 2] = data[0];
            _.pos += 3;
            return;
        }
        if (n == 2) {
            _.ensureSpace(4)?;
            _.buf[_.pos] = MARKER_FIXEXT2;
            _.buf[_.pos + 1] = type_byte;
            copyBytes(_.buf, _.pos + 2, data);
            _.pos += 4;
            return;
        }
        if (n == 4) {
            _.ensureSpace(6)?;
            _.buf[_.pos] = MARKER_FIXEXT4;
            _.buf[_.pos + 1] = type_byte;
            copyBytes(_.buf, _.pos + 2, data);
            _.pos += 6;
            return;
        }
        if (n == 8) {
            _.ensureSpace(10)?;
            _.buf[_.pos] = MARKER_FIXEXT8;
            _.buf[_.pos + 1] = type_byte;
            copyBytes(_.buf, _.pos + 2, data);
            _.pos += 10;
            return;
        }
        if (n == 16) {
            _.ensureSpace(18)?;
            _.buf[_.pos] = MARKER_FIXEXT16;
            _.buf[_.pos + 1] = type_byte;
            copyBytes(_.buf, _.pos + 2, data);
            _.pos += 18;
            return;
        }
        if (n <= 0xFF) {
            if (_.buf.len - _.pos < 3 || _.buf.len - _.pos - 3 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_EXT8;
            _.buf[_.pos + 1] = n as u8;
            _.buf[_.pos + 2] = type_byte;
            copyBytes(_.buf, _.pos + 3, data);
            _.pos += 3 + n;
            return;
        }
        if (n <= 0xFFFF) {
            if (_.buf.len - _.pos < 4 || _.buf.len - _.pos - 4 < n) {
                return MsgPackError.BufferTooSmall;
            }
            _.buf[_.pos] = MARKER_EXT16;
            writeU16Be(_.buf, _.pos + 1, n as u16);
            _.buf[_.pos + 3] = type_byte;
            copyBytes(_.buf, _.pos + 4, data);
            _.pos += 4 + n;
            return;
        }
        if (_.buf.len - _.pos < 6 || _.buf.len - _.pos - 6 < n) {
            return MsgPackError.BufferTooSmall;
        }
        _.buf[_.pos] = MARKER_EXT32;
        writeU32Be(_.buf, _.pos + 1, n as u32);
        _.buf[_.pos + 5] = type_byte;
        copyBytes(_.buf, _.pos + 6, data);
        _.pos += 6 + n;
    }
}

pub struct MsgPackDecoder {
    data: []const u8;
    pos: usize;

    static fn init(data: []const u8): MsgPackDecoder {
        return MsgPackDecoder{
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn peekMarker(): u8 ! MsgPackError {
        if (_.pos >= _.data.len) {
            return MsgPackError.UnexpectedEof;
        }
        return _.data[_.pos];
    }

    fn peekType(): MsgPackType ! MsgPackError {
        const m = _.peekMarker()?;
        if (m <= 0x7F) {
            return MsgPackType.Uint;
        }
        if (m <= 0x8F) {
            return MsgPackType.Map;
        }
        if (m <= 0x9F) {
            return MsgPackType.Array;
        }
        if (m <= 0xBF) {
            return MsgPackType.Str;
        }
        if (m >= 0xE0) {
            return MsgPackType.Int;
        }
        switch (m) {
            0xC0 => return MsgPackType.Nil,
            0xC1 => return MsgPackError.InvalidMarker,
            0xC2 | 0xC3 => return MsgPackType.Bool,
            0xC4 | 0xC5 | 0xC6 => return MsgPackType.Bin,
            0xC7 | 0xC8 | 0xC9 | 0xD4 | 0xD5 | 0xD6 | 0xD7 | 0xD8 => return MsgPackType.Ext,
            0xCA => return MsgPackType.Float32,
            0xCB => return MsgPackType.Float64,
            0xCC | 0xCD | 0xCE | 0xCF => return MsgPackType.Uint,
            0xD0 | 0xD1 | 0xD2 | 0xD3 => return MsgPackType.Int,
            0xD9 | 0xDA | 0xDB => return MsgPackType.Str,
            0xDC | 0xDD => return MsgPackType.Array,
            0xDE | 0xDF => return MsgPackType.Map,
            else => return MsgPackError.InvalidMarker,
        }
    }

    fn ensureAvail(needed: usize): void ! MsgPackError {
        if (_.data.len - _.pos < needed) {
            return MsgPackError.UnexpectedEof;
        }
    }

    fn readNil(*_): void ! MsgPackError {
        const m = _.peekMarker()?;
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        if (m != MARKER_NIL) {
            return MsgPackError.TypeMismatch;
        }
        _.pos += 1;
    }

    fn readBool(*_): bool ! MsgPackError {
        const m = _.peekMarker()?;
        if (m == MARKER_FALSE) {
            _.pos += 1;
            return false;
        }
        if (m == MARKER_TRUE) {
            _.pos += 1;
            return true;
        }
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        return MsgPackError.TypeMismatch;
    }

    fn readUint(*_): u64 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m <= 0x7F) {
            _.pos += 1;
            return u64(m);
        }
        if (m >= 0xE0) {
            return MsgPackError.Overflow;
        }
        if (m == MARKER_U8) {
            _.ensureAvail(2)?;
            const v: u64 = u64(_.data[_.pos + 1]);
            _.pos += 2;
            return v;
        }
        if (m == MARKER_U16) {
            _.ensureAvail(3)?;
            const v: u64 = u64(readU16Be(_.data, _.pos + 1));
            _.pos += 3;
            return v;
        }
        if (m == MARKER_U32) {
            _.ensureAvail(5)?;
            const v: u64 = u64(readU32Be(_.data, _.pos + 1));
            _.pos += 5;
            return v;
        }
        if (m == MARKER_U64) {
            _.ensureAvail(9)?;
            const v: u64 = readU64Be(_.data, _.pos + 1);
            _.pos += 9;
            return v;
        }
        if (m == MARKER_I8) {
            _.ensureAvail(2)?;
            const v: i8 = @as(i8, @bitCast(_.data[_.pos + 1]));
            if (v < 0) {
                return MsgPackError.Overflow;
            }
            _.pos += 2;
            return u64(v as u8);
        }
        if (m == MARKER_I16) {
            _.ensureAvail(3)?;
            const raw: u16 = readU16Be(_.data, _.pos + 1);
            const v: i16 = @as(i16, @bitCast(raw));
            if (v < 0) {
                return MsgPackError.Overflow;
            }
            _.pos += 3;
            return u64(raw);
        }
        if (m == MARKER_I32) {
            _.ensureAvail(5)?;
            const raw: u32 = readU32Be(_.data, _.pos + 1);
            const v: i32 = @as(i32, @bitCast(raw));
            if (v < 0) {
                return MsgPackError.Overflow;
            }
            _.pos += 5;
            return u64(raw);
        }
        if (m == MARKER_I64) {
            _.ensureAvail(9)?;
            const raw: u64 = readU64Be(_.data, _.pos + 1);
            const v: i64 = @as(i64, @bitCast(raw));
            if (v < 0) {
                return MsgPackError.Overflow;
            }
            _.pos += 9;
            return raw;
        }
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        return MsgPackError.TypeMismatch;
    }

    fn readInt(*_): i64 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m <= 0x7F) {
            _.pos += 1;
            return i64(m);
        }
        if (m >= 0xE0) {
            _.pos += 1;
            const v: i8 = @as(i8, @bitCast(m));
            return i64(v);
        }
        if (m == MARKER_I8) {
            _.ensureAvail(2)?;
            const v: i8 = @as(i8, @bitCast(_.data[_.pos + 1]));
            _.pos += 2;
            return i64(v);
        }
        if (m == MARKER_I16) {
            _.ensureAvail(3)?;
            const v: i16 = @as(i16, @bitCast(readU16Be(_.data, _.pos + 1)));
            _.pos += 3;
            return i64(v);
        }
        if (m == MARKER_I32) {
            _.ensureAvail(5)?;
            const v: i32 = @as(i32, @bitCast(readU32Be(_.data, _.pos + 1)));
            _.pos += 5;
            return i64(v);
        }
        if (m == MARKER_I64) {
            _.ensureAvail(9)?;
            const v: i64 = @as(i64, @bitCast(readU64Be(_.data, _.pos + 1)));
            _.pos += 9;
            return v;
        }
        if (m == MARKER_U8) {
            _.ensureAvail(2)?;
            const v: i64 = i64(_.data[_.pos + 1]);
            _.pos += 2;
            return v;
        }
        if (m == MARKER_U16) {
            _.ensureAvail(3)?;
            const v: i64 = i64(readU16Be(_.data, _.pos + 1));
            _.pos += 3;
            return v;
        }
        if (m == MARKER_U32) {
            _.ensureAvail(5)?;
            const v: i64 = i64(readU32Be(_.data, _.pos + 1));
            _.pos += 5;
            return v;
        }
        if (m == MARKER_U64) {
            _.ensureAvail(9)?;
            const raw: u64 = readU64Be(_.data, _.pos + 1);
            if (raw > MAX_I64_AS_U64) {
                return MsgPackError.Overflow;
            }
            _.pos += 9;
            return @as(i64, @bitCast(raw));
        }
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        return MsgPackError.TypeMismatch;
    }

    fn readF32(*_): f32 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        if (m != MARKER_F32) {
            return MsgPackError.TypeMismatch;
        }
        _.ensureAvail(5)?;
        const raw: u32 = readU32Be(_.data, _.pos + 1);
        _.pos += 5;
        return @as(f32, @bitCast(raw));
    }

    fn readF64(*_): f64 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        if (m != MARKER_F64) {
            return MsgPackError.TypeMismatch;
        }
        _.ensureAvail(9)?;
        const raw: u64 = readU64Be(_.data, _.pos + 1);
        _.pos += 9;
        return @as(f64, @bitCast(raw));
    }

    fn readStr(*_): []const u8 ! MsgPackError {
        const m = _.peekMarker()?;
        let header_len: usize = 0;
        let payload_len: usize = 0;

        if (m >= 0xA0 && m <= 0xBF) {
            header_len = 1;
            payload_len = usize(m & 0x1F);
        } else if (m == MARKER_STR8) {
            _.ensureAvail(2)?;
            header_len = 2;
            payload_len = usize(_.data[_.pos + 1]);
        } else if (m == MARKER_STR16) {
            _.ensureAvail(3)?;
            header_len = 3;
            payload_len = usize(readU16Be(_.data, _.pos + 1));
        } else if (m == MARKER_STR32) {
            _.ensureAvail(5)?;
            header_len = 5;
            payload_len = usize(readU32Be(_.data, _.pos + 1));
        } else if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        } else {
            return MsgPackError.TypeMismatch;
        }

        const rem = _.data.len - _.pos;
        if (rem < header_len || rem - header_len < payload_len) {
            return MsgPackError.UnexpectedEof;
        }
        const start = _.pos + header_len;
        const end = start + payload_len;
        _.pos = end;
        return _.data[start..end];
    }

    fn readBin(*_): []const u8 ! MsgPackError {
        const m = _.peekMarker()?;
        let header_len: usize = 0;
        let payload_len: usize = 0;

        if (m == MARKER_BIN8) {
            _.ensureAvail(2)?;
            header_len = 2;
            payload_len = usize(_.data[_.pos + 1]);
        } else if (m == MARKER_BIN16) {
            _.ensureAvail(3)?;
            header_len = 3;
            payload_len = usize(readU16Be(_.data, _.pos + 1));
        } else if (m == MARKER_BIN32) {
            _.ensureAvail(5)?;
            header_len = 5;
            payload_len = usize(readU32Be(_.data, _.pos + 1));
        } else if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        } else {
            return MsgPackError.TypeMismatch;
        }

        const rem = _.data.len - _.pos;
        if (rem < header_len || rem - header_len < payload_len) {
            return MsgPackError.UnexpectedEof;
        }
        const start = _.pos + header_len;
        const end = start + payload_len;
        _.pos = end;
        return _.data[start..end];
    }

    fn readArrayHeader(*_): u32 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m >= 0x90 && m <= 0x9F) {
            _.pos += 1;
            return u32(m & 0x0F);
        }
        if (m == MARKER_ARRAY16) {
            _.ensureAvail(3)?;
            const n: u32 = u32(readU16Be(_.data, _.pos + 1));
            _.pos += 3;
            return n;
        }
        if (m == MARKER_ARRAY32) {
            _.ensureAvail(5)?;
            const n: u32 = readU32Be(_.data, _.pos + 1);
            _.pos += 5;
            return n;
        }
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        return MsgPackError.TypeMismatch;
    }

    fn readMapHeader(*_): u32 ! MsgPackError {
        const m = _.peekMarker()?;
        if (m >= 0x80 && m <= 0x8F) {
            _.pos += 1;
            return u32(m & 0x0F);
        }
        if (m == MARKER_MAP16) {
            _.ensureAvail(3)?;
            const n: u32 = u32(readU16Be(_.data, _.pos + 1));
            _.pos += 3;
            return n;
        }
        if (m == MARKER_MAP32) {
            _.ensureAvail(5)?;
            const n: u32 = readU32Be(_.data, _.pos + 1);
            _.pos += 5;
            return n;
        }
        if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        }
        return MsgPackError.TypeMismatch;
    }

    fn readExt(*_): MsgPackExt ! MsgPackError {
        const m = _.peekMarker()?;
        let header_len: usize = 0;
        let payload_len: usize = 0;
        let type_offset: usize = 0;

        if (m == MARKER_FIXEXT1) {
            header_len = 2;
            payload_len = 1;
            type_offset = 1;
        } else if (m == MARKER_FIXEXT2) {
            header_len = 2;
            payload_len = 2;
            type_offset = 1;
        } else if (m == MARKER_FIXEXT4) {
            header_len = 2;
            payload_len = 4;
            type_offset = 1;
        } else if (m == MARKER_FIXEXT8) {
            header_len = 2;
            payload_len = 8;
            type_offset = 1;
        } else if (m == MARKER_FIXEXT16) {
            header_len = 2;
            payload_len = 16;
            type_offset = 1;
        } else if (m == MARKER_EXT8) {
            _.ensureAvail(3)?;
            header_len = 3;
            payload_len = usize(_.data[_.pos + 1]);
            type_offset = 2;
        } else if (m == MARKER_EXT16) {
            _.ensureAvail(4)?;
            header_len = 4;
            payload_len = usize(readU16Be(_.data, _.pos + 1));
            type_offset = 3;
        } else if (m == MARKER_EXT32) {
            _.ensureAvail(6)?;
            header_len = 6;
            payload_len = usize(readU32Be(_.data, _.pos + 1));
            type_offset = 5;
        } else if (m == MARKER_UNUSED) {
            return MsgPackError.InvalidMarker;
        } else {
            return MsgPackError.TypeMismatch;
        }

        const rem = _.data.len - _.pos;
        if (rem < header_len || rem - header_len < payload_len) {
            return MsgPackError.UnexpectedEof;
        }
        const type_code: i8 = @as(i8, @bitCast(_.data[_.pos + type_offset]));
        const start = _.pos + header_len;
        const end = start + payload_len;
        _.pos = end;
        return MsgPackExt{
            type_code: type_code,
            data: _.data[start..end],
        };
    }

    fn skipValueDepth(*_, depth: usize): void ! MsgPackError {
        if (depth > MAX_SKIP_DEPTH) {
            return MsgPackError.Overflow;
        }
        const kind = _.peekType()?;
        switch (kind) {
            .Nil => {
                _.readNil()?;
            },
            .Bool => {
                * = _.readBool()?;
            },
            .Uint => {
                * = _.readUint()?;
            },
            .Int => {
                * = _.readInt()?;
            },
            .Float32 => {
                * = _.readF32()?;
            },
            .Float64 => {
                * = _.readF64()?;
            },
            .Str => {
                * = _.readStr()?;
            },
            .Bin => {
                * = _.readBin()?;
            },
            .Ext => {
                * = _.readExt()?;
            },
            .Array => {
                const count = _.readArrayHeader()?;
                if (usize(count) > _.remaining()) {
                    return MsgPackError.UnexpectedEof;
                }
                let i: u32 = 0;
                while (i < count) : (i += 1) {
                    _.skipValueDepth(depth + 1)?;
                }
            },
            .Map => {
                const count = _.readMapHeader()?;
                if (usize(count) > _.remaining() / 2) {
                    return MsgPackError.UnexpectedEof;
                }
                let i: u32 = 0;
                while (i < count) : (i += 1) {
                    _.skipValueDepth(depth + 1)?;
                    _.skipValueDepth(depth + 1)?;
                }
            },
        }
    }

    fn skipValue(*_): void ! MsgPackError {
        const saved_pos = _.pos;
        _.skipValueDepth(0) catch e => {
            _.pos = saved_pos;
            return e;
        };
    }
}
