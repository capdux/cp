// @cap/ciborium-ll — Zero-heap RFC 8949 low-level streaming CBOR primitives
// Ported from crates.io:ciborium-ll (https://github.com/enarx/ciborium)

pub enum CborHeaderKind {
    Positive,
    Negative,
    Bytes,
    Text,
    Array,
    Map,
    Tag,
    Bool,
    Null,
    Undefined,
    Float,
    Break,
    Invalid,
}

pub struct CborHeader {
    kind: CborHeaderKind,
    u64_val: u64,
    i64_val: i64,
    f64_val: f64,
    bool_val: bool,
    len: i64,

    static fn invalid(): CborHeader {
        return CborHeader{
            kind: CborHeaderKind.Invalid,
            u64_val: 0,
            i64_val: 0,
            f64_val: 0.0,
            bool_val: false,
            len: 0,
        };
    }
}

fn cblMaxI64AsU64(): u64 {
    const hi: u64 = 0x7FFFFFFF;
    const lo: u64 = 0xFFFFFFFF;
    return (hi << 32) | lo;
}

fn cblReadBe16(data: []const u8, pos: usize): u16 {
    const b0: u16 = data[pos] as u16;
    const b1: u16 = data[pos + 1] as u16;
    return (b0 << 8) | b1;
}

fn cblReadBe32(data: []const u8, pos: usize): u32 {
    const b0: u32 = data[pos] as u32;
    const b1: u32 = data[pos + 1] as u32;
    const b2: u32 = data[pos + 2] as u32;
    const b3: u32 = data[pos + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn cblReadBe64(data: []const u8, pos: usize): u64 {
    const hi: u64 = cblReadBe32(data, pos) as u64;
    const lo: u64 = cblReadBe32(data, pos + 4) as u64;
    return (hi << 32) | lo;
}

fn cblDecodeHalf(bits: u16): f64 {
    const sign: u64 = ((bits >> 15) as u64) << 63;
    const exp: u32 = ((bits >> 10) & 0x1F) as u32;
    const frac: u32 = (bits & 0x03FF) as u32;

    if (exp == 0) {
        if (frac == 0) {
            return @as(f64, @bitCast(sign));
        }
        const sub: f64 = (frac as f64) / 16777216.0;
        if (sign != 0) return -sub;
        return sub;
    }
    if (exp == 31) {
        const inf_exp: u64 = (0x7FF as u64) << 52;
        const inf_frac: u64 = (frac as u64) << 42;
        return @as(f64, @bitCast(sign | inf_exp | inf_frac));
    }
    const f64_exp: u64 = ((exp + 1008) as u64) << 52;
    const f64_frac: u64 = (frac as u64) << 42;
    return @as(f64, @bitCast(sign | f64_exp | f64_frac));
}

fn cblArgSize(val: u64): usize {
    if (val < 24) return 1;
    if (val <= 0xFF) return 2;
    if (val <= 0xFFFF) return 3;
    if (val <= 0xFFFFFFFF) return 5;
    return 9;
}

fn cblWriteHead(buf: []u8, pos: usize, major: u8, val: u64): usize {
    const prefix: u8 = major << 5;
    if (val < 24) {
        buf[pos] = prefix | (val as u8);
        return pos + 1;
    }
    if (val <= 0xFF) {
        buf[pos] = prefix | 24;
        buf[pos + 1] = val as u8;
        return pos + 2;
    }
    if (val <= 0xFFFF) {
        buf[pos] = prefix | 25;
        buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 2] = (val & 0xFF) as u8;
        return pos + 3;
    }
    if (val <= 0xFFFFFFFF) {
        buf[pos] = prefix | 26;
        buf[pos + 1] = ((val >> 24) & 0xFF) as u8;
        buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
        buf[pos + 3] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 4] = (val & 0xFF) as u8;
        return pos + 5;
    }
    buf[pos] = prefix | 27;
    buf[pos + 1] = ((val >> 56) & 0xFF) as u8;
    buf[pos + 2] = ((val >> 48) & 0xFF) as u8;
    buf[pos + 3] = ((val >> 40) & 0xFF) as u8;
    buf[pos + 4] = ((val >> 32) & 0xFF) as u8;
    buf[pos + 5] = ((val >> 24) & 0xFF) as u8;
    buf[pos + 6] = ((val >> 16) & 0xFF) as u8;
    buf[pos + 7] = ((val >> 8) & 0xFF) as u8;
    buf[pos + 8] = (val & 0xFF) as u8;
    return pos + 9;
}

/// Low-level streaming CBOR cursor decoder (RFC 8949).
pub struct CborDecoder {
    data: []const u8,
    pos: usize,

    static fn new(data: []const u8): CborDecoder {
        return CborDecoder{
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

    fn readBytes(*_, n: usize): ([]const u8)? {
        if (n > _.data.len - _.pos) {
            return null;
        }
        const start = _.pos;
        _.pos += n;
        return _.data[start .. start + n];
    }

    fn pull(*_): CborHeader {
        if (_.pos >= _.data.len) {
            return CborHeader.invalid();
        }
        const initial: u8 = _.data[_.pos];
        const major: u8 = initial >> 5;
        const ai: u8 = initial & 0x1F;

        // Major type 7: simple values, floats, break
        if (major == 7) {
            if (ai == 20) {
                _.pos += 1;
                return CborHeader{
                    kind: CborHeaderKind.Bool,
                    u64_val: 0,
                    i64_val: 0,
                    f64_val: 0.0,
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 21) {
                _.pos += 1;
                return CborHeader{
                    kind: CborHeaderKind.Bool,
                    u64_val: 1,
                    i64_val: 1,
                    f64_val: 0.0,
                    bool_val: true,
                    len: 0,
                };
            }
            if (ai == 22) {
                _.pos += 1;
                return CborHeader{
                    kind: CborHeaderKind.Null,
                    u64_val: 0,
                    i64_val: 0,
                    f64_val: 0.0,
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 23) {
                _.pos += 1;
                return CborHeader{
                    kind: CborHeaderKind.Undefined,
                    u64_val: 0,
                    i64_val: 0,
                    f64_val: 0.0,
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 25) {
                if (_.data.len - _.pos < 3) return CborHeader.invalid();
                const bits: u16 = cblReadBe16(_.data, _.pos + 1);
                _.pos += 3;
                return CborHeader{
                    kind: CborHeaderKind.Float,
                    u64_val: bits as u64,
                    i64_val: 0,
                    f64_val: cblDecodeHalf(bits),
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 26) {
                if (_.data.len - _.pos < 5) return CborHeader.invalid();
                const bits: u32 = cblReadBe32(_.data, _.pos + 1);
                _.pos += 5;
                const f32_val = @as(f32, @bitCast(bits));
                return CborHeader{
                    kind: CborHeaderKind.Float,
                    u64_val: bits as u64,
                    i64_val: 0,
                    f64_val: f32_val as f64,
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 27) {
                if (_.data.len - _.pos < 9) return CborHeader.invalid();
                const bits: u64 = cblReadBe64(_.data, _.pos + 1);
                _.pos += 9;
                const f64_val = @as(f64, @bitCast(bits));
                return CborHeader{
                    kind: CborHeaderKind.Float,
                    u64_val: bits,
                    i64_val: 0,
                    f64_val: f64_val,
                    bool_val: false,
                    len: 0,
                };
            }
            if (ai == 31) {
                _.pos += 1;
                return CborHeader{
                    kind: CborHeaderKind.Break,
                    u64_val: 0,
                    i64_val: 0,
                    f64_val: 0.0,
                    bool_val: false,
                    len: 0,
                };
            }
            return CborHeader.invalid();
        }

        // Major types 0..6: parse argument or indefinite marker (ai == 31)
        let is_indef: bool = false;
        let arg: u64 = 0;
        let advance: usize = 1;

        if (ai < 24) {
            arg = ai as u64;
        } else if (ai == 24) {
            if (_.data.len - _.pos < 2) return CborHeader.invalid();
            arg = _.data[_.pos + 1] as u64;
            advance = 2;
        } else if (ai == 25) {
            if (_.data.len - _.pos < 3) return CborHeader.invalid();
            arg = cblReadBe16(_.data, _.pos + 1) as u64;
            advance = 3;
        } else if (ai == 26) {
            if (_.data.len - _.pos < 5) return CborHeader.invalid();
            arg = cblReadBe32(_.data, _.pos + 1) as u64;
            advance = 5;
        } else if (ai == 27) {
            if (_.data.len - _.pos < 9) return CborHeader.invalid();
            arg = cblReadBe64(_.data, _.pos + 1);
            advance = 9;
        } else if (ai == 31) {
            if (major < 2 || major > 5) return CborHeader.invalid();
            is_indef = true;
        } else {
            return CborHeader.invalid();
        }

        _.pos += advance;
        const max_i64: u64 = cblMaxI64AsU64();

        if (major == 0) {
            let iv: i64 = 0;
            if (arg <= max_i64) iv = arg as i64;
            return CborHeader{
                kind: CborHeaderKind.Positive,
                u64_val: arg,
                i64_val: iv,
                f64_val: 0.0,
                bool_val: false,
                len: 0,
            };
        }
        if (major == 1) {
            let iv: i64 = 0;
            if (arg <= max_i64) {
                iv = (-1 as i64) - (arg as i64);
            }
            return CborHeader{
                kind: CborHeaderKind.Negative,
                u64_val: arg,
                i64_val: iv,
                f64_val: 0.0,
                bool_val: false,
                len: 0,
            };
        }
        if (major == 2) {
            let l: i64 = -1;
            if (!is_indef) l = arg as i64;
            return CborHeader{
                kind: CborHeaderKind.Bytes,
                u64_val: arg,
                i64_val: 0,
                f64_val: 0.0,
                bool_val: false,
                len: l,
            };
        }
        if (major == 3) {
            let l: i64 = -1;
            if (!is_indef) l = arg as i64;
            return CborHeader{
                kind: CborHeaderKind.Text,
                u64_val: arg,
                i64_val: 0,
                f64_val: 0.0,
                bool_val: false,
                len: l,
            };
        }
        if (major == 4) {
            let l: i64 = -1;
            if (!is_indef) l = arg as i64;
            return CborHeader{
                kind: CborHeaderKind.Array,
                u64_val: arg,
                i64_val: 0,
                f64_val: 0.0,
                bool_val: false,
                len: l,
            };
        }
        if (major == 5) {
            let l: i64 = -1;
            if (!is_indef) l = arg as i64;
            return CborHeader{
                kind: CborHeaderKind.Map,
                u64_val: arg,
                i64_val: 0,
                f64_val: 0.0,
                bool_val: false,
                len: l,
            };
        }
        // major == 6 (Tag)
        return CborHeader{
            kind: CborHeaderKind.Tag,
            u64_val: arg,
            i64_val: 0,
            f64_val: 0.0,
            bool_val: false,
            len: 0,
        };
    }
}

/// Zero-heap RFC 8949 CBOR encoder writing directly into a caller-supplied []u8 buffer.
pub struct CborEncoder {
    buf: []u8,
    pos: usize,

    static fn new(buf: []u8): CborEncoder {
        return CborEncoder{
            buf: buf,
            pos: 0,
        };
    }

    fn written(): usize {
        return _.pos;
    }

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn slice(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn pushPositive(*_, val: u64): bool {
        const need = cblArgSize(val);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 0, val);
        return true;
    }

    fn pushNegativeRaw(*_, raw: u64): bool {
        const need = cblArgSize(raw);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 1, raw);
        return true;
    }

    fn pushNegative(*_, val: i64): bool {
        if (val >= 0) return false;
        const raw: u64 = (-(val + 1)) as u64;
        return _.pushNegativeRaw(raw);
    }

    fn pushBytesHeader(*_, len: i64): bool {
        if (len < 0) {
            if (_.buf.len - _.pos < 1) return false;
            _.buf[_.pos] = 0x5F;
            _.pos += 1;
            return true;
        }
        const ulen: u64 = len as u64;
        const need = cblArgSize(ulen);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 2, ulen);
        return true;
    }

    fn pushBytes(*_, data: []const u8): bool {
        const ulen: u64 = data.len as u64;
        const need = cblArgSize(ulen) + data.len;
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 2, ulen);
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos] = data[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn pushTextHeader(*_, len: i64): bool {
        if (len < 0) {
            if (_.buf.len - _.pos < 1) return false;
            _.buf[_.pos] = 0x7F;
            _.pos += 1;
            return true;
        }
        const ulen: u64 = len as u64;
        const need = cblArgSize(ulen);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 3, ulen);
        return true;
    }

    fn pushText(*_, text: string): bool {
        const ulen: u64 = text.len as u64;
        const need = cblArgSize(ulen) + text.len;
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 3, ulen);
        let i: usize = 0;
        while (i < text.len) {
            _.buf[_.pos] = text[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn pushArray(*_, len: i64): bool {
        if (len < 0) {
            if (_.buf.len - _.pos < 1) return false;
            _.buf[_.pos] = 0x9F;
            _.pos += 1;
            return true;
        }
        const ulen: u64 = len as u64;
        const need = cblArgSize(ulen);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 4, ulen);
        return true;
    }

    fn pushMap(*_, len: i64): bool {
        if (len < 0) {
            if (_.buf.len - _.pos < 1) return false;
            _.buf[_.pos] = 0xBF;
            _.pos += 1;
            return true;
        }
        const ulen: u64 = len as u64;
        const need = cblArgSize(ulen);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 5, ulen);
        return true;
    }

    fn pushTag(*_, tag: u64): bool {
        const need = cblArgSize(tag);
        if (_.buf.len - _.pos < need) return false;
        _.pos = cblWriteHead(_.buf, _.pos, 6, tag);
        return true;
    }

    fn pushBool(*_, val: bool): bool {
        if (_.buf.len - _.pos < 1) return false;
        if (val) {
            _.buf[_.pos] = 0xF5;
        } else {
            _.buf[_.pos] = 0xF4;
        }
        _.pos += 1;
        return true;
    }

    fn pushNull(*_): bool {
        if (_.buf.len - _.pos < 1) return false;
        _.buf[_.pos] = 0xF6;
        _.pos += 1;
        return true;
    }

    fn pushUndefined(*_): bool {
        if (_.buf.len - _.pos < 1) return false;
        _.buf[_.pos] = 0xF7;
        _.pos += 1;
        return true;
    }

    fn pushFloat64(*_, val: f64): bool {
        if (_.buf.len - _.pos < 9) return false;
        const bits = @as(u64, @bitCast(val));
        _.buf[_.pos] = 0xFB;
        _.buf[_.pos + 1] = ((bits >> 56) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((bits >> 48) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((bits >> 40) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((bits >> 32) & 0xFF) as u8;
        _.buf[_.pos + 5] = ((bits >> 24) & 0xFF) as u8;
        _.buf[_.pos + 6] = ((bits >> 16) & 0xFF) as u8;
        _.buf[_.pos + 7] = ((bits >> 8) & 0xFF) as u8;
        _.buf[_.pos + 8] = (bits & 0xFF) as u8;
        _.pos += 9;
        return true;
    }

    fn pushBreak(*_): bool {
        if (_.buf.len - _.pos < 1) return false;
        _.buf[_.pos] = 0xFF;
        _.pos += 1;
        return true;
    }
}
