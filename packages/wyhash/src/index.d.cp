// Type definitions for CP

export class Mul128 {
    lo: u64,
    hi: u64,
}

export fn makeU64(hi: u32, lo: u32): u64;
export fn mul128(a: u64, b: u64): Mul128;
export fn wymum(a: u64, b: u64): u64;
export fn read4(data: &[]u8, off: usize): u64;
export fn read8(data: &[]u8, off: usize): u64;
export fn wyhashBytes(data: &[]u8, seed: u64): u64;
export fn wyhash(data: string, seed: u64): u64;
export class WyRng {
    state: u64,
    fn new(seed: u64): WyRng;
    fn nextU64(_: auto): u64;
    fn nextU32(_: auto): u32;
    fn nextF64(_: auto): f64;
}

