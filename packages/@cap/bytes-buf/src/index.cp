// @cap/bytes-buf — Zero-copy byte buffer read/write cursors (Buf & BufMut)
// Clean-room MIT CP implementation of crates.io `bytes` (1.8B+ downloads)
// Zero-heap, bounds-checked big-endian & little-endian integer reading/writing.

pub const BYTES_MUT_CAP: usize = 512;

pub struct ReadU8Result { pub ok: bool, pub val: u8 }
pub struct ReadU16Result { pub ok: bool, pub val: u16 }
pub struct ReadU32Result { pub ok: bool, pub val: u32 }
pub struct ReadU64Result { pub ok: bool, pub val: u64 }

/// Zero-copy read cursor over an immutable byte slice (mirrors `bytes::Buf`).
pub struct BytesReader {
    data: []const u8,
    pos: usize,

    pub static fn new(data: []const u8): BytesReader {
        return BytesReader { data: data, pos: 0 };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) { return 0; }
        return _.data.len - _.pos;
    }

    pub fn hasRemaining(): bool {
        return _.pos < _.data.len;
    }

    pub fn position(): usize {
        return _.pos;
    }

    pub fn advance(*_, cnt: usize): bool {
        if (cnt > _.remaining()) { return false; }
        _.pos = _.pos + cnt;
        return true;
    }

    pub fn getU8(*_): ReadU8Result {
        if (_.remaining() < 1) { return ReadU8Result { ok: false, val: 0 }; }
        let v = _.data[_.pos];
        _.pos = _.pos + 1;
        return ReadU8Result { ok: true, val: v };
    }

    pub fn getU16Be(*_): ReadU16Result {
        if (_.remaining() < 2) { return ReadU16Result { ok: false, val: 0 }; }
        let p = _.pos;
        let v = ((_.data[p] as u16) << 8) | (_.data[p + 1] as u16);
        _.pos = p + 2;
        return ReadU16Result { ok: true, val: v };
    }

    pub fn getU16Le(*_): ReadU16Result {
        if (_.remaining() < 2) { return ReadU16Result { ok: false, val: 0 }; }
        let p = _.pos;
        let v = (_.data[p] as u16) | ((_.data[p + 1] as u16) << 8);
        _.pos = p + 2;
        return ReadU16Result { ok: true, val: v };
    }

    pub fn getU32Be(*_): ReadU32Result {
        if (_.remaining() < 4) { return ReadU32Result { ok: false, val: 0 }; }
        let p = _.pos;
        let v = ((_.data[p] as u32) << 24)
            | ((_.data[p + 1] as u32) << 16)
            | ((_.data[p + 2] as u32) << 8)
            | (_.data[p + 3] as u32);
        _.pos = p + 4;
        return ReadU32Result { ok: true, val: v };
    }

    pub fn getU32Le(*_): ReadU32Result {
        if (_.remaining() < 4) { return ReadU32Result { ok: false, val: 0 }; }
        let p = _.pos;
        let v = (_.data[p] as u32)
            | ((_.data[p + 1] as u32) << 8)
            | ((_.data[p + 2] as u32) << 16)
            | ((_.data[p + 3] as u32) << 24);
        _.pos = p + 4;
        return ReadU32Result { ok: true, val: v };
    }

    pub fn getU64Be(*_): ReadU64Result {
        if (_.remaining() < 8) { return ReadU64Result { ok: false, val: 0 }; }
        let hi = _.getU32Be();
        let lo = _.getU32Be();
        return ReadU64Result { ok: true, val: ((hi.val as u64) << 32) | (lo.val as u64) };
    }

    pub fn getU64Le(*_): ReadU64Result {
        if (_.remaining() < 8) { return ReadU64Result { ok: false, val: 0 }; }
        let lo = _.getU32Le();
        let hi = _.getU32Le();
        return ReadU64Result { ok: true, val: ((hi.val as u64) << 32) | (lo.val as u64) };
    }

    pub fn copyToSlice(*_, dst: []u8): bool {
        if (dst.len > _.remaining()) { return false; }
        let i: usize = 0;
        while (i < dst.len) {
            dst[i] = _.data[_.pos + i];
            i = i + 1;
        }
        _.pos = _.pos + dst.len;
        return true;
    }
}

/// Zero-heap mutable byte buffer (capacity 512, mirrors `bytes::BytesMut` / `BufMut`).
pub struct BytesMut {
    buf: [512]u8,
    used: usize,

    pub static fn new(): BytesMut {
        return BytesMut { buf: undefined, used: 0 };
    }

    pub fn len(): usize { return _.used; }
    pub fn capacity(): usize { return BYTES_MUT_CAP; }
    pub fn remainingMut(): usize { return BYTES_MUT_CAP - _.used; }
    pub fn isEmpty(): bool { return _.used == 0; }

    pub fn clear(*_) { _.used = 0; }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.used) { _.used = new_len; }
    }

    pub fn byteAt(idx: usize): u8 {
        if (idx >= _.used) { return 0; }
        return _.buf[idx];
    }

    pub fn putU8(*_, v: u8): bool {
        if (_.used + 1 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = v;
        _.used = _.used + 1;
        return true;
    }

    pub fn putU16Be(*_, v: u16): bool {
        if (_.used + 2 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.used + 1] = (v & 0xFF) as u8;
        _.used = _.used + 2;
        return true;
    }

    pub fn putU16Le(*_, v: u16): bool {
        if (_.used + 2 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = (v & 0xFF) as u8;
        _.buf[_.used + 1] = ((v >> 8) & 0xFF) as u8;
        _.used = _.used + 2;
        return true;
    }

    pub fn putU32Be(*_, v: u32): bool {
        if (_.used + 4 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.used + 1] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.used + 2] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.used + 3] = (v & 0xFF) as u8;
        _.used = _.used + 4;
        return true;
    }

    pub fn putU32Le(*_, v: u32): bool {
        if (_.used + 4 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = (v & 0xFF) as u8;
        _.buf[_.used + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.used + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.used + 3] = ((v >> 24) & 0xFF) as u8;
        _.used = _.used + 4;
        return true;
    }

    pub fn putU64Be(*_, v: u64): bool {
        if (_.used + 8 > BYTES_MUT_CAP) { return false; }
        * = _.putU32Be(((v >> 32) & 0xFFFFFFFF) as u32);
        * = _.putU32Be((v & 0xFFFFFFFF) as u32);
        return true;
    }

    pub fn putU64Le(*_, v: u64): bool {
        if (_.used + 8 > BYTES_MUT_CAP) { return false; }
        * = _.putU32Le((v & 0xFFFFFFFF) as u32);
        * = _.putU32Le(((v >> 32) & 0xFFFFFFFF) as u32);
        return true;
    }

    pub fn putSlice(*_, src: []const u8): bool {
        if (_.used + src.len > BYTES_MUT_CAP) { return false; }
        let i: usize = 0;
        while (i < src.len) {
            _.buf[_.used + i] = src[i];
            i = i + 1;
        }
        _.used = _.used + src.len;
        return true;
    }

    /// Copies the first `at` bytes into `out`, shifting the remaining bytes to the front (mirrors `split_to`).
    pub fn splitTo(*_, at: usize, out: []u8): i64 {
        if (at > _.used || at > out.len) { return -1; }
        let i: usize = 0;
        while (i < at) {
            out[i] = _.buf[i];
            i = i + 1;
        }
        let rem = _.used - at;
        let j: usize = 0;
        while (j < rem) {
            _.buf[j] = _.buf[at + j];
            j = j + 1;
        }
        _.used = rem;
        return at as i64;
    }

    /// Copies all written bytes into `out` and returns written length.
    pub fn copyAll(out: []u8): usize {
        let n = if (_.used <= out.len) { _.used } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.buf[i];
            i = i + 1;
        }
        return n;
    }
}
