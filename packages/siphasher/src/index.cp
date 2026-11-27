// @cap/siphash — Zero-allocation SipHash-2-4 & SipHash-1-3 keyed streaming hashers for CP
// Clean-room MIT implementation inspired by Rust `siphasher` and Aumasson-Bernstein SipHash spec.
// Accelerated with 2-lane `@Vector(2, u64)` ARX SIMD rounds, `@memcpy` tail buffering,
// and unrolled 32-byte block ingestion.

type SipVec2u64 = @Vector(2, u64);
type SipVec2u6 = @Vector(2, u6);

const SIP_C0: u64 = ((0x736f6d65 as u64) << 32) | (0x70736575 as u64);
const SIP_C1: u64 = ((0x646f7261 as u64) << 32) | (0x6e646f6d as u64);
const SIP_C2: u64 = ((0x6c796765 as u64) << 32) | (0x6e657261 as u64);
const SIP_C3: u64 = ((0x74656462 as u64) << 32) | (0x79746573 as u64);

// ---------------------------------------------------------------------------
// 2-Lane SIMD ARX SipRound Primitives
// ---------------------------------------------------------------------------

struct SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

fn sipRound(s: SipState) -> SipState {
    // Lane 0: (v0, v1), Lane 1: (v2, v3)
    const v_even_0: SipVec2u64 = [s.v0, s.v2];
    const v_odd_0: SipVec2u64 = [s.v1, s.v3];
    const sum_a: SipVec2u64 = v_even_0 +% v_odd_0;
    const shl_a: SipVec2u6 = [13 as u6, 16 as u6];
    const shr_a: SipVec2u6 = [51 as u6, 48 as u6];
    const rot_a: SipVec2u64 = (v_odd_0 << shl_a) | (v_odd_0 >> shr_a);
    const xor_a: SipVec2u64 = rot_a ^ sum_a;
    const even_a: [2]u64 = sum_a;
    const odd_a: [2]u64 = xor_a;
    const v0_mid: u64 = (even_a[0] << 32) | (even_a[0] >> 32);
    const v2_mid: u64 = even_a[1];

    // Cross-lane half-round: (v0_mid + v3, v2_mid + v1)
    const v_even_1: SipVec2u64 = [v0_mid, v2_mid];
    const v_odd_1: SipVec2u64 = [odd_a[1], odd_a[0]];
    const sum_b: SipVec2u64 = v_even_1 +% v_odd_1;
    const shl_b: SipVec2u6 = [21 as u6, 17 as u6];
    const shr_b: SipVec2u6 = [43 as u6, 47 as u6];
    const rot_b: SipVec2u64 = (v_odd_1 << shl_b) | (v_odd_1 >> shr_b);
    const xor_b: SipVec2u64 = rot_b ^ sum_b;
    const even_b: [2]u64 = sum_b;
    const odd_b: [2]u64 = xor_b;
    const v2_out: u64 = (even_b[1] << 32) | (even_b[1] >> 32);

    return SipState{
        v0: even_b[0],
        v1: odd_b[1],
        v2: v2_out,
        v3: odd_b[0],
    };
}

fn loadLe64(buf: []const u8, offset: usize) -> u64 {
    return (buf[offset] as u64)
        | ((buf[offset + 1] as u64) << 8)
        | ((buf[offset + 2] as u64) << 16)
        | ((buf[offset + 3] as u64) << 24)
        | ((buf[offset + 4] as u64) << 32)
        | ((buf[offset + 5] as u64) << 40)
        | ((buf[offset + 6] as u64) << 48)
        | ((buf[offset + 7] as u64) << 56);
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
            buf: undefined,
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
            const need: usize = (8 as usize) - _.buf_len;
            const take: usize = if (data.len < need) { data.len } else { need };
            if (take > 0) {
                @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
                _.buf_len += take;
                i += take;
            }
            if (_.buf_len == 8) {
                const m = loadLe64(_.buf[0..8], 0 as usize);
                _.compress(m);
                _.buf_len = 0;
            }
        }

        // Unrolled 32-byte (4x u64) fast path
        while (i + 32 <= data.len) {
            _.compress(loadLe64(data, i));
            _.compress(loadLe64(data, i + 8));
            _.compress(loadLe64(data, i + 16));
            _.compress(loadLe64(data, i + 24));
            i += 32;
        }

        // Process remaining full 8-byte blocks directly from input
        while (i + 8 <= data.len) {
            const m = loadLe64(data, i);
            _.compress(m);
            i += 8;
        }

        // Buffer remaining tail bytes (< 8)
        const rem: usize = data.len - i;
        if (rem > 0) {
            @memcpy(_.buf[_.buf_len .. _.buf_len + rem], data[i..data.len]);
            _.buf_len += rem;
        }
    }

    pub fn finish() -> u64 {
        let last: u64 = (_.total_len & 0xFF) << 56;
        let k: usize = 0;
        while (k < _.buf_len) {
            last |= (_.buf[k] as u64) << ((k * 8) as u6);
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
            buf: undefined,
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
            const need: usize = (8 as usize) - _.buf_len;
            const take: usize = if (data.len < need) { data.len } else { need };
            if (take > 0) {
                @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
                _.buf_len += take;
                i += take;
            }
            if (_.buf_len == 8) {
                const m = loadLe64(_.buf[0..8], 0 as usize);
                _.compress(m);
                _.buf_len = 0;
            }
        }

        while (i + 32 <= data.len) {
            _.compress(loadLe64(data, i));
            _.compress(loadLe64(data, i + 8));
            _.compress(loadLe64(data, i + 16));
            _.compress(loadLe64(data, i + 24));
            i += 32;
        }

        while (i + 8 <= data.len) {
            const m = loadLe64(data, i);
            _.compress(m);
            i += 8;
        }

        const rem: usize = data.len - i;
        if (rem > 0) {
            @memcpy(_.buf[_.buf_len .. _.buf_len + rem], data[i..data.len]);
            _.buf_len += rem;
        }
    }

    pub fn finish() -> u64 {
        let last: u64 = (_.total_len & 0xFF) << 56;
        let k: usize = 0;
        while (k < _.buf_len) {
            last |= (_.buf[k] as u64) << ((k * 8) as u6);
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
