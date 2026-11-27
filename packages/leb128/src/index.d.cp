// Type definitions for CP

export class UlebResult {
    value: u64,
    consumed: usize,
    valid: bool,
}

export class SlebResult {
    value: i64,
    consumed: usize,
    valid: bool,
}

export fn encodeUnsigned(val: u64, out: []u8): usize;
export fn decodeUnsigned(data: &[]u8, start: usize): UlebResult;
export fn encodeSigned(val: i64, out: []u8): usize;
export fn decodeSigned(data: &[]u8, start: usize): SlebResult;
export fn unsignedEncodedLen(val: u64): usize;
