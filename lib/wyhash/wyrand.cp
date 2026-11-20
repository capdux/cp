// Source: cp/lib/wyhash/wyrand.cp (WyRand PRNG engine with Lemire unbiased range scaling)

import { wyhashMum } from "./core";

const WYRAND_INC: u64 = 0xa0761d6478bd642f;
const WYRAND_MIX: u64 = 0xe7037ed1a0b428db;

/// Fast 64-bit WyRand pseudo-random number generator.
pub struct WyRand {
    state: u64,

    pub static fn init(seed: u64): WyRand {
        return WyRand { state: seed };
    }

    pub fn nextU64(*_): u64 {
        let next_state = ((_.state as u128) + (WYRAND_INC as u128)) as u64;
        _.state = next_state;
        return wyhashMum(next_state, next_state ^ WYRAND_MIX);
    }

    pub fn nextU32(*_): u32 {
        return (_.nextU64() >> 32) as u32;
    }

    /// Lemire's nearly divisionless unbiased bounded random integer in [0, bound).
    pub fn nextRange(*_, bound: u64): u64 {
        if (bound <= 1) {
            return 0;
        }
        let x = _.nextU64();
        let m: u128 = (x as u128) * (bound as u128);
        let l: u64 = m as u64;
        if (l < bound) {
            let two_pow_64: u128 = (1 as u128) << 64;
            let t: u64 = ((two_pow_64 - (bound as u128)) % (bound as u128)) as u64;
            while (l < t) {
                x = _.nextU64();
                m = (x as u128) * (bound as u128);
                l = m as u64;
            }
        }
        return (m >> 64) as u64;
    }

    /// Uniform f64 in [0.0, 1.0).
    pub fn nextF64(*_): f64 {
        let mantissa = _.nextU64() >> 11;
        return (mantissa as f64) * (1.0 / 9007199254740992.0);
    }

    /// Fill buffer with pseudo-random bytes.
    pub fn fillBytes(*_, buf: []u8) {
        let off: usize = 0;
        while (off + 8 <= buf.len) {
            let v = _.nextU64();
            let i: usize = 0;
            while (i < 8) {
                buf[off + i] = (v & 0xff) as u8;
                v = v >> 8;
                i += 1;
            }
            off += 8;
        }
        if (off < buf.len) {
            let v = _.nextU64();
            while (off < buf.len) {
                buf[off] = (v & 0xff) as u8;
                v = v >> 8;
                off += 1;
            }
        }
    }
}
