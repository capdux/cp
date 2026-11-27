// Type definitions for CP

export class CoverageMetric {
    total: u32,
    covered: u32,
    skipped: u32,
    fn pct(): f64;
    fn merge(other: CoverageMetric): CoverageMetric;
}

export class CoverageSummary {
    lines: CoverageMetric,
    statements: CoverageMetric,
    functions: CoverageMetric,
    branches: CoverageMetric,
    fn merge(other: CoverageSummary): CoverageSummary;
    fn isAboveThreshold(min_pct: f64): bool;
}

export class LineHit {
    line: u32,
    hits: u32,
}

export fn icovWriteStr(out: []u8, pos: usize, s: string): usize;
export fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn icovWriteU32(out: []u8, pos: usize, val: u32): usize;
export fn icovTrim(s: string): string;
export fn icovHasPrefix(s: string, prefix: string): bool;
export fn icovStrEq(a: string, b: string): bool;
export fn icovParseU32(s: string): u32;
export fn icovIndexOfByte(s: string, target: u8): usize;
export fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;
export fn parseLcovSummary(lcov_text: string): CoverageMetric;
