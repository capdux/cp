// Type definitions for CP

export fn slToLower(c: u8): u8;
export fn slIsAsciiAlnum(c: u8): bool;
export fn slFoldLatin(cp: u32, lowercase: bool, buf: []u8): usize;
export fn slugifyWithSep(input: string, sep: u8, lowercase: bool, out: []u8): usize;
export fn slugify(input: string, out: []u8): usize;
export fn slugifyTruncate(input: string, max_len: usize, out: []u8): usize;
export fn isValidSlug(s: string): bool;
