// Type definitions for CP

export class LrsResult {
    start: usize,
    len: usize,
    found: bool,
}

export fn compareSuffixes(buf: &[]u8, n: usize, a: usize, b: usize): i32;
export class SuffixArray {
    text: [256]u8,
    sa: [256]u16,
    lcp: [256]u16,
    text_len: usize,
    fn build(src: &[]u8): SuffixArray;
    fn len(): usize;
    fn suffixAt(rank: usize): u16;
    fn lcpAt(rank: usize): u16;
    fn comparePrefix(suffix_pos: usize, pat: &[]u8): i32;
    fn lowerBound(pat: &[]u8): usize;
    fn upperBound(pat: &[]u8): usize;
    fn contains(pat: &[]u8): bool;
    fn count(pat: &[]u8): usize;
    fn longestRepeatedSubstring(): LrsResult;
}

