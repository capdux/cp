// Type definitions for CP

export fn toUpperAlpha(b: u8): u8;
export fn isVowel(b: u8): bool;
export fn metaphone(word: string, max_len: usize, out: []u8): usize;
export fn soundsLike(a: string, b: string): bool;
