// Type definitions for CP

export fn tidIsAlpha(c: u8): bool;
export fn tidIsDigit(c: u8): bool;
export fn tidIsIdentChar(c: u8): bool;
export fn tidIsSpace(c: u8): bool;
export fn tidToUpper(c: u8): u8;
export fn tidToLower(c: u8): u8;
export fn toIdentifier(input: string, out: []u8): usize;
export fn toCamelIdentifier(input: string, out: []u8): usize;
export fn toConstIdentifier(input: string, out: []u8): usize;
export fn isValidIdentifier(s: string): bool;
