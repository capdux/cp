// @cap/cbor — Pure CP, zero-dependency, zero-heap-allocation RFC 8949 CBOR encoder & decoder
// Strictly follows RFC 8949 & Rust `minicbor` / `ciborium` semantics.
// Features:
//   - Big-endian wire encoding/decoding with zero heap allocation and zero `unsafe`
//   - Canonical shortest integer encoding (0..23 / 1B / 2B / 4B / 8B) for uint, negint (incl. i64 min), lengths, tags
//   - Zero-copy byte string (Major 2) & UTF-8 text string (Major 3) decoding
//   - Array (Major 4), Map (Major 5), Semantic Tag (Major 6)
//   - Simple values (false/true/null/undefined) & IEEE-754 f32/f64 (Major 7)
//   - Depth-guarded recursive `skipValue()` (`MAX_CBOR_DEPTH = 128`) with malformed-length precheck & cursor rollback

pub const MAX_CBOR_DEPTH: usize = 128;

pub err CborError {
    BufferTooSmall,
    UnexpectedEof,
    TypeMismatch,
    InvalidHeader,
    Overflow,
    UnsupportedIndefinite,
}

pub enum CborMajor: u8 {
    Uint = 0,
    NegInt = 1,
    Bytes = 2,
    Text = 3,
    Array = 4,
    Map = 5,
    Tag = 6,
    Simple = 7,
}

pub enum CborValueKind {
    Uint,
    NegInt,
    Bytes,
    Text,
    Array,
    Map,
    Tag,
    Bool,
    Null,
    Undefined,
    Float16,
    Float32,
    Float64,
    Simple,
}

const MAJOR_UINT: u8 = 0;
const MAJOR_NEGINT: u8 = 1;
const MAJOR_BYTES: u8 = 2;
const MAJOR_TEXT: u8 = 3;
const MAJOR_ARRAY: u8 = 4;
const MAJOR_MAP: u8 = 5;
const MAJOR_TAG: u8 = 6;
const MAJOR_SIMPLE: u8 = 7;

const AI_U8: u8 = 24;
const AI_U16: u8 = 25;
const AI_U32: u8 = 26;
const AI_U64: u8 = 27;
const AI_INDEFINITE: u8 = 31;

const SIMPLE_FALSE: u8 = 0xF4;
const SIMPLE_TRUE: u8 = 0xF5;
const SIMPLE_NULL: u8 = 0xF6;
const SIMPLE_UNDEFINED: u8 = 0xF7;
const SIMPLE_F32: u8 = 0xFA;
const SIMPLE_F64: u8 = 0xFB;

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

fn headerSizeForArg(val: u64): usize {
    if (val <= 23) {
        return 1;
    }
    if (val <= 0xFF) {
        return 2;
    }
    if (val <= 0xFFFF) {
        return 3;
    }
    if (val <= 0xFFFFFFFF) {
        return 5;
    }
    return 9;
}

fn majorFromCode(code: u8): CborMajor {
    if (code == MAJOR_UINT) {
        return CborMajor.Uint;
    }
    if (code == MAJOR_NEGINT) {
        return CborMajor.NegInt;
    }
    if (code == MAJOR_BYTES) {
        return CborMajor.Bytes;
    }
    if (code == MAJOR_TEXT) {
        return CborMajor.Text;
    }
    if (code == MAJOR_ARRAY) {
        return CborMajor.Array;
    }
    if (code == MAJOR_MAP) {
        return CborMajor.Map;
    }
    if (code == MAJOR_TAG) {
        return CborMajor.Tag;
    }
    return CborMajor.Simple;
}

pub struct CborEncoder {
    buf: []u8;
    pos: usize;

    static fn init(buf: []u8): CborEncoder {
        return CborEncoder{
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

    fn ensureSpace(needed: usize): void ! CborError {
        if (_.buf.len - _.pos < needed) {
            return CborError.BufferTooSmall;
        }
    }

    fn writeHeader(*_, major: u8, val: u64): void ! CborError {
        const prefix: u8 = (major << 5) & 0xE0;
        if (val <= 23) {
            _.ensureSpace(1)?;
            _.buf[_.pos] = prefix | (val as u8);
            _.pos += 1;
            return;
        }
        if (val <= 0xFF) {
            _.ensureSpace(2)?;
            _.buf[_.pos] = prefix | AI_U8;
            _.buf[_.pos + 1] = (val & 0xFF) as u8;
            _.pos += 2;
            return;
        }
        if (val <= 0xFFFF) {
            _.ensureSpace(3)?;
            _.buf[_.pos] = prefix | AI_U16;
            writeU16Be(_.buf, _.pos + 1, (val & 0xFFFF) as u16);
            _.pos += 3;
            return;
        }
        if (val <= 0xFFFFFFFF) {
            _.ensureSpace(5)?;
            _.buf[_.pos] = prefix | AI_U32;
            writeU32Be(_.buf, _.pos + 1, (val & 0xFFFFFFFF) as u32);
            _.pos += 5;
            return;
        }
        _.ensureSpace(9)?;
        _.buf[_.pos] = prefix | AI_U64;
        writeU64Be(_.buf, _.pos + 1, val);
        _.pos += 9;
    }

    fn writeUint(*_, val: u64): void ! CborError {
        return _.writeHeader(MAJOR_UINT, val);
    }

    fn writeInt(*_, val: i64): void ! CborError {
        if (val >= 0) {
            const uval: u64 = @as(u64, @bitCast(val));
            return _.writeHeader(MAJOR_UINT, uval);
        }
        // RFC 8949: Major 1 encodes -1 - n. In two's complement, ~bits(val) == -1 - val
        // without overflow even for i64::MIN (-9223372036854775808 -> n = 0x7FFFFFFFFFFFFFFF).
        const raw: u64 = @as(u64, @bitCast(val));
        const n: u64 = ~raw;
        return _.writeHeader(MAJOR_NEGINT, n);
    }

    fn writeBytes(*_, data: []const u8): void ! CborError {
        const len_u64: u64 = u64(data.len);
        const hdr_len = headerSizeForArg(len_u64);
        const rem = _.buf.len - _.pos;
        if (rem < hdr_len || rem - hdr_len < data.len) {
            return CborError.BufferTooSmall;
        }
        _.writeHeader(MAJOR_BYTES, len_u64)?;
        copyBytes(_.buf, _.pos, data);
        _.pos += data.len;
    }

    fn writeText(*_, text: []const u8): void ! CborError {
        const len_u64: u64 = u64(text.len);
        const hdr_len = headerSizeForArg(len_u64);
        const rem = _.buf.len - _.pos;
        if (rem < hdr_len || rem - hdr_len < text.len) {
            return CborError.BufferTooSmall;
        }
        _.writeHeader(MAJOR_TEXT, len_u64)?;
        copyBytes(_.buf, _.pos, text);
        _.pos += text.len;
    }

    fn writeArrayHeader(*_, count: u64): void ! CborError {
        return _.writeHeader(MAJOR_ARRAY, count);
    }

    fn writeMapHeader(*_, count: u64): void ! CborError {
        return _.writeHeader(MAJOR_MAP, count);
    }

    fn writeTag(*_, tag: u64): void ! CborError {
        return _.writeHeader(MAJOR_TAG, tag);
    }

    fn writeBool(*_, val: bool): void ! CborError {
        _.ensureSpace(1)?;
        if (val) {
            _.buf[_.pos] = SIMPLE_TRUE;
        } else {
            _.buf[_.pos] = SIMPLE_FALSE;
        }
        _.pos += 1;
    }

    fn writeNull(*_): void ! CborError {
        _.ensureSpace(1)?;
        _.buf[_.pos] = SIMPLE_NULL;
        _.pos += 1;
    }

    fn writeUndefined(*_): void ! CborError {
        _.ensureSpace(1)?;
        _.buf[_.pos] = SIMPLE_UNDEFINED;
        _.pos += 1;
    }

    fn writeF32(*_, val: f32): void ! CborError {
        _.ensureSpace(5)?;
        _.buf[_.pos] = SIMPLE_F32;
        writeU32Be(_.buf, _.pos + 1, @as(u32, @bitCast(val)));
        _.pos += 5;
    }

    fn writeF64(*_, val: f64): void ! CborError {
        _.ensureSpace(9)?;
        _.buf[_.pos] = SIMPLE_F64;
        writeU64Be(_.buf, _.pos + 1, @as(u64, @bitCast(val)));
        _.pos += 9;
    }
}

pub struct CborDecoder {
    data: []const u8;
    pos: usize;

    static fn init(data: []const u8): CborDecoder {
        return CborDecoder{
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn ensureAvail(needed: usize): void ! CborError {
        if (_.data.len - _.pos < needed) {
            return CborError.UnexpectedEof;
        }
    }

    fn peekInitialByte(): u8 ! CborError {
        if (_.pos >= _.data.len) {
            return CborError.UnexpectedEof;
        }
        return _.data[_.pos];
    }

    fn validateHeaderByte(ib: u8): void ! CborError {
        const major: u8 = ib >> 5;
        const ai: u8 = ib & 0x1F;
        if (ai >= 28 && ai <= 30) {
            return CborError.InvalidHeader;
        }
        if (ai == AI_INDEFINITE) {
            if (major == MAJOR_UINT || major == MAJOR_NEGINT || major == MAJOR_TAG) {
                return CborError.InvalidHeader;
            }
            return CborError.UnsupportedIndefinite;
        }
    }

    fn peekMajor(): CborMajor ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        return majorFromCode(ib >> 5);
    }

    fn peekKind(): CborValueKind ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        const major: u8 = ib >> 5;
        const ai: u8 = ib & 0x1F;
        if (major == MAJOR_UINT) {
            return CborValueKind.Uint;
        }
        if (major == MAJOR_NEGINT) {
            return CborValueKind.NegInt;
        }
        if (major == MAJOR_BYTES) {
            return CborValueKind.Bytes;
        }
        if (major == MAJOR_TEXT) {
            return CborValueKind.Text;
        }
        if (major == MAJOR_ARRAY) {
            return CborValueKind.Array;
        }
        if (major == MAJOR_MAP) {
            return CborValueKind.Map;
        }
        if (major == MAJOR_TAG) {
            return CborValueKind.Tag;
        }
        // Major 7: Simple values & floats
        if (ai == 20 || ai == 21) {
            return CborValueKind.Bool;
        }
        if (ai == 22) {
            return CborValueKind.Null;
        }
        if (ai == 23) {
            return CborValueKind.Undefined;
        }
        if (ai == 25) {
            return CborValueKind.Float16;
        }
        if (ai == 26) {
            return CborValueKind.Float32;
        }
        if (ai == 27) {
            return CborValueKind.Float64;
        }
        return CborValueKind.Simple;
    }

    fn readArg(*_, expected_major: u8): u64 ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        const major: u8 = ib >> 5;
        if (major != expected_major) {
            return CborError.TypeMismatch;
        }
        const ai: u8 = ib & 0x1F;
        if (ai <= 23) {
            _.pos += 1;
            return u64(ai);
        }
        if (ai == AI_U8) {
            _.ensureAvail(2)?;
            const v: u64 = u64(_.data[_.pos + 1]);
            _.pos += 2;
            return v;
        }
        if (ai == AI_U16) {
            _.ensureAvail(3)?;
            const v: u64 = u64(readU16Be(_.data, _.pos + 1));
            _.pos += 3;
            return v;
        }
        if (ai == AI_U32) {
            _.ensureAvail(5)?;
            const v: u64 = u64(readU32Be(_.data, _.pos + 1));
            _.pos += 5;
            return v;
        }
        if (ai == AI_U64) {
            _.ensureAvail(9)?;
            const v: u64 = readU64Be(_.data, _.pos + 1);
            _.pos += 9;
            return v;
        }
        return CborError.InvalidHeader;
    }

    fn readUint(*_): u64 ! CborError {
        return _.readArg(MAJOR_UINT);
    }

    fn readInt(*_): i64 ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        const major: u8 = ib >> 5;
        if (major == MAJOR_UINT) {
            const saved = _.pos;
            const v = _.readArg(MAJOR_UINT)?;
            if (v > MAX_I64_AS_U64) {
                _.pos = saved;
                return CborError.Overflow;
            }
            return @as(i64, @bitCast(v));
        }
        if (major == MAJOR_NEGINT) {
            const saved = _.pos;
            const n = _.readArg(MAJOR_NEGINT)?;
            if (n > MAX_I64_AS_U64) {
                _.pos = saved;
                return CborError.Overflow;
            }
            return @as(i64, @bitCast(~n));
        }
        return CborError.TypeMismatch;
    }

    fn readBytes(*_): []const u8 ! CborError {
        const saved = _.pos;
        const len_u64 = _.readArg(MAJOR_BYTES)?;
        if (len_u64 > u64(_.remaining())) {
            _.pos = saved;
            return CborError.UnexpectedEof;
        }
        const n: usize = len_u64 as usize;
        const start = _.pos;
        const end = start + n;
        _.pos = end;
        return _.data[start..end];
    }

    fn readText(*_): []const u8 ! CborError {
        const saved = _.pos;
        const len_u64 = _.readArg(MAJOR_TEXT)?;
        if (len_u64 > u64(_.remaining())) {
            _.pos = saved;
            return CborError.UnexpectedEof;
        }
        const n: usize = len_u64 as usize;
        const start = _.pos;
        const end = start + n;
        _.pos = end;
        return _.data[start..end];
    }

    fn readArrayHeader(*_): u64 ! CborError {
        return _.readArg(MAJOR_ARRAY);
    }

    fn readMapHeader(*_): u64 ! CborError {
        return _.readArg(MAJOR_MAP);
    }

    fn readTag(*_): u64 ! CborError {
        return _.readArg(MAJOR_TAG);
    }

    fn readBool(*_): bool ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        if (ib == SIMPLE_FALSE) {
            _.pos += 1;
            return false;
        }
        if (ib == SIMPLE_TRUE) {
            _.pos += 1;
            return true;
        }
        return CborError.TypeMismatch;
    }

    fn readNull(*_): void ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        if (ib == SIMPLE_NULL) {
            _.pos += 1;
            return;
        }
        return CborError.TypeMismatch;
    }

    fn readUndefined(*_): void ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        if (ib == SIMPLE_UNDEFINED) {
            _.pos += 1;
            return;
        }
        return CborError.TypeMismatch;
    }

    fn readF32(*_): f32 ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        if (ib != SIMPLE_F32) {
            return CborError.TypeMismatch;
        }
        _.ensureAvail(5)?;
        const bits = readU32Be(_.data, _.pos + 1);
        _.pos += 5;
        return @as(f32, @bitCast(bits));
    }

    fn readF64(*_): f64 ! CborError {
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        if (ib != SIMPLE_F64) {
            return CborError.TypeMismatch;
        }
        _.ensureAvail(9)?;
        const bits = readU64Be(_.data, _.pos + 1);
        _.pos += 9;
        return @as(f64, @bitCast(bits));
    }

    fn skipValueDepth(*_, depth: usize): void ! CborError {
        if (depth > MAX_CBOR_DEPTH) {
            return CborError.Overflow;
        }
        const ib = _.peekInitialByte()?;
        _.validateHeaderByte(ib)?;
        const major: u8 = ib >> 5;
        const ai: u8 = ib & 0x1F;

        if (major == MAJOR_UINT) {
            * = _.readArg(MAJOR_UINT)?;
            return;
        }
        if (major == MAJOR_NEGINT) {
            * = _.readArg(MAJOR_NEGINT)?;
            return;
        }
        if (major == MAJOR_BYTES) {
            * = _.readBytes()?;
            return;
        }
        if (major == MAJOR_TEXT) {
            * = _.readText()?;
            return;
        }
        if (major == MAJOR_ARRAY) {
            const count = _.readArg(MAJOR_ARRAY)?;
            if (count > u64(_.remaining())) {
                return CborError.UnexpectedEof;
            }
            let i: u64 = 0;
            while (i < count) : (i += 1) {
                _.skipValueDepth(depth + 1)?;
            }
            return;
        }
        if (major == MAJOR_MAP) {
            const count = _.readArg(MAJOR_MAP)?;
            if (count > u64(_.remaining() / 2)) {
                return CborError.UnexpectedEof;
            }
            let i: u64 = 0;
            while (i < count) : (i += 1) {
                _.skipValueDepth(depth + 1)?;
                _.skipValueDepth(depth + 1)?;
            }
            return;
        }
        if (major == MAJOR_TAG) {
            * = _.readArg(MAJOR_TAG)?;
            return _.skipValueDepth(depth + 1);
        }
        // Major 7: Simple values & IEEE-754 floats
        if (ai <= 23) {
            _.pos += 1;
            return;
        }
        if (ai == AI_U8) {
            _.ensureAvail(2)?;
            // RFC 8949 Section 3.3: 1-byte simple value MUST be >= 32
            if (_.data[_.pos + 1] < 32) {
                return CborError.InvalidHeader;
            }
            _.pos += 2;
            return;
        }
        if (ai == AI_U16) {
            _.ensureAvail(3)?;
            _.pos += 3;
            return;
        }
        if (ai == AI_U32) {
            _.ensureAvail(5)?;
            _.pos += 5;
            return;
        }
        if (ai == AI_U64) {
            _.ensureAvail(9)?;
            _.pos += 9;
            return;
        }
        return CborError.InvalidHeader;
    }

    fn skipValue(*_): void ! CborError {
        const saved_pos = _.pos;
        _.skipValueDepth(0) catch e => {
            _.pos = saved_pos;
            return e;
        };
    }
}
