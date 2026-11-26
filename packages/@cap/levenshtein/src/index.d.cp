// Type definitions for CP

export fn absDiff(a: usize, b: usize): usize;
export fn min3(a: usize, b: usize, c: usize): usize;
export fn distance(a: string, b: string): usize;
export fn damerauDistance(a: string, b: string): usize;
export fn similarity(a: string, b: string): f64;
export fn withinDistance(a: string, b: string, max_dist: usize): bool;
export fn closestIndex(query: string, candidates: &[]string, max_dist: usize): i64;
