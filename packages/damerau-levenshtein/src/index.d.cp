// Type definitions for CP

export const DL_MAX_OSA_LEN: usize;
export const DL_MAX_TRUE_LEN: usize;

export class DamLevResult {
    steps: usize,
    relative: f64,
    similarity: f64,
}

export fn damerauLevenshteinStepsBytes(a: []const u8, b: []const u8): usize;
export fn damerauLevenshteinSteps(a: string, b: string): usize;
export fn osaDistanceBytes(a: []const u8, b: []const u8): usize;
export fn osaDistance(a: string, b: string): usize;
export fn trueDamerauLevenshteinBytes(a: []const u8, b: []const u8): usize;
export fn trueDamerauLevenshteinSteps(a: string, b: string): usize;
export fn damerauLevenshteinBoundedBytes(a: []const u8, b: []const u8, maxDistance: usize): usize?;
export fn damerauLevenshteinBounded(a: string, b: string, maxDistance: usize): usize?;
export fn damerauLevenshtein(a: string, b: string): DamLevResult;
export fn trueDamerauLevenshtein(a: string, b: string): DamLevResult;
export fn similarity(a: string, b: string): f64;
export fn trueSimilarity(a: string, b: string): f64;
