// Type definitions for CP

export fn toLower(c: u8): u8;
export fn min3(a: usize, b: usize, c: usize): usize;
export fn levenshtein(a: string, b: string): usize;
export fn damerauLevenshtein(a: string, b: string, ignore_case: bool): usize;
export fn similarity(a: string, b: string): f64;
export class Suggestion {
    found: bool,
    match_str: string,
    distance: usize,
    index: usize,
}

export fn didYouMeanWith(input: string, candidates: []string, max_dist: usize): Suggestion;
export fn didYouMean(input: string, candidates: []string): string?;
