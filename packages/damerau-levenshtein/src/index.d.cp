// Type definitions for CP

export class DamLevResult {
    steps: usize,
    relative: f64,
    similarity: f64,
}

export fn dlvMin3(a: usize, b: usize, c: usize): usize;
export fn damerauLevenshteinSteps(a: string, b: string): usize;
export fn damerauLevenshtein(a: string, b: string): DamLevResult;
