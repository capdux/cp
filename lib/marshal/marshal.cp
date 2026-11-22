// Origin: Pure-CP Stdlib (Zero-Alloc Ruby Marshal 4.8 Binary Parser) | Created: 2026-11-22

pub const MARSHAL_MAJOR: u8 = 4;
pub const MARSHAL_MINOR: u8 = 8;
pub const MAX_MARSHAL_DEPTH: u32 = 64;

pub const TYPE_NIL: u8 = '0';
pub const TYPE_TRUE: u8 = 'T';
pub const TYPE_FALSE: u8 = 'F';
pub const TYPE_FIXNUM: u8 = 'i';
pub const TYPE_FLOAT: u8 = 'f';
pub const TYPE_BIGNUM: u8 = 'l';
pub const TYPE_STRING: u8 = '"';
pub const TYPE_SYMBOL: u8 = ':';
pub const TYPE_SYMLINK: u8 = ';';
pub const TYPE_ARRAY: u8 = '[';
pub const TYPE_HASH: u8 = '{';
pub const TYPE_HASH_DEF: u8 = '}';
pub const TYPE_IVAR: u8 = 'I';
pub const TYPE_OBJECT: u8 = 'o';
pub const TYPE_LINK: u8 = '@';
pub const TYPE_REGEXP: u8 = '/';
pub const TYPE_STRUCT: u8 = 'S';
pub const TYPE_CLASS: u8 = 'c';
pub const TYPE_MODULE: u8 = 'm';

pub struct MarshalHeader {
    major: u8,
    minor: u8,
}

pub struct MarshalIntResult {
    value: i32,
    next_pos: usize,
}

pub fn parseMarshalHeader(data: []const u8): MarshalHeader? {
    if (data.len < 2) {
        return null;
    }
    if (data[0] != MARSHAL_MAJOR || data[1] != MARSHAL_MINOR) {
        return null;
    }
    return MarshalHeader{
        .major = data[0],
        .minor = data[1],
    };
}

pub fn isMarshal(data: []const u8): bool {
    if (_hdr := parseMarshalHeader(data)) {
        return true;
    }
    return false;
}

pub fn readMarshalInt(data: []const u8, pos: usize): MarshalIntResult? {
    if (pos >= data.len) {
        return null;
    }
    const raw_c = data[pos];
    let c: i8 = 0;
    unsafe {
        c = @bitCast(raw_c);
    }

    if (c == 0) {
        return MarshalIntResult{
            .value = 0,
            .next_pos = pos + 1,
        };
    }
    if (c >= 5) {
        return MarshalIntResult{
            .value = (c as i32) - 5,
            .next_pos = pos + 1,
        };
    }
    if (c <= -5) {
        return MarshalIntResult{
            .value = (c as i32) + 5,
            .next_pos = pos + 1,
        };
    }

    if (c > 0) {
        // 1..4 bytes little-endian positive integer
        const n: usize = c as usize;
        if (n > data.len - (pos + 1)) {
            return null;
        }
        let acc: u32 = 0;
        let i: usize = 0;
        while (i < n) {
            const shift: u5 = (i * 8) as u5;
            acc = acc | ((data[pos + 1 + i] as u32) << shift);
            i += 1;
        }
        let val: i32 = 0;
        unsafe {
            val = @bitCast(acc);
        }
        return MarshalIntResult{
            .value = val,
            .next_pos = pos + 1 + n,
        };
    }

    // -4..-1 => 1..4 bytes little-endian negative two's complement integer
    const n: usize = (-(c as i32)) as usize;
    if (n > data.len - (pos + 1)) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < n) {
        const shift: u5 = (i * 8) as u5;
        acc = acc | ((data[pos + 1 + i] as u32) << shift);
        i += 1;
    }
    if (n < 4) {
        const sign_shift: u5 = (n * 8) as u5;
        acc = acc | ((0xFFFF_FFFF as u32) << sign_shift);
    }
    let val: i32 = 0;
    unsafe {
        val = @bitCast(acc);
    }
    return MarshalIntResult{
        .value = val,
        .next_pos = pos + 1 + n,
    };
}

pub struct MarshalReader {
    data: []const u8,
    pos: usize,
    valid_header: bool,

    fn init(data: []const u8): MarshalReader {
        if (!isMarshal(data)) {
            return MarshalReader{
                .data = data,
                .pos = data.len,
                .valid_header = false,
            };
        }
        return MarshalReader{
            .data = data,
            .pos = 2,
            .valid_header = true,
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

    fn peekTag(): u8? {
        if (!_.valid_header || _.pos >= _.data.len) {
            return null;
        }
        return _.data[_.pos];
    }

    fn readInt(*_): i32? {
        if (!_.valid_header) {
            return null;
        }
        const res = readMarshalInt(_.data, _.pos) ?? return null;
        _.pos = res.next_pos;
        return res.value;
    }

    fn readNil(*_): bool {
        const tag = _.peekTag() ?? return false;
        if (tag != TYPE_NIL) {
            return false;
        }
        _.pos += 1;
        return true;
    }

    fn readBool(*_): bool? {
        const tag = _.peekTag() ?? return null;
        if (tag == TYPE_TRUE) {
            _.pos += 1;
            return true;
        }
        if (tag == TYPE_FALSE) {
            _.pos += 1;
            return false;
        }
        return null;
    }

    fn readFixnum(*_): i32? {
        const tag = _.peekTag() ?? return null;
        if (tag != TYPE_FIXNUM) {
            return null;
        }
        _.pos += 1;
        return _.readInt();
    }

    fn readRawBytes(*_): ([]const u8)? {
        const len_i32 = _.readInt() ?? return null;
        if (len_i32 < 0) {
            return null;
        }
        const n: usize = len_i32 as usize;
        if (n > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }

    fn readSymbol(*_): ([]const u8)? {
        const tag = _.peekTag() ?? return null;
        if (tag != TYPE_SYMBOL) {
            return null;
        }
        _.pos += 1;
        return _.readRawBytes();
    }

    fn readSymlink(*_): i32? {
        const tag = _.peekTag() ?? return null;
        if (tag != TYPE_SYMLINK) {
            return null;
        }
        _.pos += 1;
        return _.readInt();
    }

    fn readArrayLen(*_): usize? {
        const tag = _.peekTag() ?? return null;
        if (tag != TYPE_ARRAY) {
            return null;
        }
        _.pos += 1;
        const n = _.readInt() ?? return null;
        if (n < 0) {
            return null;
        }
        return n as usize;
    }

    fn readHashLen(*_): usize? {
        const tag = _.peekTag() ?? return null;
        if (tag != TYPE_HASH) {
            return null;
        }
        _.pos += 1;
        const n = _.readInt() ?? return null;
        if (n < 0) {
            return null;
        }
        return n as usize;
    }

    fn readString(*_): ([]const u8)? {
        const tag = _.peekTag() ?? return null;
        if (tag == TYPE_STRING) {
            _.pos += 1;
            return _.readRawBytes();
        }
        if (tag == TYPE_IVAR) {
            _.pos += 1;
            const inner_tag = _.peekTag() ?? return null;
            if (inner_tag != TYPE_STRING) {
                return null;
            }
            _.pos += 1;
            const s = _.readRawBytes() ?? return null;
            const ivar_cnt = _.readInt() ?? return null;
            if (ivar_cnt < 0) {
                return null;
            }
            let i: i32 = 0;
            while (i < ivar_cnt) {
                if (!_.skipValueDepth(1)) return null;
                if (!_.skipValueDepth(1)) return null;
                i += 1;
            }
            return s;
        }
        return null;
    }

    fn skipValueDepth(*_, depth: u32): bool {
        if (!_.valid_header || depth > MAX_MARSHAL_DEPTH) {
            return false;
        }
        const tag = _.peekTag() ?? return false;
        _.pos += 1;

        if (tag == TYPE_NIL || tag == TYPE_TRUE || tag == TYPE_FALSE) {
            return true;
        }
        if (tag == TYPE_FIXNUM || tag == TYPE_SYMLINK || tag == TYPE_LINK) {
            if (_v := _.readInt()) {
                return true;
            }
            return false;
        }
        if (tag == TYPE_STRING || tag == TYPE_SYMBOL || tag == TYPE_FLOAT || tag == TYPE_CLASS || tag == TYPE_MODULE) {
            if (_b := _.readRawBytes()) {
                return true;
            }
            return false;
        }
        if (tag == TYPE_REGEXP) {
            if (_b := _.readRawBytes()) {
                // 1 byte regexp options flag
                if (_.remaining() < 1) return false;
                _.pos += 1;
                return true;
            }
            return false;
        }
        if (tag == TYPE_BIGNUM) {
            if (_.remaining() < 1) return false;
            const sign = _.data[_.pos];
            if (sign != '+' && sign != '-') return false;
            _.pos += 1;
            const short_cnt = _.readInt() ?? return false;
            if (short_cnt < 0) return false;
            const sc: usize = short_cnt as usize;
            if (sc > _.remaining() / 2) return false;
            _.pos += sc * 2;
            return true;
        }
        if (tag == TYPE_ARRAY) {
            if (depth + 1 > MAX_MARSHAL_DEPTH) return false;
            const n = _.readInt() ?? return false;
            if (n < 0) return false;
            let i: i32 = 0;
            while (i < n) {
                if (!_.skipValueDepth(depth + 1)) return false;
                i += 1;
            }
            return true;
        }
        if (tag == TYPE_HASH || tag == TYPE_HASH_DEF) {
            if (depth + 1 > MAX_MARSHAL_DEPTH) return false;
            const pairs = _.readInt() ?? return false;
            if (pairs < 0) return false;
            let i: i32 = 0;
            while (i < pairs) {
                if (!_.skipValueDepth(depth + 1)) return false;
                if (!_.skipValueDepth(depth + 1)) return false;
                i += 1;
            }
            if (tag == TYPE_HASH_DEF) {
                if (!_.skipValueDepth(depth + 1)) return false;
            }
            return true;
        }
        if (tag == TYPE_IVAR) {
            if (depth + 1 > MAX_MARSHAL_DEPTH) return false;
            if (!_.skipValueDepth(depth + 1)) return false;
            const ivars = _.readInt() ?? return false;
            if (ivars < 0) return false;
            let i: i32 = 0;
            while (i < ivars) {
                if (!_.skipValueDepth(depth + 1)) return false;
                if (!_.skipValueDepth(depth + 1)) return false;
                i += 1;
            }
            return true;
        }
        if (tag == TYPE_OBJECT || tag == TYPE_STRUCT) {
            if (depth + 1 > MAX_MARSHAL_DEPTH) return false;
            if (!_.skipValueDepth(depth + 1)) return false;
            const members = _.readInt() ?? return false;
            if (members < 0) return false;
            let i: i32 = 0;
            while (i < members) {
                if (!_.skipValueDepth(depth + 1)) return false;
                if (!_.skipValueDepth(depth + 1)) return false;
                i += 1;
            }
            return true;
        }
        return false;
    }

    fn skipValue(*_): bool {
        return _.skipValueDepth(0);
    }
}
