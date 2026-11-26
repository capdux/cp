// Type definitions for CP

export fn addWrap64(a: u64, b: u64): u64;
export fn rotl64(x: u64, r: u64): u64;
export class SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

export fn sipRound(s: SipState): SipState;
export fn loadLe64(buf: []u8, offset: usize): u64;
export fn loadLe64Str(s: string, offset: usize): u64;
export class SipHasher24 {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
    buf: [8]u8,
    buf_len: usize,
    total_len: u64,
    fn init(k0: u64, k1: u64): SipHasher24;
    fn compress(_: auto, m: u64);
    fn write(_: auto, data: string);
    fn finish(): u64;
}

export fn siphash24(k0: u64, k1: u64, data: string): u64;
export class SipHasher13 {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
    buf: [8]u8,
    buf_len: usize,
    total_len: u64,
    fn init(k0: u64, k1: u64): SipHasher13;
    fn compress(_: auto, m: u64);
    fn write(_: auto, data: string);
    fn finish(): u64;
}

export fn siphash13(k0: u64, k1: u64, data: string): u64;
