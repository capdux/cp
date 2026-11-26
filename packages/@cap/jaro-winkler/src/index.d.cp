// Type definitions for CP

export fn jaro(a: string, b: string): f64;
export fn jaroWinklerWithScale(a: string, b: string, prefix_scale: f64): f64;
export fn jaroWinkler(a: string, b: string): f64;
export fn bestMatchIndex(query: string, candidates: &[]string, min_score: f64): i64;
