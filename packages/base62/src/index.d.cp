// Type definitions for CP

export class DecodeU64Result {
    value: u64,
    ok: bool,
}

export fn charToVal(c: u8): i32;
export fn isValidBase62(src: &[]u8): bool;
export fn encodeU64(value: u64, dst: []u8): usize;
export fn encodeU64Padded(value: u64, width: usize, dst: []u8): usize;
export fn decodeU64(src: &[]u8): DecodeU64Result;
export fn encodeBytes(src: &[]u8, dst: []u8): i64;
export fn decodeBytes(src: &[]u8, dst: []u8): i64;
