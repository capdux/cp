// @cap/bencode — BitTorrent BEP-3 deterministic Bencode encoder/decoder/validator
// MIT License (clean-room CP implementation, zero-heap)

pub const KIND_INVALID: u8 = 0;
pub const KIND_INT: u8 = 1;
pub const KIND_BYTES: u8 = 2;
pub const KIND_LIST: u8 = 3;
pub const KIND_DICT: u8 = 4;

const MAX_DEPTH: usize = 64;

pub struct BencodeIntResult {
    pub value: i64,
    pub next_pos: usize,
    pub ok: bool,
}

pub struct BencodeBytesResult {
    pub start: usize,
    pub len: usize,
    pub next_pos: usize,
    pub ok: bool,
}

pub struct BencodeWriter {
    pub buf: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(buf: []u8) -> BencodeWriter {
        return BencodeWriter {
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    fn writeByte(*_, b: u8) -> bool {
        if (_.overflow || _.pos >= _.buf.len) {
            _.overflow = true;
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeU64Decimal(*_, val: u64) -> bool {
        let digits: [24]u8 = undefined;
        let count: usize = 0;
        let cur: u64 = val;
        if (cur == 0) {
            digits[0] = '0' as u8;
            count = 1;
        } else {
            while (cur > 0) {
                const rem: u8 = (cur % 10) as u8;
                digits[count] = ('0' as u8) + rem;
                count += 1;
                cur = cur / 10;
            }
        }
        let i: usize = count;
        while (i > 0) {
            i -= 1;
            if (!_.writeByte(digits[i])) {
                return false;
            }
        }
        return true;
    }

    pub fn writeInt(*_, v: i64) -> bool {
        if (!_.writeByte('i' as u8)) {
            return false;
        }
        let mag: u64 = 0;
        if (v < 0) {
            if (!_.writeByte('-' as u8)) {
                return false;
            }
            // Overflow-safe magnitude for all negative i64 including i64::MIN
            mag = ((-(v + 1)) as u64) + 1;
        } else {
            mag = v as u64;
        }
        if (!_.writeU64Decimal(mag)) {
            return false;
        }
        return _.writeByte('e' as u8);
    }

    pub fn writeBytes(*_, data: []const u8) -> bool {
        if (!_.writeU64Decimal(data.len as u64)) {
            return false;
        }
        if (!_.writeByte(':' as u8)) {
            return false;
        }
        if (_.overflow || data.len > _.buf.len - _.pos) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos] = data[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    pub fn beginList(*_) -> bool {
        return _.writeByte('l' as u8);
    }

    pub fn beginDict(*_) -> bool {
        return _.writeByte('d' as u8);
    }

    pub fn end(*_) -> bool {
        return _.writeByte('e' as u8);
    }

    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn len() -> usize {
        return _.pos;
    }
}

pub fn peekKind(src: []const u8, pos: usize) -> u8 {
    if (pos >= src.len) {
        return KIND_INVALID;
    }
    const c: u8 = src[pos];
    if (c == 'i' as u8) {
        return KIND_INT;
    }
    if (c >= '0' as u8 && c <= '9' as u8) {
        return KIND_BYTES;
    }
    if (c == 'l' as u8) {
        return KIND_LIST;
    }
    if (c == 'd' as u8) {
        return KIND_DICT;
    }
    return KIND_INVALID;
}

pub fn decodeInt(src: []const u8, pos: usize) -> BencodeIntResult {
    const fail = BencodeIntResult { value: 0, next_pos: pos, ok: false };
    if (pos >= src.len || src[pos] != 'i' as u8) {
        return fail;
    }
    let i: usize = pos + 1;
    if (i >= src.len) {
        return fail;
    }
    let is_neg: bool = false;
    if (src[i] == '-' as u8) {
        is_neg = true;
        i += 1;
    }
    if (i >= src.len || src[i] < '0' as u8 || src[i] > '9' as u8) {
        return fail;
    }
    // Reject negative zero (i-0e) and leading zeros (i03e, i00e)
    if (src[i] == '0' as u8) {
        if (is_neg) {
            return fail;
        }
        if (i + 1 >= src.len || src[i + 1] != 'e' as u8) {
            return fail;
        }
        return BencodeIntResult { value: 0, next_pos: i + 2, ok: true };
    }

    const max_pos_mag: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const max_neg_mag: u64 = (0x80000000 as u64) << 32;
    let max_mag: u64 = max_pos_mag;
    if (is_neg) {
        max_mag = max_neg_mag;
    }

    let acc: u64 = 0;
    while (i < src.len && src[i] >= '0' as u8 && src[i] <= '9' as u8) {
        const d: u64 = (src[i] - ('0' as u8)) as u64;
        if (acc > (max_mag - d) / 10) {
            return fail;
        }
        acc = acc * 10 + d;
        i += 1;
    }

    if (i >= src.len || src[i] != 'e' as u8) {
        return fail;
    }

    let val: i64 = 0;
    if (is_neg) {
        if (acc == max_neg_mag) {
            val = -(max_pos_mag as i64) - 1;
        } else {
            val = -(acc as i64);
        }
    } else {
        val = acc as i64;
    }
    return BencodeIntResult { value: val, next_pos: i + 1, ok: true };
}

pub fn decodeBytes(src: []const u8, pos: usize) -> BencodeBytesResult {
    const fail = BencodeBytesResult { start: 0, len: 0, next_pos: pos, ok: false };
    if (pos >= src.len) {
        return fail;
    }
    const first: u8 = src[pos];
    if (first < '0' as u8 || first > '9' as u8) {
        return fail;
    }
    if (first == '0' as u8) {
        if (pos + 1 >= src.len || src[pos + 1] != ':' as u8) {
            return fail;
        }
        return BencodeBytesResult { start: pos + 2, len: 0, next_pos: pos + 2, ok: true };
    }

    let i: usize = pos;
    let str_len: usize = 0;
    while (i < src.len && src[i] >= '0' as u8 && src[i] <= '9' as u8) {
        const d: usize = (src[i] - ('0' as u8)) as usize;
        if (str_len > (src.len - d) / 10) {
            return fail;
        }
        str_len = str_len * 10 + d;
        i += 1;
    }
    if (i >= src.len || src[i] != ':' as u8) {
        return fail;
    }
    const data_start: usize = i + 1;
    if (str_len > src.len - data_start) {
        return fail;
    }
    return BencodeBytesResult {
        start: data_start,
        len: str_len,
        next_pos: data_start + str_len,
        ok: true,
    };
}

fn compareBytes(a: []const u8, b: []const u8) -> i32 {
    let min_len: usize = a.len;
    if (b.len < min_len) {
        min_len = b.len;
    }
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn parseValueEnd(src: []const u8, pos: usize, depth: usize) -> i64 {
    if (pos >= src.len || depth > MAX_DEPTH) {
        return -1;
    }
    const c: u8 = src[pos];
    if (c == 'i' as u8) {
        const r: BencodeIntResult = decodeInt(src, pos);
        if (!r.ok) {
            return -1;
        }
        return r.next_pos as i64;
    }
    if (c >= '0' as u8 && c <= '9' as u8) {
        const r: BencodeBytesResult = decodeBytes(src, pos);
        if (!r.ok) {
            return -1;
        }
        return r.next_pos as i64;
    }
    if (c == 'l' as u8) {
        let cur: usize = pos + 1;
        while (cur < src.len && src[cur] != 'e' as u8) {
            const nxt: i64 = parseValueEnd(src, cur, depth + 1);
            if (nxt < 0) {
                return -1;
            }
            cur = nxt as usize;
        }
        if (cur >= src.len || src[cur] != 'e' as u8) {
            return -1;
        }
        return (cur + 1) as i64;
    }
    if (c == 'd' as u8) {
        let cur: usize = pos + 1;
        let has_prev: bool = false;
        let prev_start: usize = 0;
        let prev_len: usize = 0;
        while (cur < src.len && src[cur] != 'e' as u8) {
            const k: BencodeBytesResult = decodeBytes(src, cur);
            if (!k.ok) {
                return -1;
            }
            if (has_prev) {
                const prev_slice: []const u8 = src[prev_start..(prev_start + prev_len)];
                const curr_slice: []const u8 = src[k.start..(k.start + k.len)];
                if (compareBytes(prev_slice, curr_slice) >= 0) {
                    return -1;
                }
            }
            has_prev = true;
            prev_start = k.start;
            prev_len = k.len;

            const val_end: i64 = parseValueEnd(src, k.next_pos, depth + 1);
            if (val_end < 0) {
                return -1;
            }
            cur = val_end as usize;
        }
        if (cur >= src.len || src[cur] != 'e' as u8) {
            return -1;
        }
        return (cur + 1) as i64;
    }
    return -1;
}

pub fn validate(src: []const u8) -> bool {
    const end_pos: i64 = parseValueEnd(src, 0, 0);
    return end_pos >= 0 && (end_pos as usize) == src.len;
}

pub fn dictLookup(src: []const u8, key: []const u8) -> BencodeBytesResult {
    const fail = BencodeBytesResult { start: 0, len: 0, next_pos: 0, ok: false };
    if (src.len < 2 || src[0] != 'd' as u8) {
        return fail;
    }
    let cur: usize = 1;
    while (cur < src.len && src[cur] != 'e' as u8) {
        const k: BencodeBytesResult = decodeBytes(src, cur);
        if (!k.ok) {
            return fail;
        }
        const val_start: usize = k.next_pos;
        const val_end: i64 = parseValueEnd(src, val_start, 1);
        if (val_end < 0) {
            return fail;
        }
        const key_slice: []const u8 = src[k.start..(k.start + k.len)];
        if (compareBytes(key_slice, key) == 0) {
            const end_u: usize = val_end as usize;
            return BencodeBytesResult {
                start: val_start,
                len: end_u - val_start,
                next_pos: end_u,
                ok: true,
            };
        }
        cur = val_end as usize;
    }
    return fail;
}
