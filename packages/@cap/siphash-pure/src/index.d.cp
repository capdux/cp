// Type definitions for CP

export fn shU64(hi: u32, lo: u32): u64;
export fn shAdd64(a: u64, b: u64): u64;
export fn shRotl64(x: u64, bits: u32): u64;
export fn shReadLe64(data: &[]u8, offset: usize): u64;
export class SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

export fn shRound(st: &mut SipState);
export fn shRun(k0: u64, k1: u64, data: &[]u8, c_rounds: usize, d_rounds: usize): u64;
export fn siphash24(k0: u64, k1: u64, data: &[]u8): u64;
export fn siphash13(k0: u64, k1: u64, data: &[]u8): u64;
export fn siphash24Key16(key: [16]u8, data: &[]u8): u64;
export fn siphash13Key16(key: [16]u8, data: &[]u8): u64;
