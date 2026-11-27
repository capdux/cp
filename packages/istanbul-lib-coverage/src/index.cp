// @cap/istanbul-lib-coverage — Zero-heap code coverage metrics, summary merging, and LCOV codec.

import { isDigit, trim, startsWith, strEq, findByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub struct CoverageMetric {
    total: u32,
    covered: u32,
    skipped: u32,

    fn pct(): f64 {
        if (_.total == 0) {
            return 100.0;
        }
        const cov_f: f64 = f64(_.covered);
        const tot_f: f64 = f64(_.total);
        return (cov_f * 100.0) / tot_f;
    }

    fn merge(other: CoverageMetric): CoverageMetric {
        return CoverageMetric{
            total: _.total + other.total,
            covered: _.covered + other.covered,
            skipped: _.skipped + other.skipped,
        };
    }
}

pub struct CoverageSummary {
    lines: CoverageMetric,
    statements: CoverageMetric,
    functions: CoverageMetric,
    branches: CoverageMetric,

    fn merge(other: CoverageSummary): CoverageSummary {
        return CoverageSummary{
            lines: _.lines.merge(other.lines),
            statements: _.statements.merge(other.statements),
            functions: _.functions.merge(other.functions),
            branches: _.branches.merge(other.branches),
        };
    }

    fn isAboveThreshold(min_pct: f64): bool {
        if (_.lines.pct() < min_pct) return false;
        if (_.statements.pct() < min_pct) return false;
        if (_.functions.pct() < min_pct) return false;
        if (_.branches.pct() < min_pct) return false;
        return true;
    }
}

pub struct LineHit {
    line: u32,
    hits: u32,
}

fn icovWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn icovParseU32(s: string): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        acc = acc * 10 + u32(s[i] - '0');
        i += 1;
    }
    return acc;
}

pub fn formatLcovRecord(file_path: string, hits: []const LineHit, out: []u8): usize {
    let pos: usize = 0;
    pos = writeStr("SF:", out, pos);
    pos = writeStr(file_path, out, pos);
    pos = icovWriteByte(out, pos, '\n');

    let covered: u32 = 0;
    let i: usize = 0;
    while (i < hits.len) {
        const h = hits[i];
        pos = writeStr("DA:", out, pos);
        pos = writeU64(u64(h.line), out, pos);
        pos = icovWriteByte(out, pos, ',');
        pos = writeU64(u64(h.hits), out, pos);
        pos = icovWriteByte(out, pos, '\n');
        if (h.hits > 0) {
            covered += 1;
        }
        i += 1;
    }

    const total: u32 = u32(hits.len);
    pos = writeStr("LF:", out, pos);
    pos = writeU64(u64(total), out, pos);
    pos = icovWriteByte(out, pos, '\n');

    pos = writeStr("LH:", out, pos);
    pos = writeU64(u64(covered), out, pos);
    pos = icovWriteByte(out, pos, '\n');

    pos = writeStr("end_of_record\n", out, pos);
    return pos;
}

pub fn parseLcovSummary(lcov_text: string): CoverageMetric {
    let total: u32 = 0;
    let covered: u32 = 0;

    let rec_da_total: u32 = 0;
    let rec_da_covered: u32 = 0;
    let rec_lf: u32 = 0;
    let rec_lh: u32 = 0;
    let has_lf: bool = false;
    let has_lh: bool = false;
    let has_rec: bool = false;

    let i: usize = 0;
    while (i <= lcov_text.len) {
        let line_start = i;
        while (i < lcov_text.len && lcov_text[i] != '\n') {
            i += 1;
        }
        const raw_line = lcov_text[line_start..i];
        const line = trim(raw_line);
        i += 1;

        if (line.len == 0) {
            continue;
        }

        if (strEq(line, "end_of_record")) {
            if (has_rec) {
                if (has_lf) {
                    total += rec_lf;
                } else {
                    total += rec_da_total;
                }
                if (has_lh) {
                    covered += rec_lh;
                } else {
                    covered += rec_da_covered;
                }
            }
            rec_da_total = 0;
            rec_da_covered = 0;
            rec_lf = 0;
            rec_lh = 0;
            has_lf = false;
            has_lh = false;
            has_rec = false;
        } else if (startsWith(line, "DA:")) {
            const body = line[3..line.len];
            if (comma := findByte(body, ',', 0)) {
                const hits_str = body[comma + 1..body.len];
                const hit_count = icovParseU32(hits_str);
                rec_da_total += 1;
                if (hit_count > 0) {
                    rec_da_covered += 1;
                }
                has_rec = true;
            }
        } else if (startsWith(line, "LF:")) {
            const body = trim(line[3..line.len]);
            rec_lf = icovParseU32(body);
            has_lf = true;
            has_rec = true;
        } else if (startsWith(line, "LH:")) {
            const body = trim(line[3..line.len]);
            rec_lh = icovParseU32(body);
            has_lh = true;
            has_rec = true;
        }
    }

    if (has_rec) {
        if (has_lf) {
            total += rec_lf;
        } else {
            total += rec_da_total;
        }
        if (has_lh) {
            covered += rec_lh;
        } else {
            covered += rec_da_covered;
        }
    }

    return CoverageMetric{
        total: total,
        covered: covered,
        skipped: 0,
    };
}
