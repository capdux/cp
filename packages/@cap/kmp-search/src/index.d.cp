// Type definitions for CP

export fn computeLps(pattern: &[]u8, out_lps: []usize): usize;
export fn findFirst(haystack: &[]u8, needle: &[]u8): i64;
export fn findAll(haystack: &[]u8, needle: &[]u8, overlapping: bool, out_offsets: []usize): usize;
export fn countMatches(haystack: &[]u8, needle: &[]u8, overlapping: bool): usize;
export fn contains(haystack: &[]u8, needle: &[]u8): bool;
