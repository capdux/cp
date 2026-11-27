// Origin: Pure-CP Stdlib (Zero-Alloc Universal Prefix Integer Codes & Bitstream Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn msbMask(bit_in_byte: usize): u8 {
    if (bit_in_byte == 0) { return 0x80; }
    if (bit_in_byte == 1) { return 0x40; }
    if (bit_in_byte == 2) { return 0x20; }
    if (bit_in_byte == 3) { return 0x10; }
    if (bit_in_byte == 4) { return 0x08; }
    if (bit_in_byte == 5) { return 0x04; }
    if (bit_in_byte == 6) { return 0x02; }
    return 0x01;
}

/// Returns the number of significant bits in `n` (`0 -> 0`, `1 -> 1`, `2..3 -> 2`, ..., `2^63.. -> 64`).
fn bitLengthU64(n: u64): u8 {
    let v = n;
    let len: u8 = 0;
    while (v != 0) {
        v = v >> 1;
        len += 1;
    }
    return len;
}

fn fillFibTable(fib: []u64) {
    fib[0] = 1;
    fib[1] = 2;
    let i: usize = 2;
    while (i < 92) {
        fib[i] = fib[i - 1] + fib[i - 2];
        i += 1;
    }
}

// ---------------------------------------------------------------------------
// MSB-First Zero-Allocation BitWriter & BitReader
// ---------------------------------------------------------------------------

/// MSB-first zero-allocation bit writer over a caller-supplied byte slice.
pub struct BitWriter {
    buf: []u8,
    bit_pos: usize,

    static fn init(buf: []u8): BitWriter {
        return BitWriter {
            buf: buf,
            bit_pos: 0,
        };
    }

    fn writeBit(*_, b: u1): bool {
        const byte_idx = _.bit_pos / 8;
        const bit_in_byte = _.bit_pos % 8;
        if (byte_idx >= _.buf.len) {
            return false;
        }
        if (bit_in_byte == 0) {
            _.buf[byte_idx] = 0;
        }
        if (b == 1) {
            _.buf[byte_idx] = _.buf[byte_idx] | msbMask(bit_in_byte);
        }
        _.bit_pos += 1;
        return true;
    }

    /// Writes the low `n` bits (`0..64`) of `val` in MSB-first order.
    fn writeBits(*_, val: u64, n: u8): bool {
        if (n > 64) {
            return false;
        }
        let rem: u8 = n;
        while (rem > 0) {
            rem -= 1;
            const b: u1 = ((val >> (rem as u6)) & 1) as u1;
            if (!_.writeBit(b)) {
                return false;
            }
        }
        return true;
    }

    fn writeZeros(*_, n: u8): bool {
        let i: u8 = 0;
        while (i < n) {
            if (!_.writeBit(0)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn bytesUsed(): usize {
        return (_.bit_pos + 7) / 8;
    }

    fn flush(): []const u8 {
        return _.buf[0.._.bytesUsed()];
    }
}

/// MSB-first zero-allocation bit reader over an immutable byte slice.
pub struct BitReader {
    data: []const u8,
    bit_pos: usize,
    bit_len: usize,

    static fn init(data: []const u8): BitReader {
        return BitReader {
            data: data,
            bit_pos: 0,
            bit_len: data.len * 8,
        };
    }

    fn bitsRemaining(): usize {
        return _.bit_len - _.bit_pos;
    }

    fn readBit(*_): u1? {
        if (_.bit_pos >= _.bit_len) {
            return null;
        }
        const byte_idx = _.bit_pos / 8;
        const bit_in_byte = _.bit_pos % 8;
        _.bit_pos += 1;
        const b: u1 = if ((_.data[byte_idx] & msbMask(bit_in_byte)) != 0) 1 else 0;
        return b;
    }

    /// Reads `n` bits (`0..64`) in MSB-first order.
    fn readBits(*_, n: u8): u64? {
        if (n > 64 || (n as usize) > _.bitsRemaining()) {
            return null;
        }
        let acc: u64 = 0;
        let i: u8 = 0;
        while (i < n) {
            const b = _.readBit() ?? return null;
            acc = (acc << 1) | (b as u64);
            i += 1;
        }
        return acc;
    }
}

// ---------------------------------------------------------------------------
// Elias Universal Codes (Positive Integers n >= 1)
// ---------------------------------------------------------------------------

/// Encodes positive integer `n >= 1` using Elias Gamma coding.
pub fn writeEliasGamma(bw: *BitWriter, n: u64): bool {
    if (n == 0) {
        return false;
    }
    const blen = bitLengthU64(n);
    const zeros: u8 = blen - 1;
    if (!bw.writeZeros(zeros)) {
        return false;
    }
    return bw.writeBits(n, blen);
}

/// Decodes a positive integer (`n >= 1`) encoded with Elias Gamma coding.
pub fn readEliasGamma(br: *BitReader): u64? {
    let zeros: u8 = 0;
    while (true) {
        const b = br.readBit() ?? return null;
        if (b == 1) {
            break;
        }
        zeros += 1;
        if (zeros > 63) {
            return null;
        }
    }
    if (zeros == 0) {
        return 1;
    }
    const rem = br.readBits(zeros) ?? return null;
    return ((1 as u64) << (zeros as u6)) | rem;
}

/// Encodes positive integer `n >= 1` using Elias Delta coding.
pub fn writeEliasDelta(bw: *BitWriter, n: u64): bool {
    if (n == 0) {
        return false;
    }
    const blen = bitLengthU64(n);
    if (!writeEliasGamma(bw, blen as u64)) {
        return false;
    }
    return bw.writeBits(n, blen - 1);
}

/// Decodes a positive integer (`n >= 1`) encoded with Elias Delta coding.
pub fn readEliasDelta(br: *BitReader): u64? {
    const blen_u64 = readEliasGamma(br) ?? return null;
    if (blen_u64 == 0 || blen_u64 > 64) {
        return null;
    }
    const blen: u8 = blen_u64 as u8;
    if (blen == 1) {
        return 1;
    }
    const rem_bits: u8 = blen - 1;
    const rem = br.readBits(rem_bits) ?? return null;
    return ((1 as u64) << (rem_bits as u6)) | rem;
}

/// Encodes positive integer `n >= 1` using Elias Omega coding.
pub fn writeEliasOmega(bw: *BitWriter, n: u64): bool {
    if (n == 0) {
        return false;
    }
    let groups: [8]u64 = [0; 8];
    let lens: [8]u8 = [0; 8];
    let count: usize = 0;

    let cur = n;
    while (cur > 1) {
        const blen = bitLengthU64(cur);
        groups[count] = cur;
        lens[count] = blen;
        count += 1;
        cur = (blen - 1) as u64;
    }

    while (count > 0) {
        count -= 1;
        if (!bw.writeBits(groups[count], lens[count])) {
            return false;
        }
    }
    return bw.writeBit(0);
}

/// Decodes a positive integer (`n >= 1`) encoded with Elias Omega coding.
pub fn readEliasOmega(br: *BitReader): u64? {
    let n: u64 = 1;
    while (true) {
        const b = br.readBit() ?? return null;
        if (b == 0) {
            return n;
        }
        if (n > 63) {
            return null;
        }
        const rem = br.readBits(n as u8) ?? return null;
        n = ((1 as u64) << (n as u6)) | rem;
    }
}

// ---------------------------------------------------------------------------
// Exp-Golomb Codes (H.264 / HEVC ue(v) and se(v))
// ---------------------------------------------------------------------------

/// Encodes unsigned integer `v` using 0-order Exp-Golomb coding `ue(v)`.
pub fn writeExpGolombU(bw: *BitWriter, v: u64): bool {
    if (v == 0xFFFFFFFFFFFFFFFF) {
        return false;
    }
    return writeEliasGamma(bw, v + 1);
}

/// Decodes unsigned integer `ue(v)`.
pub fn readExpGolombU(br: *BitReader): u64? {
    const g = readEliasGamma(br) ?? return null;
    return g - 1;
}

/// Encodes signed integer `v` using H.264/HEVC Exp-Golomb coding `se(v)`
/// (`0->0, 1->1, -1->2, 2->3, -2->4, ...`).
pub fn writeExpGolombS(bw: *BitWriter, v: i64): bool {
    let code: u64 = 0;
    if (v > 0) {
        const mag = v as u64;
        if (mag > 0x7FFFFFFFFFFFFFFF) {
            return false;
        }
        code = (mag * 2) - 1;
    } else if (v < 0) {
        // Compute |v| safely without overflowing i64::MIN
        const mag: u64 = (-(v + 1) as u64) + 1;
        if (mag > 0x7FFFFFFFFFFFFFFF) {
            return false;
        }
        code = mag * 2;
    }
    return writeExpGolombU(bw, code);
}

/// Decodes signed integer `se(v)`.
pub fn readExpGolombS(br: *BitReader): i64? {
    const code = readExpGolombU(br) ?? return null;
    if (code == 0) {
        return 0;
    }
    if ((code & 1) == 1) {
        return ((code + 1) / 2) as i64;
    }
    const mag = (code / 2) as i64;
    return -mag;
}

// ---------------------------------------------------------------------------
// Golomb-Rice Coding (k = 0..62)
// ---------------------------------------------------------------------------

/// Encodes unsigned integer `v` using Golomb-Rice coding with parameter `k` (`0..62`).
/// Writes unary quotient (`q` zero bits followed by a `1` bit) and `k`-bit remainder.
pub fn writeRice(bw: *BitWriter, v: u64, k: u8): bool {
    if (k > 62) {
        return false;
    }
    const q: u64 = v >> (k as u6);
    const rem_mask: u64 = if (k == 0) 0 else (((1 as u64) << (k as u6)) - 1);
    const r: u64 = v & rem_mask;

    let i: u64 = 0;
    while (i < q) {
        if (!bw.writeBit(0)) {
            return false;
        }
        i += 1;
    }
    if (!bw.writeBit(1)) {
        return false;
    }
    return bw.writeBits(r, k);
}

/// Decodes a Golomb-Rice encoded unsigned integer with parameter `k` (`0..62`).
pub fn readRice(br: *BitReader, k: u8): u64? {
    if (k > 62) {
        return null;
    }
    const max_q: u64 = (0xFFFFFFFFFFFFFFFF as u64) >> (k as u6);
    let q: u64 = 0;
    while (true) {
        const b = br.readBit() ?? return null;
        if (b == 1) {
            break;
        }
        if (q >= max_q) {
            return null;
        }
        q += 1;
    }
    const r = br.readBits(k) ?? return null;
    return (q << (k as u6)) | r;
}

// ---------------------------------------------------------------------------
// Fibonacci Zeckendorf Coding (n >= 1, self-synchronizing `11` terminator)
// ---------------------------------------------------------------------------

/// Encodes positive integer `n >= 1` using Fibonacci (Zeckendorf) coding.
/// Emits LSB-first Zeckendorf bits followed by a trailing `1` bit (forming a `11` terminator).
pub fn writeFibonacci(bw: *BitWriter, n: u64): bool {
    if (n == 0) {
        return false;
    }
    let fib: [92]u64 = [0; 92];
    fillFibTable(fib[0..]);

    // Find highest index hi where fib[hi] <= n
    let hi: usize = 0;
    while (hi + 1 < 92 && fib[hi + 1] <= n) {
        hi += 1;
    }

    let bits: [92]u1 = [0; 92];
    let rem = n;
    let idx: usize = hi + 1;
    while (idx > 0) {
        idx -= 1;
        if (fib[idx] <= rem) {
            bits[idx] = 1;
            rem -= fib[idx];
        }
    }

    let i: usize = 0;
    while (i <= hi) {
        if (!bw.writeBit(bits[i])) {
            return false;
        }
        i += 1;
    }
    // Self-synchronizing terminating `1`
    return bw.writeBit(1);
}

/// Decodes a positive integer (`n >= 1`) encoded with Fibonacci (Zeckendorf) coding.
pub fn readFibonacci(br: *BitReader): u64? {
    let fib: [92]u64 = [0; 92];
    fillFibTable(fib[0..]);

    let acc: u64 = 0;
    let prev_one: bool = false;
    let idx: usize = 0;

    while (true) {
        const b = br.readBit() ?? return null;
        if (b == 1) {
            if (prev_one) {
                return acc;
            }
            if (idx >= 92) {
                return null;
            }
            if (fib[idx] > 0xFFFFFFFFFFFFFFFF - acc) {
                return null;
            }
            acc += fib[idx];
            prev_one = true;
        } else {
            prev_one = false;
        }
        idx += 1;
        if (idx > 92) {
            return null;
        }
    }
}
