// Type definitions for CP

export fn toUpper(c: u8): u8;
export fn isAlpha(c: u8): bool;
export fn soundexDigit(u: u8): u8;
export fn soundex(word: string, out: []u8): usize;
export fn soundsLike(a: string, b: string): bool;
export fn soundexDifference(a: string, b: string): u8;
