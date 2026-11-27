// Type definitions for CP

export fn parseNibble(c: u8): u8?;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn encodeHex(input: string, out: []u8): string;
export fn encodeHexUpper(input: string, out: []u8): string;
export fn isValidHex(s: string): bool;
export fn decodeHex(hex_str: string, out: []u8): string?;
export fn decodeHexPrefixed(hex_str: string, out: []u8): string?;
export fn encodeU32Hex(v: u32, out: []u8): string;
export fn encodeU64Hex(v: u64, out: []u8): string;
export fn hexDump(data: string, out: []u8): string;
