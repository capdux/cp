// @cap/bincode — Zero-allocation bincode v2 compact binary codec for CP
// Clean-room MIT implementation inspired by Rust `bincode` v2 (crates.io #381, 391M+ downloads).
//
// Wire format (bincode v2 standard):
// - Unsigned varint: 0..=250 -> 1 byte; 251..=65535 -> [251, u16 LE];
//   65536..=4294967295 -> [252, u32 LE]; >= 2^32 -> [253, u64 LE].
// - Signed integers: overflow-free ZigZag mapping over the entire i64 domain, then varint.
// - Strings: varint byte length prefix followed by raw UTF-8 bytes (zero-copy slice on decode).
// - Booleans: strictly 0 or 1; Option<string>: 0 (null) or 1 + string.

const TAG_U16: u8 = 251;
const TAG_U32: u8 = 252;
const TAG_U64: u8 = 253;

pub struct OptStrResult {
    pub ok: bool,
    pub has_value: bool,
    pub value: string,
}

fn zigzagEncode(v: i64) -> u64 {
    if (v >= 0) {
        return (v as u64) << 1;
    }
    const mag: u64 = (0 - (v + 1)) as u64;
    return (mag << 1) | 1;
}

fn zigzagDecode(u: u64) -> i64 {
    if ((u & 1) == 0) {
        return (u >> 1) as i64;
    }
    const half: i64 = (u >> 1) as i64;
    return (0 - half) - 1;
}

/// Zero-allocation bincode v2 encoder writing directly into caller-provided `out: []u8`.
pub struct BincodeEncoder {
    pub out: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(out: []u8) -> BincodeEncoder {
        return BincodeEncoder{ out: out, pos: 0, overflow: false };
    }

    pub fn asSlice() -> string {
        return _.out[0.._.pos];
    }

    fn writeByte(*_, b: u8) -> bool {
        if (_.overflow || _.pos >= _.out.len) {
            _.overflow = true;
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    pub fn encodeBool(*_, v: bool) -> bool {
        return _.writeByte(if (v) { 1 as u8 } else { 0 as u8 });
    }

    pub fn encodeFixedU32(*_, v: u32) -> bool {
        if (_.overflow || _.pos + 4 > _.out.len) {
            _.overflow = true;
            return false;
        }
        _.out[_.pos] = (v & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.out[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    pub fn encodeFixedU64(*_, v: u64) -> bool {
        if (_.overflow || _.pos + 8 > _.out.len) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < 8) {
            _.out[_.pos + i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
            i += 1;
        }
        _.pos += 8;
        return true;
    }

    /// Encodes `u64` using bincode v2 compact varint encoding.
    pub fn encodeU64(*_, v: u64) -> bool {
        if (v <= 250) {
            return _.writeByte(v as u8);
        }
        if (v <= 65535) {
            if (_.overflow || _.pos + 3 > _.out.len) {
                _.overflow = true;
                return false;
            }
            _.out[_.pos] = TAG_U16;
            _.out[_.pos + 1] = (v & 0xFF) as u8;
            _.out[_.pos + 2] = ((v >> 8) & 0xFF) as u8;
            _.pos += 3;
            return true;
        }
        if (v <= 4294967295) {
            if (!_.writeByte(TAG_U32)) {
                return false;
            }
            return _.encodeFixedU32(v as u32);
        }
        if (!_.writeByte(TAG_U64)) {
            return false;
        }
        return _.encodeFixedU64(v);
    }

    /// Encodes `i64` using ZigZag mapping followed by bincode v2 varint.
    pub fn encodeI64(*_, v: i64) -> bool {
        return _.encodeU64(zigzagEncode(v));
    }

    /// Encodes a string with a varint byte-length prefix followed by raw bytes.
    pub fn encodeStr(*_, s: string) -> bool {
        if (!_.encodeU64(s.len as u64)) {
            return false;
        }
        if (_.overflow || _.pos + s.len > _.out.len) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
        return true;
    }

    /// Encodes an optional string: `0` for `null`, or `1` followed by length-prefixed string.
    pub fn encodeOptStr(*_, s: string?) -> bool {
        if (s == null) {
            return _.writeByte(0 as u8);
        }
        if (!_.writeByte(1 as u8)) {
            return false;
        }
        return _.encodeStr(s.?);
    }
}

/// Zero-copy bincode v2 decoder reading directly from `data: string`.
pub struct BincodeDecoder {
    pub data: string,
    pub pos: usize,

    pub static fn init(data: string) -> BincodeDecoder {
        return BincodeDecoder{ data: data, pos: 0 };
    }

    pub fn remaining() -> usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    pub fn atEnd() -> bool {
        return _.pos >= _.data.len;
    }

    /// Decodes a boolean byte. Strictly rejects values other than `0` or `1`.
    pub fn decodeBool(*_) -> bool? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const b = _.data[_.pos];
        if (b == 0) {
            _.pos += 1;
            return false;
        }
        if (b == 1) {
            _.pos += 1;
            return true;
        }
        return null;
    }

    pub fn decodeFixedU32(*_) -> u32? {
        if (_.pos + 4 > _.data.len) {
            return null;
        }
        const b0 = _.data[_.pos] as u32;
        const b1 = _.data[_.pos + 1] as u32;
        const b2 = _.data[_.pos + 2] as u32;
        const b3 = _.data[_.pos + 3] as u32;
        _.pos += 4;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    pub fn decodeFixedU64(*_) -> u64? {
        if (_.pos + 8 > _.data.len) {
            return null;
        }
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            acc = acc | ((_.data[_.pos + i] as u64) << ((i * 8) as u6));
            i += 1;
        }
        _.pos += 8;
        return acc;
    }

    /// Decodes a bincode v2 varint `u64`. Rejects unknown tags (`254`, `255`) and truncated streams.
    pub fn decodeU64(*_) -> u64? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const tag = _.data[_.pos];
        if (tag <= 250) {
            _.pos += 1;
            return tag as u64;
        }
        if (tag == TAG_U16) {
            if (_.pos + 3 > _.data.len) {
                return null;
            }
            const b0 = _.data[_.pos + 1] as u64;
            const b1 = _.data[_.pos + 2] as u64;
            _.pos += 3;
            return b0 | (b1 << 8);
        }
        if (tag == TAG_U32) {
            if (_.pos + 5 > _.data.len) {
                return null;
            }
            _.pos += 1;
            const v_opt = _.decodeFixedU32();
            if (v_opt == null) {
                return null;
            }
            return v_opt.? as u64;
        }
        if (tag == TAG_U64) {
            if (_.pos + 9 > _.data.len) {
                return null;
            }
            _.pos += 1;
            return _.decodeFixedU64();
        }
        return null;
    }

    /// Decodes a ZigZag varint `i64`.
    pub fn decodeI64(*_) -> i64? {
        const u_opt = _.decodeU64();
        if (u_opt == null) {
            return null;
        }
        return zigzagDecode(u_opt.?);
    }

    /// Decodes a length-prefixed string as a zero-copy slice into `data`.
    pub fn decodeStr(*_) -> string? {
        const len_opt = _.decodeU64();
        if (len_opt == null) {
            return null;
        }
        const slen = len_opt.? as usize;
        if (slen > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += slen;
        return _.data[start.._.pos];
    }

    /// Decodes an optional string (`0` -> none, `1` + string -> some).
    pub fn decodeOptStr(*_) -> OptStrResult {
        if (_.pos >= _.data.len) {
            return OptStrResult{ ok: false, has_value: false, value: "" };
        }
        const tag = _.data[_.pos];
        if (tag == 0) {
            _.pos += 1;
            return OptStrResult{ ok: true, has_value: false, value: "" };
        }
        if (tag == 1) {
            _.pos += 1;
            const s_opt = _.decodeStr();
            if (s_opt == null) {
                return OptStrResult{ ok: false, has_value: false, value: "" };
            }
            return OptStrResult{ ok: true, has_value: true, value: s_opt.? };
        }
        return OptStrResult{ ok: false, has_value: false, value: "" };
    }
}
