// Type definitions for CP

export class SubstringMatch {
    a_start: usize,
    b_start: usize,
    len: usize,
}

export class DiffStats {
    common: usize,
    insertions: usize,
    deletions: usize,
}

export fn lcsLength(a: string, b: string): usize;
export fn diffStats(a: string, b: string): DiffStats;
export fn lcsSimilarity(a: string, b: string): f64;
export fn longestCommonSubstring(a: string, b: string): SubstringMatch;
