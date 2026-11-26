// @cap/bitstream — Zero-allocation MSB-first & LSB-first BitReader & BitWriter for CP
// Clean-room MIT implementation inspired by Rust `bitstream-io`.
//
// Features:
// - Dual bit ordering: MSB-first (network / H.264 / JPEG) & LSB-first (DEFLATE / WebP / GIF).
// - Full 0..=64-bit arbitrary unaligned reads & writes with strict bounds checks.
// - Non-destructive `peekBits`, `skipBits`, `alignToByte`, and 0-th order unsigned Exp-Golomb
//   coding (`writeUe` / `readUe` per ITU-T H.264 / HEVC §9.1).
// - Zero heap allocation operating directly on caller slices.

pub struct BitReadResult {
    pub value: u64,
    pub ok: bool,
}

/// Zero-allocation streaming bit reader supporting both MSB-first and LSB-first bit orders.
pub struct BitReader {
    pub buf: []const u8,
    pub bit_pos: usize,
    pub msb_first: bool,

    pub static fn initMsb(buf: []const u8) -> BitReader {
        return BitReader{ buf: buf, bit_pos: 0, msb_first: true };
    }

    pub static fn initLsb(buf: []const u8) -> BitReader {
        return BitReader{ buf: buf, bit_pos: 0, msb_first: false };
    }

    pub fn bitPosition() -> usize {
        return _.bit_pos;
    }

    pub fn bitsRemaining() -> usize {
        const total = _.buf.len * 8;
        if (_.bit_pos >= total) {
            return 0;
        }
        return total - _.bit_pos;
    }

    fn readBitAt(pos: usize) -> u8 {
        const byte_idx = pos >> 3;
        const bit_in_byte = (pos & 7) as u3;
        if (_.msb_first) {
            const shift = (7 as u3) - bit_in_byte;
            return (_.buf[byte_idx] >> shift) & 1;
        }
        return (_.buf[byte_idx] >> bit_in_byte) & 1;
    }

    /// Reads a single bit (`0` or `1`) and advances the cursor by 1 bit.
    pub fn readBit(*_) -> BitReadResult {
        if (_.bitsRemaining() == 0) {
            return BitReadResult{ value: 0, ok: false };
        }
        const b = _.readBitAt(_.bit_pos);
        _.bit_pos += 1;
        return BitReadResult{ value: b as u64, ok: true };
    }

    /// Non-destructively peeks `count` (`0..=64`) bits without advancing the cursor.
    pub fn peekBits(count: u8) -> BitReadResult {
        if (count > 64 || (count as usize) > _.bitsRemaining()) {
            return BitReadResult{ value: 0, ok: false };
        }
        if (count == 0) {
            return BitReadResult{ value: 0, ok: true };
        }

        let val: u64 = 0;
        let i: u8 = 0;
        while (i < count) {
            const b = _.readBitAt(_.bit_pos + (i as usize)) as u64;
            if (_.msb_first) {
                val = (val << 1) | b;
            } else {
                val = val | (b << (i as u6));
            }
            i += 1;
        }
        return BitReadResult{ value: val, ok: true };
    }

    /// Reads `count` (`0..=64`) bits and advances the cursor.
    pub fn readBits(*_, count: u8) -> BitReadResult {
        const res = _.peekBits(count);
        if (!res.ok) {
            return res;
        }
        _.bit_pos += count as usize;
        return res;
    }

    /// Advances the cursor by `count` bits. Returns `false` if fewer than `count` bits remain.
    pub fn skipBits(*_, count: usize) -> bool {
        if (count > _.bitsRemaining()) {
            return false;
        }
        _.bit_pos += count;
        return true;
    }

    /// Advances the cursor to the next whole-byte boundary (no-op if already aligned).
    pub fn alignToByte(*_) {
        const rem = _.bit_pos & 7;
        if (rem != 0) {
            _.bit_pos += (8 as usize) - rem;
        }
    }

    /// Decodes a 0-th order unsigned Exp-Golomb code `ue(v)` (H.264 / HEVC §9.1).
    pub fn readUe(*_) -> BitReadResult {
        let leading_zeros: u8 = 0;
        while (true) {
            const b = _.readBit();
            if (!b.ok) {
                return BitReadResult{ value: 0, ok: false };
            }
            if (b.value == 1) {
                break;
            }
            leading_zeros += 1;
            if (leading_zeros > 62) {
                return BitReadResult{ value: 0, ok: false };
            }
        }
        if (leading_zeros == 0) {
            return BitReadResult{ value: 0, ok: true };
        }
        // Read suffix bits MSB-first regardless of reader mode so ue(v) is self-consistent
        if ((leading_zeros as usize) > _.bitsRemaining()) {
            return BitReadResult{ value: 0, ok: false };
        }
        let suffix: u64 = 0;
        let i: u8 = 0;
        while (i < leading_zeros) {
            const b = _.readBit();
            if (!b.ok) {
                return BitReadResult{ value: 0, ok: false };
            }
            suffix = (suffix << 1) | b.value;
            i += 1;
        }
        const base: u64 = ((1 as u64) << (leading_zeros as u6)) - 1;
        return BitReadResult{ value: base + suffix, ok: true };
    }
}

/// Zero-allocation streaming bit writer supporting both MSB-first and LSB-first bit orders.
pub struct BitWriter {
    pub buf: []u8,
    pub bit_pos: usize,
    pub msb_first: bool,

    pub static fn initMsb(buf: []u8) -> BitWriter {
        return BitWriter{ buf: buf, bit_pos: 0, msb_first: true };
    }

    pub static fn initLsb(buf: []u8) -> BitWriter {
        return BitWriter{ buf: buf, bit_pos: 0, msb_first: false };
    }

    pub fn bitsWritten() -> usize {
        return _.bit_pos;
    }

    pub fn bytesUsed() -> usize {
        return (_.bit_pos + 7) >> 3;
    }

    /// Writes a single bit (`bit & 1`). Automatically zeroes each newly entered output byte.
    pub fn writeBit(*_, bit: u8) -> bool {
        if (_.bit_pos >= _.buf.len * 8) {
            return false;
        }
        const byte_idx = _.bit_pos >> 3;
        const bit_in_byte = (_.bit_pos & 7) as u3;
        if (bit_in_byte == 0) {
            _.buf[byte_idx] = 0;
        }
        if ((bit & 1) != 0) {
            if (_.msb_first) {
                const shift = (7 as u3) - bit_in_byte;
                _.buf[byte_idx] = _.buf[byte_idx] | ((1 as u8) << shift);
            } else {
                _.buf[byte_idx] = _.buf[byte_idx] | ((1 as u8) << bit_in_byte);
            }
        }
        _.bit_pos += 1;
        return true;
    }

    /// Writes the lowest `count` (`0..=64`) bits of `value`. Returns `false` if buffer is too small.
    pub fn writeBits(*_, value: u64, count: u8) -> bool {
        if (count > 64) {
            return false;
        }
        const total = _.buf.len * 8;
        if (_.bit_pos > total || (count as usize) > total - _.bit_pos) {
            return false;
        }
        if (count == 0) {
            return true;
        }

        if (_.msb_first) {
            let i: u8 = count;
            while (i > 0) {
                i -= 1;
                const b = ((value >> (i as u6)) & 1) as u8;
                if (!_.writeBit(b)) {
                    return false;
                }
            }
        } else {
            let i: u8 = 0;
            while (i < count) {
                const b = ((value >> (i as u6)) & 1) as u8;
                if (!_.writeBit(b)) {
                    return false;
                }
                i += 1;
            }
        }
        return true;
    }

    /// Pads with zero bits to reach the next whole-byte boundary.
    pub fn alignToByte(*_) -> bool {
        const rem = _.bit_pos & 7;
        if (rem == 0) {
            return true;
        }
        const pad = (8 as usize) - rem;
        let i: usize = 0;
        while (i < pad) {
            if (!_.writeBit(0 as u8)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Encodes `value` as a 0-th order unsigned Exp-Golomb code `ue(v)` (H.264 / HEVC §9.1).
    pub fn writeUe(*_, value: u64) -> bool {
        if (value == (0xFFFFFFFFFFFFFFFF as u64)) {
            return false;
        }
        const code_num: u64 = value + 1;
        let bits: u8 = 0;
        let tmp = code_num;
        while (tmp > 0) {
            bits += 1;
            tmp = tmp >> 1;
        }
        const leading_zeros: u8 = bits - 1;
        const needed: usize = (leading_zeros as usize) + (bits as usize);
        const total = _.buf.len * 8;
        if (_.bit_pos > total || needed > total - _.bit_pos) {
            return false;
        }

        let z: u8 = 0;
        while (z < leading_zeros) {
            if (!_.writeBit(0 as u8)) {
                return false;
            }
            z += 1;
        }
        let i: u8 = bits;
        while (i > 0) {
            i -= 1;
            const b = ((code_num >> (i as u6)) & 1) as u8;
            if (!_.writeBit(b)) {
                return false;
            }
        }
        return true;
    }
}
