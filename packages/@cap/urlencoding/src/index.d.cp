// Type definitions for CP

export fn urlencHexUpper(nibble: u8): u8;
export fn urlencHexVal(ch: u8): i16;
export fn isUnreserved(b: u8): bool;
export fn encodedLen(input: string): usize;
export fn encodeBinary(data: &[]u8, out: []u8): usize;
export fn encode(input: string, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
export fn decodeLossy(input: string, out: []u8): usize;
