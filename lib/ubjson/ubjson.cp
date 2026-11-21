// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc UBJSON Draft 12 Binary Codec)

pub const UBJ_NULL: u8 = 'Z';
pub const UBJ_NOOP: u8 = 'N';
pub const UBJ_TRUE: u8 = 'T';
pub const UBJ_FALSE: u8 = 'F';
pub const UBJ_INT8: u8 = 'i';
pub const UBJ_UINT8: u8 = 'U';
pub const UBJ_INT16: u8 = 'I';
pub const UBJ_INT32: u8 = 'l';
pub const UBJ_INT64: u8 = 'L';
pub const UBJ_FLOAT32: u8 = 'd';
pub const UBJ_FLOAT64: u8 = 'D';
pub const UBJ_CHAR: u8 = 'C';
pub const UBJ_STRING: u8 = 'S';
pub const UBJ_HIGH_PREC: u8 = 'H';
pub const UBJ_ARRAY_START: u8 = '[';
pub const UBJ_ARRAY_END: u8 = ']';
pub const UBJ_OBJECT_START: u8 = '{';
pub const UBJ_OBJECT_END: u8 = '}';
pub const UBJ_OPT_TYPE: u8 = '$';
pub const UBJ_OPT_COUNT: u8 = '#';

pub const MAX_UBJ_DEPTH: u32 = 64;

pub struct UbjsonContainerHeader {
    is_object: bool,
    elem_type: u8?,
    count: u64?,
}

fn isValidValueMarker(m: u8): bool {
    return m == UBJ_NULL ||
        m == UBJ_NOOP ||
        m == UBJ_TRUE ||
        m == UBJ_FALSE ||
        m == UBJ_INT8 ||
        m == UBJ_UINT8 ||
        m == UBJ_INT16 ||
        m == UBJ_INT32 ||
        m == UBJ_INT64 ||
        m == UBJ_FLOAT32 ||
        m == UBJ_FLOAT64 ||
        m == UBJ_CHAR ||
        m == UBJ_STRING ||
        m == UBJ_HIGH_PREC ||
        m == UBJ_ARRAY_START ||
        m == UBJ_OBJECT_START;
}

pub struct UbjsonWriter {
    buf: []u8,
    pos: usize,

    fn init(buf: []u8): UbjsonWriter {
        return UbjsonWriter{
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

    fn writeRawU8(*_, v: u8): bool {
        if (_.remaining() < 1) {
            return false;
        }
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeRawU16BE(*_, v: u16): bool {
        if (_.remaining() < 2) {
            return false;
        }
        _.buf[_.pos] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 1] = (v & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    fn writeRawU32BE(*_, v: u32): bool {
        if (_.remaining() < 4) {
            return false;
        }
        _.buf[_.pos] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 3] = (v & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    fn writeRawU64BE(*_, v: u64): bool {
        if (_.remaining() < 8) {
            return false;
        }
        _.buf[_.pos] = ((v >> 56) & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 48) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 40) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 32) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.pos + 5] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 6] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 7] = (v & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    fn writeRawI32BE(*_, v: i32): bool {
        let bits: u32 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeRawU32BE(bits);
    }

    fn writeRawBytes(*_, data: []const u8): bool {
        if (data.len > _.remaining()) {
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

    fn writeNull(*_): bool {
        return _.writeRawU8(UBJ_NULL);
    }

    fn writeNoop(*_): bool {
        return _.writeRawU8(UBJ_NOOP);
    }

    fn writeBool(*_, v: bool): bool {
        if (v) {
            return _.writeRawU8(UBJ_TRUE);
        }
        return _.writeRawU8(UBJ_FALSE);
    }

    fn writeInt8(*_, v: i8): bool {
        if (_.remaining() < 2) {
            return false;
        }
        let bits: u8 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = UBJ_INT8;
        _.buf[_.pos + 1] = bits;
        _.pos += 2;
        return true;
    }

    fn writeUint8(*_, v: u8): bool {
        if (_.remaining() < 2) {
            return false;
        }
        _.buf[_.pos] = UBJ_UINT8;
        _.buf[_.pos + 1] = v;
        _.pos += 2;
        return true;
    }

    fn writeInt16(*_, v: i16): bool {
        if (_.remaining() < 3) {
            return false;
        }
        let bits: u16 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = UBJ_INT16;
        _.pos += 1;
        return _.writeRawU16BE(bits);
    }

    fn writeInt32(*_, v: i32): bool {
        if (_.remaining() < 5) {
            return false;
        }
        _.buf[_.pos] = UBJ_INT32;
        _.pos += 1;
        return _.writeRawI32BE(v);
    }

    fn writeInt64(*_, v: i64): bool {
        if (_.remaining() < 9) {
            return false;
        }
        let bits: u64 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = UBJ_INT64;
        _.pos += 1;
        return _.writeRawU64BE(bits);
    }

    fn writeCompactInt(*_, v: i64): bool {
        if (v >= 0 && v <= 255) {
            return _.writeUint8(v as u8);
        }
        if (v >= -128 && v <= 127) {
            return _.writeInt8(v as i8);
        }
        if (v >= -32768 && v <= 32767) {
            return _.writeInt16(v as i16);
        }
        if (v >= -2147483648 && v <= 2147483647) {
            return _.writeInt32(v as i32);
        }
        return _.writeInt64(v);
    }

    fn writeFloat32(*_, v: f32): bool {
        if (_.remaining() < 5) {
            return false;
        }
        let bits: u32 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = UBJ_FLOAT32;
        _.pos += 1;
        return _.writeRawU32BE(bits);
    }

    fn writeFloat64(*_, v: f64): bool {
        if (_.remaining() < 9) {
            return false;
        }
        let bits: u64 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        _.buf[_.pos] = UBJ_FLOAT64;
        _.pos += 1;
        return _.writeRawU64BE(bits);
    }

    fn writeChar(*_, ch: u8): bool {
        if (ch > 127 || _.remaining() < 2) {
            return false;
        }
        _.buf[_.pos] = UBJ_CHAR;
        _.buf[_.pos + 1] = ch;
        _.pos += 2;
        return true;
    }

    fn writeKey(*_, key: []const u8): bool {
        const orig = _.pos;
        if (!_.writeCompactInt(key.len as i64)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeRawBytes(key)) {
            _.pos = orig;
            return false;
        }
        return true;
    }

    fn writeString(*_, s: []const u8): bool {
        const orig = _.pos;
        if (!_.writeRawU8(UBJ_STRING)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeKey(s)) {
            _.pos = orig;
            return false;
        }
        return true;
    }

    fn writeArrayStart(*_): bool {
        return _.writeRawU8(UBJ_ARRAY_START);
    }

    fn writeArrayEnd(*_): bool {
        return _.writeRawU8(UBJ_ARRAY_END);
    }

    fn writeTypedArrayHeader(*_, elem_marker: u8, count: u64): bool {
        if (!isValidValueMarker(elem_marker) || count > 0x7FFF_FFFF_FFFF_FFFF) {
            return false;
        }
        const orig = _.pos;
        if (!_.writeRawU8(UBJ_ARRAY_START)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeRawU8(UBJ_OPT_TYPE)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeRawU8(elem_marker)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeRawU8(UBJ_OPT_COUNT)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeCompactInt(count as i64)) {
            _.pos = orig;
            return false;
        }
        return true;
    }

    fn writeObjectStart(*_): bool {
        return _.writeRawU8(UBJ_OBJECT_START);
    }

    fn writeObjectEnd(*_): bool {
        return _.writeRawU8(UBJ_OBJECT_END);
    }
}

pub struct UbjsonReader {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): UbjsonReader {
        return UbjsonReader{
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

    fn skipNoops(*_) {
        while (_.pos < _.data.len && _.data[_.pos] == UBJ_NOOP) {
            _.pos += 1;
        }
    }

    fn peekMarker(*_): u8? {
        _.skipNoops();
        if (_.pos >= _.data.len) {
            return null;
        }
        return _.data[_.pos];
    }

    fn readRawU8(*_): u8? {
        if (_.remaining() < 1) {
            return null;
        }
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readRawU16BE(*_): u16? {
        if (_.remaining() < 2) {
            return null;
        }
        const b0 = _.data[_.pos] as u16;
        const b1 = _.data[_.pos + 1] as u16;
        _.pos += 2;
        return (b0 << 8) | b1;
    }

    fn readRawU32BE(*_): u32? {
        if (_.remaining() < 4) {
            return null;
        }
        const b0 = _.data[_.pos] as u32;
        const b1 = _.data[_.pos + 1] as u32;
        const b2 = _.data[_.pos + 2] as u32;
        const b3 = _.data[_.pos + 3] as u32;
        _.pos += 4;
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    fn readRawU64BE(*_): u64? {
        if (_.remaining() < 8) {
            return null;
        }
        const hi = _.readRawU32BE() ?? return null;
        const lo = _.readRawU32BE() ?? return null;
        return ((hi as u64) << 32) | (lo as u64);
    }

    fn readRawI32BE(*_): i32? {
        const raw = _.readRawU32BE() ?? return null;
        unsafe {
            const v: i32 = @bitCast(raw);
            return v;
        }
    }

    fn readNull(*_): bool {
        const m = _.peekMarker() ?? return false;
        if (m != UBJ_NULL) {
            return false;
        }
        _.pos += 1;
        return true;
    }

    fn readBool(*_): bool? {
        const m = _.peekMarker() ?? return null;
        if (m == UBJ_TRUE) {
            _.pos += 1;
            return true;
        }
        if (m == UBJ_FALSE) {
            _.pos += 1;
            return false;
        }
        return null;
    }

    fn readInt(*_): i64? {
        const m = _.peekMarker() ?? return null;
        if (m == UBJ_UINT8) {
            if (_.remaining() < 2) return null;
            _.pos += 1;
            const v = _.readRawU8() ?? return null;
            return v as i64;
        }
        if (m == UBJ_INT8) {
            if (_.remaining() < 2) return null;
            _.pos += 1;
            const raw = _.readRawU8() ?? return null;
            unsafe {
                const v: i8 = @bitCast(raw);
                return v as i64;
            }
        }
        if (m == UBJ_INT16) {
            if (_.remaining() < 3) return null;
            _.pos += 1;
            const raw = _.readRawU16BE() ?? return null;
            unsafe {
                const v: i16 = @bitCast(raw);
                return v as i64;
            }
        }
        if (m == UBJ_INT32) {
            if (_.remaining() < 5) return null;
            _.pos += 1;
            const v = _.readRawI32BE() ?? return null;
            return v as i64;
        }
        if (m == UBJ_INT64) {
            if (_.remaining() < 9) return null;
            _.pos += 1;
            const raw = _.readRawU64BE() ?? return null;
            unsafe {
                const v: i64 = @bitCast(raw);
                return v;
            }
        }
        return null;
    }

    fn readFloat32(*_): f32? {
        const m = _.peekMarker() ?? return null;
        if (m != UBJ_FLOAT32 || _.remaining() < 5) {
            return null;
        }
        _.pos += 1;
        const raw = _.readRawU32BE() ?? return null;
        unsafe {
            const v: f32 = @bitCast(raw);
            return v;
        }
    }

    fn readFloat64(*_): f64? {
        const m = _.peekMarker() ?? return null;
        if (m != UBJ_FLOAT64 || _.remaining() < 9) {
            return null;
        }
        _.pos += 1;
        const raw = _.readRawU64BE() ?? return null;
        unsafe {
            const v: f64 = @bitCast(raw);
            return v;
        }
    }

    fn readChar(*_): u8? {
        const m = _.peekMarker() ?? return null;
        if (m != UBJ_CHAR || _.remaining() < 2) {
            return null;
        }
        _.pos += 1;
        const ch = _.readRawU8() ?? return null;
        if (ch > 127) {
            return null;
        }
        return ch;
    }

    fn readLengthPrefixedBytes(*_): ([]const u8)? {
        const len_i64 = _.readInt() ?? return null;
        if (len_i64 < 0) {
            return null;
        }
        const len_u64 = len_i64 as u64;
        if (len_u64 > (_.remaining() as u64)) {
            return null;
        }
        const n = len_u64 as usize;
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }

    fn readString(*_): ([]const u8)? {
        const m = _.peekMarker() ?? return null;
        if (m != UBJ_STRING) {
            return null;
        }
        _.pos += 1;
        return _.readLengthPrefixedBytes();
    }

    fn readKey(*_): ([]const u8)? {
        const m = _.peekMarker() ?? return null;
        if (m == UBJ_OBJECT_END) {
            _.pos += 1;
            return null;
        }
        return _.readLengthPrefixedBytes();
    }

    fn readContainerHeader(*_, is_object: bool): UbjsonContainerHeader? {
        const m = _.peekMarker() ?? return null;
        if (is_object) {
            if (m != UBJ_OBJECT_START) return null;
        } else {
            if (m != UBJ_ARRAY_START) return null;
        }
        _.pos += 1;

        let elem_type: u8? = null;
        let count: u64? = null;

        if (_.pos < _.data.len && _.data[_.pos] == UBJ_OPT_TYPE) {
            _.pos += 1;
            const et = _.readRawU8() ?? return null;
            if (!isValidValueMarker(et)) {
                return null;
            }
            elem_type = et;
            // Per UBJSON Draft 12: '$' type MUST be followed by '#' count
            if (_.pos >= _.data.len || _.data[_.pos] != UBJ_OPT_COUNT) {
                return null;
            }
        }

        if (_.pos < _.data.len && _.data[_.pos] == UBJ_OPT_COUNT) {
            _.pos += 1;
            const c_i64 = _.readInt() ?? return null;
            if (c_i64 < 0) {
                return null;
            }
            count = c_i64 as u64;
        }

        return UbjsonContainerHeader{
            .is_object = is_object,
            .elem_type = elem_type,
            .count = count,
        };
    }

    fn skipBytes(*_, n: usize): bool {
        if (n > _.remaining()) {
            return false;
        }
        _.pos += n;
        return true;
    }

    fn skipPayloadByMarker(*_, m: u8, depth: u32): bool {
        if (depth > MAX_UBJ_DEPTH) {
            return false;
        }
        if (m == UBJ_NULL || m == UBJ_NOOP || m == UBJ_TRUE || m == UBJ_FALSE) {
            return true;
        }
        if (m == UBJ_INT8 || m == UBJ_UINT8) {
            return _.skipBytes(1);
        }
        if (m == UBJ_CHAR) {
            const ch = _.readRawU8() ?? return false;
            return ch <= 127;
        }
        if (m == UBJ_INT16) {
            return _.skipBytes(2);
        }
        if (m == UBJ_INT32 || m == UBJ_FLOAT32) {
            return _.skipBytes(4);
        }
        if (m == UBJ_INT64 || m == UBJ_FLOAT64) {
            return _.skipBytes(8);
        }
        if (m == UBJ_STRING || m == UBJ_HIGH_PREC) {
            if (_s := _.readLengthPrefixedBytes()) {
                return true;
            }
            return false;
        }
        if (m == UBJ_ARRAY_START || m == UBJ_OBJECT_START) {
            if (depth + 1 > MAX_UBJ_DEPTH) {
                return false;
            }
            // Backtrack 1 byte so readContainerHeader can inspect '[' / '{'
            _.pos -= 1;
            const is_obj = (m == UBJ_OBJECT_START);
            const hdr = _.readContainerHeader(is_obj) ?? return false;

            if (cnt := hdr.count) {
                if (!is_obj) {
                    if (et := hdr.elem_type) {
                        // Fast O(1) skip for fixed-width primitive typed arrays
                        let stride: u64 = 0;
                        let is_fixed = false;
                        if (et == UBJ_NULL || et == UBJ_NOOP || et == UBJ_TRUE || et == UBJ_FALSE) {
                            stride = 0;
                            is_fixed = true;
                        } else if (et == UBJ_INT8 || et == UBJ_UINT8) {
                            stride = 1;
                            is_fixed = true;
                        } else if (et == UBJ_INT16) {
                            stride = 2;
                            is_fixed = true;
                        } else if (et == UBJ_INT32 || et == UBJ_FLOAT32) {
                            stride = 4;
                            is_fixed = true;
                        } else if (et == UBJ_INT64 || et == UBJ_FLOAT64) {
                            stride = 8;
                            is_fixed = true;
                        }
                        if (is_fixed) {
                            if (stride == 0) {
                                return true;
                            }
                            const rem_u64 = _.remaining() as u64;
                            if (cnt > rem_u64 / stride) {
                                return false;
                            }
                            return _.skipBytes((cnt * stride) as usize);
                        }
                        let i: u64 = 0;
                        while (i < cnt) {
                            if (!_.skipPayloadByMarker(et, depth + 1)) {
                                return false;
                            }
                            i += 1;
                        }
                        return true;
                    }
                    let i: u64 = 0;
                    while (i < cnt) {
                        if (!_.skipValueDepth(depth + 1)) {
                            return false;
                        }
                        i += 1;
                    }
                    return true;
                } else {
                    let i: u64 = 0;
                    while (i < cnt) {
                        if (_k := _.readLengthPrefixedBytes()) {
                            // valid key
                        } else {
                            return false;
                        }
                        if (et := hdr.elem_type) {
                            if (!_.skipPayloadByMarker(et, depth + 1)) {
                                return false;
                            }
                        } else {
                            if (!_.skipValueDepth(depth + 1)) {
                                return false;
                            }
                        }
                        i += 1;
                    }
                    return true;
                }
            } else {
                if (!is_obj) {
                    while (true) {
                        const nxt = _.peekMarker() ?? return false;
                        if (nxt == UBJ_ARRAY_END) {
                            _.pos += 1;
                            return true;
                        }
                        if (!_.skipValueDepth(depth + 1)) {
                            return false;
                        }
                    }
                } else {
                    while (true) {
                        const nxt = _.peekMarker() ?? return false;
                        if (nxt == UBJ_OBJECT_END) {
                            _.pos += 1;
                            return true;
                        }
                        if (_k := _.readLengthPrefixedBytes()) {
                            // valid key
                        } else {
                            return false;
                        }
                        if (!_.skipValueDepth(depth + 1)) {
                            return false;
                        }
                    }
                }
            }
        }
        return false;
    }

    fn skipValueDepth(*_, depth: u32): bool {
        if (depth > MAX_UBJ_DEPTH) {
            return false;
        }
        const m = _.peekMarker() ?? return false;
        _.pos += 1;
        return _.skipPayloadByMarker(m, depth);
    }

    fn skipValue(*_): bool {
        return _.skipValueDepth(0);
    }
}
