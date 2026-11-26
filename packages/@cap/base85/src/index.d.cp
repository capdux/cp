// Type definitions for CP

export fn z85Index(c: u8): i8;
export fn isSpace(c: u8): bool;
export fn encodeZ85(input: string, out: []u8): string?;
export fn decodeZ85(encoded: string, out: []u8): string?;
export fn encodeAscii85(input: string, out: []u8): string?;
export fn decodeAscii85(encoded: string, out: []u8): string?;
