// @cap/nibble-vec — Zero-heap 4-bit nibble vector for radix trees and prefix compression

pub const NIBBLE_MAX_BYTES: usize = 64;

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

    fn len(): usize {
        return _.nibble_len;
    }

    fn isEmpty(): bool {
        return _.nibble_len == 0;
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
}
