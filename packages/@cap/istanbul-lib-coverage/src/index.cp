// @cap/istanbul-lib-coverage — Zero-heap code coverage metrics, summary merging, and LCOV codec.

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

fn icovWriteStr(out: []u8, pos: usize, s: string): usize {
    let cur = pos;
    let i: usize = 0;
    while (i < s.len && cur < out.len) {
        out[cur] = s[i];
        cur += 1;
        i += 1;
    }
    return cur;
}

fn icovWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn icovWriteU32(out: []u8, pos: usize, val: u32): usize {
    if (val == 0) {
        return icovWriteByte(out, pos, '0');
    }
    let digits: [16]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0 && n < 16) {
        const rem: u8 = u8(v % 10);
        digits[n] = u8(48) + rem;
        v = v / 10;
        n += 1;
    }
    let cur = pos;
    while (n > 0) {
        n -= 1;
        cur = icovWriteByte(out, cur, digits[n]);
    }
    return cur;
}

fn icovTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn icovHasPrefix(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn icovStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn icovParseU32(s: string): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') break;
        acc = acc * 10 + u32(c - '0');
        i += 1;
    }
    return acc;
}

fn icovIndexOfByte(s: string, target: u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) return i;
        i += 1;
    }
    return s.len;
}

pub fn formatLcovRecord(file_path: string, hits: []const LineHit, out: []u8): usize {
    let pos: usize = 0;
    pos = icovWriteStr(out, pos, "SF:");
    pos = icovWriteStr(out, pos, file_path);
    pos = icovWriteByte(out, pos, '\n');

    let covered: u32 = 0;
    let i: usize = 0;
    while (i < hits.len) {
        const h = hits[i];
        pos = icovWriteStr(out, pos, "DA:");
        pos = icovWriteU32(out, pos, h.line);
        pos = icovWriteByte(out, pos, ',');
        pos = icovWriteU32(out, pos, h.hits);
        pos = icovWriteByte(out, pos, '\n');
        if (h.hits > 0) {
            covered += 1;
        }
        i += 1;
    }

    const total: u32 = u32(hits.len);
    pos = icovWriteStr(out, pos, "LF:");
    pos = icovWriteU32(out, pos, total);
    pos = icovWriteByte(out, pos, '\n');

    pos = icovWriteStr(out, pos, "LH:");
    pos = icovWriteU32(out, pos, covered);
    pos = icovWriteByte(out, pos, '\n');

    pos = icovWriteStr(out, pos, "end_of_record\n");
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
        const line = icovTrim(raw_line);
        i += 1;

        if (line.len == 0) {
            continue;
        }

        if (icovStrEq(line, "end_of_record")) {
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
        } else if (icovHasPrefix(line, "DA:")) {
            const body = line[3..line.len];
            const comma = icovIndexOfByte(body, ',');
            if (comma < body.len) {
                const hits_str = body[comma + 1..body.len];
                const hit_count = icovParseU32(hits_str);
                rec_da_total += 1;
                if (hit_count > 0) {
                    rec_da_covered += 1;
                }
                has_rec = true;
            }
        } else if (icovHasPrefix(line, "LF:")) {
            const body = icovTrim(line[3..line.len]);
            rec_lf = icovParseU32(body);
            has_lf = true;
            has_rec = true;
        } else if (icovHasPrefix(line, "LH:")) {
            const body = icovTrim(line[3..line.len]);
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
