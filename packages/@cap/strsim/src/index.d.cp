// Type definitions for CP

export fn clampLen(n: usize): usize;
export fn strEq(a: string, b: string): bool;
export fn hamming(a: string, b: string): usize?;
export fn levenshtein(a: string, b: string): usize;
export fn normalizedLevenshtein(a: string, b: string): f64;
export fn osaDistance(a: string, b: string): usize;
export fn jaro(a: string, b: string): f64;
export fn jaroWinkler(a: string, b: string): f64;
export fn sorensenDice(a: string, b: string): f64;
