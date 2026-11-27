// Type definitions for CP

export fn hexUpper(nibble: u8): u8;
export fn hexVal(b: u8): i32;
export fn isDirectPrintable(b: u8): bool;
export fn encode(data: &[]u8, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
