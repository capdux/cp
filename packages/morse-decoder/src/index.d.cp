// Type definitions for CP

export fn toUpperAscii(c: u8): u8;
export fn isWs(c: u8): bool;
export fn bytesEq(a: string, b: string): bool;
export fn encodeChar(c: u8): string;
export fn decodeSymbol(sym: string): u8;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn encodeText(input: string, out: []u8): usize;
export fn decodeText(morse: string, out: []u8): usize;
