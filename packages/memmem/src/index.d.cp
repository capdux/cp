// Type definitions for CP

export class Finder {
    needle_buf: [MAX_NEEDLE]u8,
    needle_len: usize,
    bad_char: [256]usize,
    lps: [MAX_NEEDLE]u16,
    is_periodic: bool,
    fn init(needle: &[]u8): Finder;
    fn needleLen(): usize;
    fn findKmpFrom(haystack: &[]u8, start: usize): i64;
    fn findFrom(haystack: &[]u8, start: usize): i64;
    fn findIn(haystack: &[]u8): i64;
    fn countIn(haystack: &[]u8): usize;
}

export fn findLongNeedle(haystack: &[]u8, needle: &[]u8): i64;
export fn find(haystack: &[]u8, needle: &[]u8): i64;
export fn rfind(haystack: &[]u8, needle: &[]u8): i64;
export fn contains(haystack: &[]u8, needle: &[]u8): bool;
export fn count(haystack: &[]u8, needle: &[]u8): usize;
