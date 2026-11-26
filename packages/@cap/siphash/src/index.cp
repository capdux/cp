// @cap/siphash — Zero-allocation SipHash-2-4 & SipHash-1-3 keyed streaming hashers for CP
// Clean-room MIT implementation inspired by Rust `siphasher` and Aumasson-Bernstein SipHash spec.

const U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

const SIP_C0: u64 = ((0x736f6d65 as u64) << 32) | (0x70736575 as u64);
const SIP_C1: u64 = ((0x646f7261 as u64) << 32) | (0x6e646f6d as u64);
const SIP_C2: u64 = ((0x6c796765 as u64) << 32) | (0x6e657261 as u64);
const SIP_C3: u64 = ((0x74656462 as u64) << 32) | (0x79746573 as u64);

// ---------------------------------------------------------------------------
// Overflow-Safe 64-bit Arithmetic & SipRound Primitives
// ---------------------------------------------------------------------------

fn addWrap64(a: u64, b: u64) -> u64 {
    const space = U64_MAX - b;
    if (a > space) {
        return (a - space) - 1;
    }
    return a + b;
}

fn rotl64(x: u64, r: u64) -> u64 {
    return (x << r) | (x >> (64 - r));
}

struct SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

fn sipRound(s: SipState) -> SipState {
    let v0 = addWrap64(s.v0, s.v1);
    let v1 = rotl64(s.v1, 13) ^ v0;
    v0 = rotl64(v0, 32);

    let v2 = addWrap64(s.v2, s.v3);
    let v3 = rotl64(s.v3, 16) ^ v2;

    v0 = addWrap64(v0, v3);
    v3 = rotl64(v3, 21) ^ v0;

    v2 = addWrap64(v2, v1);
    v1 = rotl64(v1, 17) ^ v2;
    v2 = rotl64(v2, 32);

    return SipState{ v0: v0, v1: v1, v2: v2, v3: v3 };
}

fn loadLe64(buf: []u8, offset: usize) -> u64 {
    return (buf[offset] as u64)
        | ((buf[offset + 1] as u64) << 8)
        | ((buf[offset + 2] as u64) << 16)
        | ((buf[offset + 3] as u64) << 24)
        | ((buf[offset + 4] as u64) << 32)
        | ((buf[offset + 5] as u64) << 40)
        | ((buf[offset + 6] as u64) << 48)
        | ((buf[offset + 7] as u64) << 56);
}

fn loadLe64Str(s: string, offset: usize) -> u64 {
    return (s[offset] as u64)
        | ((s[offset + 1] as u64) << 8)
        | ((s[offset + 2] as u64) << 16)
        | ((s[offset + 3] as u64) << 24)
        | ((s[offset + 4] as u64) << 32)
        | ((s[offset + 5] as u64) << 40)
        | ((s[offset + 6] as u64) << 48)
        | ((s[offset + 7] as u64) << 56);
}

// ---------------------------------------------------------------------------
// 1. SipHash-2-4 Streaming Hasher
// ---------------------------------------------------------------------------

pub struct SipHasher24 {
    pub v0: u64,
    pub v1: u64,
    pub v2: u64,
    pub v3: u64,
    pub buf: [8]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn init(k0: u64, k1: u64) -> SipHasher24 {
        return SipHasher24{
            v0: k0 ^ SIP_C0,
            v1: k1 ^ SIP_C1,
            v2: k0 ^ SIP_C2,
            v3: k1 ^ SIP_C3,
            buf: [0, 0, 0, 0, 0, 0, 0, 0],
            buf_len: 0,
            total_len: 0,
        };
    }

    pub fn compress(*_, m: u64) {
        let s = SipState{ v0: _.v0, v1: _.v1, v2: _.v2, v3: _.v3 ^ m };
        s = sipRound(s);
        s = sipRound(s);
        _.v0 = s.v0 ^ m;
        _.v1 = s.v1;
        _.v2 = s.v2;
        _.v3 = s.v3;
    }

    pub fn write(*_, data: string) {
        _.total_len += data.len as u64;
        let i: usize = 0;

        // Fill partial buffered block first
        if (_.buf_len > 0) {
            while (i < data.len && _.buf_len < 8) {
                _.buf[_.buf_len] = data[i];
                _.buf_len += 1;
                i += 1;
            }
            if (_.buf_len == 8) {
                const m = loadLe64(_.buf[0..8], 0 as usize);
                _.compress(m);
                _.buf_len = 0;
            }
        }

        // Process full 8-byte blocks directly from input
        while (i + 8 <= data.len) {
            const m = loadLe64Str(data, i);
            _.compress(m);
            i += 8;
        }

        // Buffer remaining tail bytes (< 8)
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            i += 1;
        }
    }

    pub fn finish() -> u64 {
        let last: u64 = (_.total_len & 0xFF) << 56;
        let k: usize = 0;
        while (k < _.buf_len) {
            last |= (_.buf[k] as u64) << ((k * 8) as u64);
            k += 1;
        }

        let s = SipState{ v0: _.v0, v1: _.v1, v2: _.v2, v3: _.v3 ^ last };
        s = sipRound(s);
        s = sipRound(s);
        s.v0 ^= last;
        s.v2 ^= 0xff;
        s = sipRound(s);
        s = sipRound(s);
        s = sipRound(s);
        s = sipRound(s);
        return s.v0 ^ s.v1 ^ s.v2 ^ s.v3;
    }
}

/// Computes SipHash-2-4 with 128-bit key `(k0, k1)` over `data`.
pub fn siphash24(k0: u64, k1: u64, data: string) -> u64 {
    let h = SipHasher24.init(k0, k1);
    h.write(data);
    return h.finish();
}

// ---------------------------------------------------------------------------
// 2. SipHash-1-3 Streaming Hasher (Rust `std::collections::HashMap` default)
// ---------------------------------------------------------------------------

pub struct SipHasher13 {
    pub v0: u64,
    pub v1: u64,
    pub v2: u64,
    pub v3: u64,
    pub buf: [8]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn init(k0: u64, k1: u64) -> SipHasher13 {
        return SipHasher13{
            v0: k0 ^ SIP_C0,
            v1: k1 ^ SIP_C1,
            v2: k0 ^ SIP_C2,
            v3: k1 ^ SIP_C3,
            buf: [0, 0, 0, 0, 0, 0, 0, 0],
            buf_len: 0,
            total_len: 0,
        };
    }

    pub fn compress(*_, m: u64) {
        let s = SipState{ v0: _.v0, v1: _.v1, v2: _.v2, v3: _.v3 ^ m };
        s = sipRound(s);
        _.v0 = s.v0 ^ m;
        _.v1 = s.v1;
        _.v2 = s.v2;
        _.v3 = s.v3;
    }

    pub fn write(*_, data: string) {
        _.total_len += data.len as u64;
        let i: usize = 0;

        if (_.buf_len > 0) {
            while (i < data.len && _.buf_len < 8) {
                _.buf[_.buf_len] = data[i];
                _.buf_len += 1;
                i += 1;
            }
            if (_.buf_len == 8) {
                const m = loadLe64(_.buf[0..8], 0 as usize);
                _.compress(m);
                _.buf_len = 0;
            }
        }

        while (i + 8 <= data.len) {
            const m = loadLe64Str(data, i);
            _.compress(m);
            i += 8;
        }

        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            i += 1;
        }
    }

    pub fn finish() -> u64 {
        let last: u64 = (_.total_len & 0xFF) << 56;
        let k: usize = 0;
        while (k < _.buf_len) {
            last |= (_.buf[k] as u64) << ((k * 8) as u64);
            k += 1;
        }

        let s = SipState{ v0: _.v0, v1: _.v1, v2: _.v2, v3: _.v3 ^ last };
        s = sipRound(s);
        s.v0 ^= last;
        s.v2 ^= 0xff;
        s = sipRound(s);
        s = sipRound(s);
        s = sipRound(s);
        return s.v0 ^ s.v1 ^ s.v2 ^ s.v3;
    }
}

/// Computes SipHash-1-3 with 128-bit key `(k0, k1)` over `data`.
pub fn siphash13(k0: u64, k1: u64, data: string) -> u64 {
    let h = SipHasher13.init(k0, k1);
    h.write(data);
    return h.finish();
}
