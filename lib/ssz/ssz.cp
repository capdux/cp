// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Ethereum SSZ Deterministic Codec)

pub const SSZ_OFFSET_BYTES: usize = 4;
pub const SSZ_CHUNK_BYTES: usize = 32;

fn readU32LEAt(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

pub struct SszWriter {
    buf: []u8,
    pos: usize,

    fn init(buf: []u8): SszWriter {
        return SszWriter{
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

    fn writeU8(*_, v: u8): bool {
        if (_.remaining() < 1) {
            return false;
        }
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeBool(*_, v: bool): bool {
        if (v) {
            return _.writeU8(1);
        }
        return _.writeU8(0);
    }

    fn writeU16(*_, v: u16): bool {
        if (_.remaining() < 2) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    fn writeU32(*_, v: u32): bool {
        if (_.remaining() < 4) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    fn writeU64(*_, v: u64): bool {
        if (_.remaining() < 8) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((v >> 32) & 0xFF) as u8;
        _.buf[_.pos + 5] = ((v >> 40) & 0xFF) as u8;
        _.buf[_.pos + 6] = ((v >> 48) & 0xFF) as u8;
        _.buf[_.pos + 7] = ((v >> 56) & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    fn writeU128(*_, v: u128): bool {
        if (_.remaining() < 16) {
            return false;
        }
        const lo: u64 = (v & (0xFFFF_FFFF_FFFF_FFFF as u128)) as u64;
        const hi: u64 = (v >> 64) as u64;
        if (!_.writeU64(lo)) {
            return false;
        }
        return _.writeU64(hi);
    }

    fn writeOffset(*_, off: u32): bool {
        return _.writeU32(off);
    }

    fn writeFixedBytes(*_, data: []const u8): bool {
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

    fn writeZeroPad(*_, n: usize): bool {
        if (n > _.remaining()) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            _.buf[_.pos + i] = 0;
            i += 1;
        }
        _.pos += n;
        return true;
    }

    fn writeBitlist(*_, bits: []const bool): bool {
        const needed_bytes: usize = (bits.len >> 3) + 1;
        if (needed_bytes > _.remaining()) {
            return false;
        }
        const start = _.pos;
        let k: usize = 0;
        while (k < needed_bytes) {
            _.buf[start + k] = 0;
            k += 1;
        }
        let i: usize = 0;
        while (i < bits.len) {
            if (bits[i]) {
                const shift: u3 = (i & 7) as u3;
                _.buf[start + (i >> 3)] = _.buf[start + (i >> 3)] | ((1 as u8) << shift);
            }
            i += 1;
        }
        // Append SSZ delimiter bit at position bits.len
        const delim_shift: u3 = (bits.len & 7) as u3;
        _.buf[start + (bits.len >> 3)] = _.buf[start + (bits.len >> 3)] | ((1 as u8) << delim_shift);
        _.pos += needed_bytes;
        return true;
    }

    fn writeBitvector(*_, bits: []const bool): bool {
        let needed_bytes: usize = 0;
        if (bits.len > 0) {
            needed_bytes = ((bits.len - 1) >> 3) + 1;
        }
        if (needed_bytes > _.remaining()) {
            return false;
        }
        const start = _.pos;
        let k: usize = 0;
        while (k < needed_bytes) {
            _.buf[start + k] = 0;
            k += 1;
        }
        let i: usize = 0;
        while (i < bits.len) {
            if (bits[i]) {
                const shift: u3 = (i & 7) as u3;
                _.buf[start + (i >> 3)] = _.buf[start + (i >> 3)] | ((1 as u8) << shift);
            }
            i += 1;
        }
        _.pos += needed_bytes;
        return true;
    }
}

pub struct SszReader {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): SszReader {
        return SszReader{
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

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn readU8(*_): u8? {
        if (_.remaining() < 1) {
            return null;
        }
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readBool(*_): bool? {
        const raw = _.readU8() ?? return null;
        if (raw == 0) return false;
        if (raw == 1) return true;
        return null;
    }

    fn readU16(*_): u16? {
        if (_.remaining() < 2) {
            return null;
        }
        const b0 = _.data[_.pos] as u16;
        const b1 = _.data[_.pos + 1] as u16;
        _.pos += 2;
        return b0 | (b1 << 8);
    }

    fn readU32(*_): u32? {
        if (_.remaining() < 4) {
            return null;
        }
        const v = readU32LEAt(_.data, _.pos);
        _.pos += 4;
        return v;
    }

    fn readU64(*_): u64? {
        if (_.remaining() < 8) {
            return null;
        }
        const lo = readU32LEAt(_.data, _.pos) as u64;
        const hi = readU32LEAt(_.data, _.pos + 4) as u64;
        _.pos += 8;
        return lo | (hi << 32);
    }

    fn readU128(*_): u128? {
        if (_.remaining() < 16) {
            return null;
        }
        const lo = _.readU64() ?? return null;
        const hi = _.readU64() ?? return null;
        return (lo as u128) | ((hi as u128) << 64);
    }

    fn readOffset(*_): u32? {
        return _.readU32();
    }

    fn readFixedBytes(*_, n: usize): ([]const u8)? {
        if (n > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }
}

pub fn sszBitlistLen(raw: []const u8): usize? {
    if (raw.len == 0) {
        return null;
    }
    const last = raw[raw.len - 1];
    if (last == 0) {
        return null;
    }
    let msb: u8 = 7;
    while (true) {
        const shift: u3 = msb as u3;
        if ((last & ((1 as u8) << shift)) != 0) {
            return (raw.len - 1) * 8 + (msb as usize);
        }
        if (msb == 0) {
            break;
        }
        msb -= 1;
    }
    return null;
}

pub fn sszBitlistGet(raw: []const u8, idx: usize): bool? {
    const bit_len = sszBitlistLen(raw) ?? return null;
    if (idx >= bit_len) {
        return null;
    }
    const shift: u3 = (idx & 7) as u3;
    return (raw[idx >> 3] & ((1 as u8) << shift)) != 0;
}

pub fn sszBitvectorValid(raw: []const u8, bit_count: usize): bool {
    let expected_bytes: usize = 0;
    if (bit_count > 0) {
        expected_bytes = ((bit_count - 1) >> 3) + 1;
    }
    if (raw.len != expected_bytes) {
        return false;
    }
    const rem_bits = bit_count & 7;
    if (rem_bits != 0) {
        const shift: u3 = rem_bits as u3;
        if ((raw[raw.len - 1] >> shift) != 0) {
            return false;
        }
    }
    return true;
}

pub fn sszBitvectorGet(raw: []const u8, bit_count: usize, idx: usize): bool? {
    if (!sszBitvectorValid(raw, bit_count)) {
        return null;
    }
    if (idx >= bit_count) {
        return null;
    }
    const shift: u3 = (idx & 7) as u3;
    return (raw[idx >> 3] & ((1 as u8) << shift)) != 0;
}

pub struct SszOffsetListIter {
    data: []const u8,
    count: usize,
    index: usize,
    valid: bool,

    fn init(data: []const u8): SszOffsetListIter {
        if (data.len == 0) {
            return SszOffsetListIter{
                .data = data,
                .count = 0,
                .index = 0,
                .valid = true,
            };
        }
        if (data.len < SSZ_OFFSET_BYTES) {
            return SszOffsetListIter{
                .data = data,
                .count = 0,
                .index = 0,
                .valid = false,
            };
        }
        const first_off: usize = readU32LEAt(data, 0) as usize;
        if (first_off < SSZ_OFFSET_BYTES || (first_off % SSZ_OFFSET_BYTES) != 0 || first_off > data.len) {
            return SszOffsetListIter{
                .data = data,
                .count = 0,
                .index = 0,
                .valid = false,
            };
        }

        const count = first_off / SSZ_OFFSET_BYTES;
        let prev = first_off;
        let i: usize = 1;
        while (i < count) {
            const cur: usize = readU32LEAt(data, i * SSZ_OFFSET_BYTES) as usize;
            if (cur < prev || cur > data.len) {
                return SszOffsetListIter{
                    .data = data,
                    .count = 0,
                    .index = 0,
                    .valid = false,
                };
            }
            prev = cur;
            i += 1;
        }

        return SszOffsetListIter{
            .data = data,
            .count = count,
            .index = 0,
            .valid = true,
        };
    }

    fn isValid(): bool {
        return _.valid;
    }

    fn len(): usize {
        return _.count;
    }

    fn next(*_): ([]const u8)? {
        if (!_.valid || _.index >= _.count) {
            return null;
        }
        const start: usize = readU32LEAt(_.data, _.index * SSZ_OFFSET_BYTES) as usize;
        let end: usize = _.data.len;
        if (_.index + 1 < _.count) {
            end = readU32LEAt(_.data, (_.index + 1) * SSZ_OFFSET_BYTES) as usize;
        }
        _.index += 1;
        return _.data[start..end];
    }
}

pub fn sszChunkCount(byte_len: usize): usize {
    if (byte_len == 0) {
        return 0;
    }
    return ((byte_len - 1) / SSZ_CHUNK_BYTES) + 1;
}

pub fn sszNextPowerOfTwo(n: usize): usize {
    if (n <= 1) {
        return 1;
    }
    let p: usize = 1;
    while (p < n) {
        if (p > (0x7FFF_FFFF_FFFF_FFFF as usize)) {
            return p;
        }
        p = p << 1;
    }
    return p;
}
