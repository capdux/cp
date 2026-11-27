// adler2 — Zero-heap RFC 1950 Adler-32 Checksum Engine (Rust `adler2` crate parity)
// Clean-room MIT implementation in pure CP (streaming, SIMD NMAX=5552 deferred modulo, rolling window, and combine).

pub const ADLER32_MOD: u32 = 65521;
pub const ADLER32_NMAX: usize = 5552;
const NMAX: usize = 5552;
type AdlVec16U8 = @Vector(16, u8);
type AdlVec16U32 = @Vector(16, u32);

pub struct Adler32 {
    s1: u32,
    s2: u32,

    /// Creates a new Adler-32 checksum state initialized to `(s1 = 1, s2 = 0)`.
    pub static fn init(): Adler32 {
        return Adler32 { s1: 1, s2: 0 };
    }

    /// Rust `adler2::Adler32::new` constructor.
    pub static fn new(): Adler32 {
        return Adler32.init();
    }

    /// Resumes an Adler-32 state from an existing checksum value.
    pub static fn fromValue(value: u32): Adler32 {
        return Adler32 {
            s1: (value & 0xFFFF) % ADLER32_MOD,
            s2: ((value >> 16) & 0xFFFF) % ADLER32_MOD,
        };
    }

    /// Rust `adler2::Adler32::from_checksum` constructor.
    pub static fn fromChecksum(value: u32): Adler32 {
        return Adler32.fromValue(value);
    }

    /// Feeds a byte slice into the checksum using SIMD 16-byte chunks and `NMAX=5552` deferred modulo.
    pub fn update(*_, data: []const u8) {
        const weights: AdlVec16U32 = [16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1];
        let s1: u32 = _.s1;
        let s2: u32 = _.s2;
        let pos: usize = 0;
        const n: usize = data.len;
        while (pos < n) {
            let chunk: usize = n - pos;
            if (chunk > NMAX) {
                chunk = NMAX;
            }
            const end: usize = pos + chunk;
            while (pos + 16 <= end) {
                const v8: AdlVec16U8 = data[pos .. pos + 16][0..16].*;
                const v32: AdlVec16U32 = v8;
                s2 += s1 * 16 + @reduce(.Add, v32 * weights);
                s1 += @reduce(.Add, v32);
                pos += 16;
            }
            while (pos < end) {
                s1 = s1 + (data[pos] as u32);
                s2 = s2 + s1;
                pos += 1;
            }
            s1 = s1 % ADLER32_MOD;
            s2 = s2 % ADLER32_MOD;
        }
        _.s1 = s1;
        _.s2 = s2;
    }

    /// Rust `adler2::Adler32::write_slice` streaming update.
    pub fn writeSlice(*_, data: []const u8) {
        _.update(data);
    }

    /// Feeds a UTF-8 string into the checksum.
    pub fn writeStr(*_, s: string) {
        _.update(s);
    }

    /// Feeds a single byte into the checksum.
    pub fn updateByte(*_, b: u8) {
        let s1: u32 = _.s1 + (b as u32);
        if (s1 >= ADLER32_MOD) {
            s1 -= ADLER32_MOD;
        }
        let s2: u32 = _.s2 + s1;
        if (s2 >= ADLER32_MOD) {
            s2 -= ADLER32_MOD;
        }
        _.s1 = s1;
        _.s2 = s2;
    }

    /// Rust `adler2::Adler32::write_byte` single-byte streaming update.
    pub fn writeByte(*_, b: u8) {
        _.updateByte(b);
    }

    /// Rolling window update: remove oldest byte `out_byte` and append `in_byte` for a window of `window_len` bytes.
    pub fn roll(*_, window_len: usize, out_byte: u8, in_byte: u8) {
        const w: u32 = (window_len % (ADLER32_MOD as usize)) as u32;
        const ob: u32 = out_byte as u32;
        const ib: u32 = in_byte as u32;
        const s1: u32 = (_.s1 + ADLER32_MOD - ob + ib) % ADLER32_MOD;
        const remove_term: u32 = ((w * ob) % ADLER32_MOD);
        const s2: u32 = (_.s2 + ADLER32_MOD + ADLER32_MOD - remove_term + s1 - 1) % ADLER32_MOD;
        _.s1 = s1;
        _.s2 = s2;
    }

    /// Returns the current 32-bit Adler-32 checksum.
    pub fn finalize(): u32 {
        return (_.s2 << 16) | _.s1;
    }

    /// Rust `adler2::Adler32::checksum` getter.
    pub fn checksum(): u32 {
        return _.finalize();
    }

    /// Resets the checksum state back to initial `(s1 = 1, s2 = 0)`.
    pub fn reset(*_) {
        _.s1 = 1;
        _.s2 = 0;
    }
}

/// One-shot Adler-32 checksum of a byte slice.
pub fn adler32(data: []const u8): u32 {
    let h = Adler32.init();
    h.update(data);
    return h.finalize();
}

/// Rust `adler2::adler32_slice` one-shot helper.
pub fn adler32Slice(data: []const u8): u32 {
    return adler32(data);
}

/// One-shot Adler-32 checksum of a string.
pub fn adler32Str(s: string): u32 {
    return adler32(s);
}

/// Combine two Adler-32 checksums: `adler32(A || B)` given `adler32(A)`, `adler32(B)`, and `len(B)`.
pub fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32 {
    const rem: u32 = (len2 % (ADLER32_MOD as u64)) as u32;
    const sum1: u32 = adler1 & 0xFFFF;
    const sum2: u32 = ((rem as u64) * (sum1 as u64) % (ADLER32_MOD as u64)) as u32;
    const s1: u32 = (sum1 + (adler2 & 0xFFFF) + ADLER32_MOD - 1) % ADLER32_MOD;
    const s2: u32 = (sum2 + ((adler1 >> 16) & 0xFFFF) + ((adler2 >> 16) & 0xFFFF) + ADLER32_MOD - rem) % ADLER32_MOD;
    return (s2 << 16) | s1;
}

/// Alias matching zlib/Rust `adler32_combine` naming.
pub fn adler32Combine(adler1: u32, adler2: u32, len2: u64): u32 {
    return combineAdler32(adler1, adler2, len2);
}

export {
    ADLER32_MOD,
    ADLER32_NMAX,
    Adler32,
    adler32,
    adler32Slice,
    adler32Str,
    combineAdler32,
    adler32Combine,
};
