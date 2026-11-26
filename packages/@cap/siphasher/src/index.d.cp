// Type definitions for CP

export fn sipU64(hi: u32, lo: u32): u64;
export fn sipAdd64(a: u64, b: u64): u64;
export fn sipRotl64(x: u64, r: u64): u64;
export fn sipLoadLe64(buf: []u8, off: usize): u64;
export class SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

export fn sipRound(s: &mut SipState);
export fn sipHashCore(k0: u64, k1: u64, data: []u8, c_rounds: usize, d_rounds: usize): u64;
export fn sipHash24(k0: u64, k1: u64, data: []u8): u64;
export fn sipHash13(k0: u64, k1: u64, data: []u8): u64;
export fn sipHash24Key16(key16: []u8, data: []u8): u64?;
export fn sipHash24Str(k0: u64, k1: u64, s: string): u64;
