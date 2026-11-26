// Type definitions for CP

export fn dcToLower(c: u8): u8;
export fn dcIsSpace(c: u8): bool;
export fn dcNormalize(src: &[]u8, strip_ws_lower: bool, dst: []u8): usize;
export fn dcEval(a: string, b: string, strip_ws_lower: bool): f64;
export fn diceCoefficient(a: string, b: string): f64;
export fn diceCaseSensitive(a: string, b: string): f64;
export fn bestMatchIndex(target: string, candidates: &[]string, min_rating: f64): i64;
