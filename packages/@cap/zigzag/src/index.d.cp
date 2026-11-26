// Type definitions for CP

export class VarintResult {
    value: i64,
    consumed: usize,
    valid: bool,
}

export fn encodeI32(n: i32): u32;
export fn decodeI32(u: u32): i32;
export fn encodeI64(n: i64): u64;
export fn decodeI64(u: u64): i64;
export fn encodeZigzagVarint(n: i64, out: []u8): usize;
export fn decodeZigzagVarint(data: &[]u8, start: usize): VarintResult;
