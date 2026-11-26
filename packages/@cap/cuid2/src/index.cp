// @cap/cuid2 — Zero-allocation deterministic Cuid2 generator & validator with built-in SHA-256 for CP
// Clean-room MIT implementation inspired by npm `@paralleldrive/cuid2` and Rust `cuid2`.

const BASE36_CHARS: string = "0123456789abcdefghijklmnopqrstuvwxyz";
const MIN_ID_LEN: usize = 8;
const MAX_ID_LEN: usize = 32;

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) * (0x100000000 as u64)) | (lo as u64);
}

fn addWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const lo = (a & mask32) + (b & mask32);
    const carry = lo / shift32;
    const hi = ((a / shift32) + (b / shift32) + carry) & mask32;
    return (hi * shift32) | (lo & mask32);
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const a_lo = a & mask32;
    const a_hi = a / shift32;
    const b_lo = b & mask32;
    const b_hi = b / shift32;
    const p_lo = a_lo * b_lo;
    const mid1 = (a_lo * b_hi) & mask32;
    const mid2 = (a_hi * b_lo) & mask32;
    const hi_sum = ((p_lo / shift32) + mid1 + mid2) & mask32;
    return (hi_sum * shift32) | (p_lo & mask32);
}

fn nextEntropy(state: *u64) -> u64 {
    const c1 = makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32);
    const m1 = makeU64(0xBF58476D as u32, 0x1CE4E5B9 as u32);
    const m2 = makeU64(0x94D049BB as u32, 0x133111EB as u32);
    state.* = addWrap64(state.*, c1);
    let z = state.*;
    z = mulWrap64(z ^ (z / (0x40000000 as u64)), m1);
    z = mulWrap64(z ^ (z / (0x8000000 as u64)), m2);
    return z ^ (z / (0x80000000 as u64));
}

// --- Built-in FIPS 180-4 SHA-256 (32-byte single-block fast path) ---

fn add32(a: u32, b: u32) -> u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn pow2_64(n: u32) -> u64 {
    let v: u64 = 1;
    let i: u32 = 0;
    while (i < n) {
        v = v * (2 as u64);
        i += 1 as u32;
    }
    return v;
}

fn rotr32(x: u32, n: u32) -> u32 {
    const d = pow2_64(n);
    const m = pow2_64((32 as u32) - n);
    const xu = x as u64;
    return (((xu / d) | ((xu % d) * m)) & (0xFFFFFFFF as u64)) as u32;
}

fn shr32(x: u32, n: u32) -> u32 {
    return ((x as u64) / pow2_64(n)) as u32;
}

fn bitNot32(x: u32) -> u32 {
    return x ^ (0xFFFFFFFF as u32);
}

fn sha256K(i: usize) -> u32 {
    const table: [64]u32 = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    ];
    return table[i];
}

fn writeU64Be(buf: *[64]u8, off: usize, v: u64) {
    buf[off] = ((v / (0x100000000000000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 1] = ((v / (0x1000000000000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 2] = ((v / (0x10000000000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 3] = ((v / (0x100000000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 4] = ((v / (0x1000000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 5] = ((v / (0x10000 as u64)) & (0xFF as u64)) as u8;
    buf[off + 6] = ((v / (0x100 as u64)) & (0xFF as u64)) as u8;
    buf[off + 7] = (v & (0xFF as u64)) as u8;
}

fn sha256FourU64(v0: u64, v1: u64, v2: u64, v3: u64) -> [32]u8 {
    // 32-byte message fits in a single 64-byte SHA-256 block with padding
    let block: [64]u8 = undefined;
    let z: usize = 0;
    while (z < 64) {
        block[z] = 0 as u8;
        z += 1;
    }
    writeU64Be(&block, 0 as usize, v0);
    writeU64Be(&block, 8 as usize, v1);
    writeU64Be(&block, 16 as usize, v2);
    writeU64Be(&block, 24 as usize, v3);
    block[32] = 0x80 as u8;
    // Bit length = 256 = 0x0100 at block[62..64]
    block[62] = 0x01 as u8;
    block[63] = 0x00 as u8;

    let w: [64]u32 = undefined;
    let i: usize = 0;
    while (i < 16) {
        const b0 = block[i * 4] as u32;
        const b1 = block[(i * 4) + 1] as u32;
        const b2 = block[(i * 4) + 2] as u32;
        const b3 = block[(i * 4) + 3] as u32;
        w[i] = (b0 * (16777216 as u32)) | (b1 * (65536 as u32)) | (b2 * (256 as u32)) | b3;
        i += 1;
    }
    while (i < 64) {
        const s0 = rotr32(w[i - 15], 7 as u32) ^ rotr32(w[i - 15], 18 as u32) ^ shr32(w[i - 15], 3 as u32);
        const s1 = rotr32(w[i - 2], 17 as u32) ^ rotr32(w[i - 2], 19 as u32) ^ shr32(w[i - 2], 10 as u32);
        w[i] = add32(add32(w[i - 16], s0), add32(w[i - 7], s1));
        i += 1;
    }

    let h: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    let a = h[0];
    let b = h[1];
    let c = h[2];
    let d = h[3];
    let e = h[4];
    let f = h[5];
    let g = h[6];
    let hh = h[7];

    let r: usize = 0;
    while (r < 64) {
        const S1 = rotr32(e, 6 as u32) ^ rotr32(e, 11 as u32) ^ rotr32(e, 25 as u32);
        const ch = (e & f) ^ (bitNot32(e) & g);
        const temp1 = add32(add32(add32(hh, S1), ch), add32(sha256K(r), w[r]));
        const S0 = rotr32(a, 2 as u32) ^ rotr32(a, 13 as u32) ^ rotr32(a, 22 as u32);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = add32(S0, maj);

        hh = g;
        g = f;
        f = e;
        e = add32(d, temp1);
        d = c;
        c = b;
        b = a;
        a = add32(temp1, temp2);
        r += 1;
    }

    h[0] = add32(h[0], a);
    h[1] = add32(h[1], b);
    h[2] = add32(h[2], c);
    h[3] = add32(h[3], d);
    h[4] = add32(h[4], e);
    h[5] = add32(h[5], f);
    h[6] = add32(h[6], g);
    h[7] = add32(h[7], hh);

    let out: [32]u8 = undefined;
    let k: usize = 0;
    while (k < 8) {
        const v = h[k];
        out[k * 4] = ((v / (16777216 as u32)) & (0xFF as u32)) as u8;
        out[(k * 4) + 1] = ((v / (65536 as u32)) & (0xFF as u32)) as u8;
        out[(k * 4) + 2] = ((v / (256 as u32)) & (0xFF as u32)) as u8;
        out[(k * 4) + 3] = (v & (0xFF as u32)) as u8;
        k += 1;
    }
    return out;
}

// Divide big-endian 256-bit integer in-place by 36, returning remainder 0..35
fn divMod36(bytes: *[32]u8) -> u8 {
    let rem: u32 = 0;
    let i: usize = 0;
    while (i < 32) {
        const cur = (rem * (256 as u32)) + (bytes[i] as u32);
        bytes[i] = (cur / (36 as u32)) as u8;
        rem = cur % (36 as u32);
        i += 1;
    }
    return rem as u8;
}

pub struct Cuid2Generator {
    pub counter: u64,
    pub fingerprint: u64,
    pub rng_state: u64,
    pub id_len: usize,

    pub static fn init(fingerprint: u64, seed: u64, id_len: usize) -> Cuid2Generator {
        let clamped = id_len;
        if (clamped < MIN_ID_LEN) {
            clamped = MIN_ID_LEN;
        }
        if (clamped > MAX_ID_LEN) {
            clamped = MAX_ID_LEN;
        }
        return Cuid2Generator{
            counter: 0 as u64,
            fingerprint: fingerprint,
            rng_state: seed,
            id_len: clamped,
        };
    }

    pub fn next(*_, timestamp_ms: u64, dst: []u8) -> usize {
        if (dst.len < _.id_len) {
            return 0 as usize;
        }
        const ent = nextEntropy(&_.rng_state);
        const cnt = _.counter;
        _.counter += 1 as u64;

        let digest = sha256FourU64(timestamp_ms, cnt, _.fingerprint, ent);
        // Leading character must be a lowercase ASCII letter 'a'..'z'
        const firstLetterIdx = (digest[0] % (26 as u8));
        dst[0] = ('a' as u8) + firstLetterIdx;

        // Remaining id_len - 1 characters in lowercase Base36 via 256-bit divMod36
        let pos: usize = 1;
        while (pos < _.id_len) {
            const rem = divMod36(&digest);
            dst[pos] = BASE36_CHARS[rem as usize];
            pos += 1;
        }
        return _.id_len;
    }

    pub fn counterValue() -> u64 {
        return _.counter;
    }
}

pub fn isCuid2(s: []const u8) -> bool {
    if (s.len < (2 as usize) || s.len > MAX_ID_LEN) {
        return false;
    }
    if (s[0] < 'a' || s[0] > 'z') {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        const c = s[i];
        const ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z');
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isCuid2WithLen(s: []const u8, expected_len: usize) -> bool {
    if (s.len != expected_len) {
        return false;
    }
    return isCuid2(s);
}

export {
    Cuid2Generator,
    isCuid2,
    isCuid2WithLen,
};
