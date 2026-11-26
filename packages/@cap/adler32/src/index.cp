// @cap/adler32 — Zero-heap RFC 1950 Adler-32 Checksum Engine
// Clean-room MIT implementation in pure CP (streaming, rolling window, and combine).

pub const ADLER32_MOD: u32 = 65521;
const NMAX: usize = 5552;

pub struct Adler32 {
    s1: u32,
    s2: u32,

    pub static fn init(): Adler32 {
        return Adler32 { s1: 1, s2: 0 };
    }

    pub static fn fromValue(checksum: u32): Adler32 {
        return Adler32 {
            s1: checksum & 0xFFFF,
            s2: (checksum >> 16) & 0xFFFF,
        };
    }

    pub fn update(*_, data: []const u8) {
        let s1: u32 = _.s1;
        let s2: u32 = _.s2;
        let pos: usize = 0;
        let n: usize = data.len;
        while (pos < n) {
            let chunk: usize = n - pos;
            if (chunk > NMAX) {
                chunk = NMAX;
            }
            let end: usize = pos + chunk;
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

    pub fn updateByte(*_, b: u8) {
        let s1: u32 = (_.s1 + (b as u32)) % ADLER32_MOD;
        let s2: u32 = (_.s2 + s1) % ADLER32_MOD;
        _.s1 = s1;
        _.s2 = s2;
    }

    // Rolling window update: remove oldest byte `out_byte` and append `in_byte` for a window of `window_len` bytes
    pub fn roll(*_, window_len: usize, out_byte: u8, in_byte: u8) {
        let w: u32 = (window_len % (ADLER32_MOD as usize)) as u32;
        let ob: u32 = out_byte as u32;
        let ib: u32 = in_byte as u32;
        let s1: u32 = (_.s1 + ADLER32_MOD - ob + ib) % ADLER32_MOD;
        let remove_term: u32 = ((w * ob) % ADLER32_MOD);
        let s2: u32 = (_.s2 + ADLER32_MOD + ADLER32_MOD - remove_term + s1 - 1) % ADLER32_MOD;
        _.s1 = s1;
        _.s2 = s2;
    }

    pub fn finalize(): u32 {
        return (_.s2 << 16) | _.s1;
    }

    pub fn reset(*_) {
        _.s1 = 1;
        _.s2 = 0;
    }
}

pub fn adler32(data: []const u8): u32 {
    let h = Adler32.init();
    h.update(data);
    return h.finalize();
}

// Combine two Adler-32 checksums: adler32(A || B) given adler32(A), adler32(B), and len(B)
pub fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32 {
    let rem: u32 = (len2 % (ADLER32_MOD as u64)) as u32;
    let sum1: u32 = adler1 & 0xFFFF;
    let sum2: u32 = ((rem as u64) * (sum1 as u64) % (ADLER32_MOD as u64)) as u32;
    let s1: u32 = (sum1 + (adler2 & 0xFFFF) + ADLER32_MOD - 1) % ADLER32_MOD;
    let s2: u32 = (sum2 + ((adler1 >> 16) & 0xFFFF) + ((adler2 >> 16) & 0xFFFF) + ADLER32_MOD - rem) % ADLER32_MOD;
    return (s2 << 16) | s1;
}
