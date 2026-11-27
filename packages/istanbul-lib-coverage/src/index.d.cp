// Type definitions for CP

import { isDigit, trim, startsWith, strEq, findByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
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

export fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn icovParseU32(s: string): u32;
export fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;
export fn parseLcovSummary(lcov_text: string): CoverageMetric;
