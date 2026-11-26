// Type definitions for CP

export class DecodeResult {
    value: i64,
    consumed: usize,
    valid: bool,
}

export fn b64Char(v: u8): u8;
export fn b64Val(c: u8): i32;
export fn encodeOne(val: i64, out: []u8): usize;
export fn decodeOne(input: string, start: usize): DecodeResult;
export fn encodeSlice(values: &[]i64, out: []u8): usize;
export fn decodeSlice(input: string, out: []i64): i64;
