// @cap/nibble-vec — Zero-heap 4-bit nibble vector for radix trees and prefix compression
// Clean-room MIT implementation modeled on crates.io `nibble_vec`.

pub const NIBBLE_MAX_BYTES: usize = 64;
pub type Nibblet = NibbleVec;

fn nvecMaxNibbles(): usize {
    return NIBBLE_MAX_BYTES * 2;
}

fn nvecGetRaw(data: [64]u8, idx: usize): u8 {
    const byte_idx: usize = idx >> 1;
    const b: u8 = data[byte_idx];
    if ((idx & 1) == 0) {
        return (b >> 4) & 0x0F;
    }
    return b & 0x0F;
}

pub struct NibbleVec {
    data: [64]u8,
    nibble_len: usize,

    static fn new(): NibbleVec {
        let buf: [64]u8 = undefined;
        let i: usize = 0;
        while (i < NIBBLE_MAX_BYTES) : (i += 1) {
            buf[i] = 0;
        }
        return NibbleVec{ data: buf, nibble_len: 0 };
    }

    static fn fromBytes(bytes: []const u8): NibbleVec {
        let v = NibbleVec.new();
        let limit: usize = bytes.len;
        if (limit > NIBBLE_MAX_BYTES) {
            limit = NIBBLE_MAX_BYTES;
        }
        let i: usize = 0;
        while (i < limit) : (i += 1) {
            v.data[i] = bytes[i];
        }
        v.nibble_len = limit * 2;
        return v;
    }

    /// Upstream `nibble_vec::NibbleVec::from_byte_slice` constructor.
    static fn fromByteSlice(bytes: []const u8): NibbleVec {
        return NibbleVec.fromBytes(bytes);
    }

    fn len(): usize {
        return _.nibble_len;
    }

    fn isEmpty(): bool {
        return _.nibble_len == 0;
    }

    /// Number of backing bytes needed to hold all nibbles (`ceil(len / 2)`).
    fn byteLen(): usize {
        return (_.nibble_len + 1) >> 1;
    }

    /// Returns a read-only view of the packed backing bytes (`len = ceil(nibble_len / 2)`).
    fn asBytes(*_): []const u8 {
        const blen: usize = (_.nibble_len + 1) >> 1;
        return _.data[0..blen];
    }

    /// Copies the packed backing bytes into `dst` and returns the number of bytes copied.
    fn copyBytes(dst: []u8): usize {
        const blen: usize = (_.nibble_len + 1) >> 1;
        const n: usize = if (blen < dst.len) { blen } else { dst.len };
        let i: usize = 0;
        while (i < n) : (i += 1) {
            dst[i] = _.data[i];
        }
        return n;
    }

    fn push(*_, nib: u8): bool {
        if (_.nibble_len >= nvecMaxNibbles()) {
            return false;
        }
        const byte_idx: usize = _.nibble_len >> 1;
        const val: u8 = nib & 0x0F;
        if ((_.nibble_len & 1) == 0) {
            _.data[byte_idx] = (_.data[byte_idx] & 0x0F) | (val << 4);
        } else {
            _.data[byte_idx] = (_.data[byte_idx] & 0xF0) | val;
        }
        _.nibble_len += 1;
        return true;
    }

    fn pop(*_): u8? {
        if (_.nibble_len == 0) {
            return null;
        }
        _.nibble_len -= 1;
        const idx: usize = _.nibble_len;
        const byte_idx: usize = idx >> 1;
        const b: u8 = _.data[byte_idx];
        if ((idx & 1) == 0) {
            _.data[byte_idx] = b & 0x0F;
            return (b >> 4) & 0x0F;
        }
        _.data[byte_idx] = b & 0xF0;
        return b & 0x0F;
    }

    fn get(idx: usize): u8? {
        if (idx >= _.nibble_len) {
            return null;
        }
        return nvecGetRaw(_.data, idx);
    }

    fn set(*_, idx: usize, nib: u8): bool {
        if (idx >= _.nibble_len) {
            return false;
        }
        const byte_idx: usize = idx >> 1;
        const val: u8 = nib & 0x0F;
        if ((idx & 1) == 0) {
            _.data[byte_idx] = (_.data[byte_idx] & 0x0F) | (val << 4);
        } else {
            _.data[byte_idx] = (_.data[byte_idx] & 0xF0) | val;
        }
        return true;
    }

    /// Removes all nibbles from this vector.
    fn clear(*_) {
        let i: usize = 0;
        while (i < NIBBLE_MAX_BYTES) : (i += 1) {
            _.data[i] = 0;
        }
        _.nibble_len = 0;
    }

    /// Truncates the vector to at most `new_len` nibbles.
    fn truncate(*_, new_len: usize) {
        while (_.nibble_len > new_len) {
            * = _.pop();
        }
    }

    /// Splits the vector at `idx`: keeps `[0..idx)` in place and returns `[idx..len)` as a new `NibbleVec`.
    fn split(*_, idx: usize): NibbleVec {
        let tail = NibbleVec.new();
        if (idx >= _.nibble_len) {
            return tail;
        }
        let i: usize = idx;
        while (i < _.nibble_len) : (i += 1) {
            * = tail.push(nvecGetRaw(_.data, i));
        }
        _.truncate(idx);
        return tail;
    }

    /// Returns a new `NibbleVec` formed by concatenating `_` followed by `other`.
    fn join(other: NibbleVec): NibbleVec {
        let out = NibbleVec.new();
        let i: usize = 0;
        while (i < _.nibble_len) : (i += 1) {
            * = out.push(nvecGetRaw(_.data, i));
        }
        let j: usize = 0;
        while (j < other.nibble_len) : (j += 1) {
            if (!out.push(nvecGetRaw(other.data, j))) {
                return out;
            }
        }
        return out;
    }

    /// Appends all nibbles from `other` into `_`. Returns `false` if capacity was exceeded.
    fn extend(*_, other: NibbleVec): bool {
        let j: usize = 0;
        while (j < other.nibble_len) : (j += 1) {
            if (!_.push(nvecGetRaw(other.data, j))) {
                return false;
            }
        }
        return true;
    }

    /// Returns a sub-vector containing nibbles `[start..end)`.
    fn slice(start: usize, end: usize): NibbleVec {
        let out = NibbleVec.new();
        let s: usize = start;
        let e: usize = if (end > _.nibble_len) { _.nibble_len } else { end };
        while (s < e) : (s += 1) {
            * = out.push(nvecGetRaw(_.data, s));
        }
        return out;
    }

    fn commonPrefixLen(other: NibbleVec): usize {
        let min_len: usize = _.nibble_len;
        if (other.nibble_len < min_len) {
            min_len = other.nibble_len;
        }
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if (nvecGetRaw(_.data, i) != nvecGetRaw(other.data, i)) {
                return i;
            }
        }
        return min_len;
    }

    /// Returns `true` if `_` begins with all nibbles of `prefix`.
    fn hasPrefix(prefix: NibbleVec): bool {
        return _.commonPrefixLen(prefix) == prefix.nibble_len;
    }

    /// Returns `true` if `_` and `other` have identical length and nibble contents.
    fn equals(other: NibbleVec): bool {
        if (_.nibble_len != other.nibble_len) {
            return false;
        }
        return _.commonPrefixLen(other) == _.nibble_len;
    }
}
