// Source: Absorbed from /data/dev/bun/src/options_types/CodeCoverageOptions.cp

/// 单行代码执行命中记录
pub struct LineHit {
    line_no: u32 = 0,
    hits: u64 = 0,
}

/// 覆盖率门禁阈值（百分比 0.0..100.0）
pub struct CoverageThresholds {
    lines_pct: f32 = 0.0,
    functions_pct: f32 = 0.0,
    branches_pct: f32 = 0.0,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlBytes(s[0..prefix.len], prefix);
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) { return null; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        const d: u64 = @as(u64, c - '0');
        if (acc > ((@as(u64, 18446744073709551615) - d) / 10)) { return null; }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

fn parseU32(s: []const u8): u32? {
    if (v := parseU64(s)) {
        if (v > 4294967295) { return null; }
        return @as(u32, @intCast(v));
    }
    return null;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeU64(*_, val: u64) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 单文件 LCOV 代码覆盖率记录（零堆分配，固定容量 128 行命中项）
pub struct FileCoverage {
    source_file: []const u8 = "",
    lines: [128]LineHit = [LineHit{}; 128],
    line_count: usize = 0,
    lines_found: u32 = 0,
    lines_hit: u32 = 0,
    functions_found: u32 = 0,
    functions_hit: u32 = 0,
    branches_found: u32 = 0,
    branches_hit: u32 = 0,

    static fn init(source_file: []const u8): FileCoverage {
        return FileCoverage{ source_file: source_file };
    }

    /// 记录或累加指定行号的命中次数，自动维护 `lines_found` 与 `lines_hit`
    fn recordLineHit(*_, line_no: u32, hits: u64): bool {
        if (line_no == 0) { return false; }

        let i: usize = 0;
        while (i < _.line_count) {
            if (_.lines[i].line_no == line_no) {
                if (_.lines[i].hits == 0 && hits > 0) {
                    _.lines_hit += 1;
                }
                _.lines[i].hits += hits;
                return true;
            }
            i += 1;
        }

        if (_.line_count >= 128) {
            return false;
        }
        _.lines[_.line_count] = LineHit{ line_no: line_no, hits: hits };
        _.line_count += 1;
        _.lines_found += 1;
        if (hits > 0) {
            _.lines_hit += 1;
        }
        return true;
    }

    /// 行覆盖率百分比（0 行文件按 LCOV 惯例返回 100.0）
    fn lineCoveragePct(): f32 {
        if (_.lines_found == 0) { return 100.0; }
        return (@as(f32, @floatFromInt(_.lines_hit)) * 100.0) / @as(f32, @floatFromInt(_.lines_found));
    }

    /// 函数覆盖率百分比（0 函数文件返回 100.0）
    fn functionCoveragePct(): f32 {
        if (_.functions_found == 0) { return 100.0; }
        return (@as(f32, @floatFromInt(_.functions_hit)) * 100.0) / @as(f32, @floatFromInt(_.functions_found));
    }

    /// 分支覆盖率百分比（0 分支文件返回 100.0）
    fn branchCoveragePct(): f32 {
        if (_.branches_found == 0) { return 100.0; }
        return (@as(f32, @floatFromInt(_.branches_hit)) * 100.0) / @as(f32, @floatFromInt(_.branches_found));
    }

    /// 校验当前文件是否满足全部覆盖率门禁阈值
    fn meetsThresholds(t: CoverageThresholds): bool {
        return _.lineCoveragePct() >= t.lines_pct &&
            _.functionCoveragePct() >= t.functions_pct &&
            _.branchCoveragePct() >= t.branches_pct;
    }

    /// 序列化为标准 LCOV 记录块写入 `out`，缓冲区不足返回 `null`
    fn formatLcov(out: []u8): ([]const u8)? {
        let w = OutWriter{ buf: out };
        w.writeStr("SF:");
        w.writeStr(_.source_file);
        w.writeByte('\n');

        if (_.functions_found > 0) {
            w.writeStr("FNF:");
            w.writeU64(@as(u64, _.functions_found));
            w.writeByte('\n');
            w.writeStr("FNH:");
            w.writeU64(@as(u64, _.functions_hit));
            w.writeByte('\n');
        }

        if (_.branches_found > 0) {
            w.writeStr("BRF:");
            w.writeU64(@as(u64, _.branches_found));
            w.writeByte('\n');
            w.writeStr("BRH:");
            w.writeU64(@as(u64, _.branches_hit));
            w.writeByte('\n');
        }

        let i: usize = 0;
        while (i < _.line_count) {
            w.writeStr("DA:");
            w.writeU64(@as(u64, _.lines[i].line_no));
            w.writeByte(',');
            w.writeU64(_.lines[i].hits);
            w.writeByte('\n');
            i += 1;
        }

        w.writeStr("LF:");
        w.writeU64(@as(u64, _.lines_found));
        w.writeByte('\n');
        w.writeStr("LH:");
        w.writeU64(@as(u64, _.lines_hit));
        w.writeByte('\n');
        w.writeStr("end_of_record\n");

        return w.finish();
    }
}

/// 解析单个标准 LCOV 记录块（支持 `SF:`、`DA:`、`FNF:`/`FNH:`、`BRF:`/`BRH:`、`LF:`/`LH:` 与 `end_of_record`）
pub fn parseLcovRecord(input: []const u8): FileCoverage? {
    let cov = FileCoverage{};
    let has_sf = false;
    let has_end = false;
    let saw_da = false;
    let explicit_lf: u32? = null;
    let explicit_lh: u32? = null;

    let pos: usize = 0;
    while (pos < input.len) {
        const line_start = pos;
        while (pos < input.len && input[pos] != '\n') {
            pos += 1;
        }
        let line_end = pos;
        if (line_end > line_start && input[line_end - 1] == '\r') {
            line_end -= 1;
        }
        if (pos < input.len && input[pos] == '\n') {
            pos += 1;
        }

        const line = input[line_start..line_end];
        if (line.len == 0) {
            continue;
        }

        if (eqlBytes(line, "end_of_record")) {
            has_end = true;
            break;
        }

        if (startsWith(line, "SF:")) {
            const path = line[3..line.len];
            if (path.len == 0) { return null; }
            cov.source_file = path;
            has_sf = true;
        } else if (startsWith(line, "DA:")) {
            const payload = line[3..line.len];
            let comma1: usize? = null;
            let k: usize = 0;
            while (k < payload.len) {
                if (payload[k] == ',') {
                    comma1 = k;
                    break;
                }
                k += 1;
            }
            if (c1 := comma1) {
                const line_str = payload[0..c1];
                let hits_end = payload.len;
                let m = c1 + 1;
                while (m < payload.len) {
                    if (payload[m] == ',') {
                        hits_end = m;
                        break;
                    }
                    m += 1;
                }
                const hits_str = payload[(c1 + 1)..hits_end];
                if (ln := parseU32(line_str)) {
                    if (hc := parseU64(hits_str)) {
                        if (!cov.recordLineHit(ln, hc)) { return null; }
                        saw_da = true;
                    } else {
                        return null;
                    }
                } else {
                    return null;
                }
            } else {
                return null;
            }
        } else if (startsWith(line, "FNF:")) {
            if (v := parseU32(line[4..line.len])) { cov.functions_found = v; } else { return null; }
        } else if (startsWith(line, "FNH:")) {
            if (v := parseU32(line[4..line.len])) { cov.functions_hit = v; } else { return null; }
        } else if (startsWith(line, "BRF:")) {
            if (v := parseU32(line[4..line.len])) { cov.branches_found = v; } else { return null; }
        } else if (startsWith(line, "BRH:")) {
            if (v := parseU32(line[4..line.len])) { cov.branches_hit = v; } else { return null; }
        } else if (startsWith(line, "LF:")) {
            if (v := parseU32(line[3..line.len])) { explicit_lf = v; } else { return null; }
        } else if (startsWith(line, "LH:")) {
            if (v := parseU32(line[3..line.len])) { explicit_lh = v; } else { return null; }
        } else if (startsWith(line, "TN:") || startsWith(line, "FN:") || startsWith(line, "FNDA:") || startsWith(line, "BRDA:")) {
            continue;
        } else {
            return null;
        }
    }

    if (!has_sf || !has_end) {
        return null;
    }
    if (!saw_da) {
        if (lf := explicit_lf) { cov.lines_found = lf; }
        if (lh := explicit_lh) { cov.lines_hit = lh; }
    }
    return cov;
}
