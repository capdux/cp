// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc LCOV .info Tracefile Parser)

/// LCOV `.info` 记录类型枚举
pub enum LcovRecordKind: u8 {
    test_name = 0,
    source_file = 1,
    fn_def = 2,
    fn_data = 3,
    fn_found = 4,
    fn_hit = 5,
    line_data = 6,
    line_found = 7,
    line_hit = 8,
    branch_data = 9,
    branch_found = 10,
    branch_hit = 11,
    end_of_record = 12,
    unknown = 13,
}

/// `DA:<line>,<hits>[,checksum]` 行覆盖记录
pub struct LcovLineHit {
    line: u32,
    hits: u64,
}

/// `BRDA:<line>,<block>,<branch>,<taken|->` 分支覆盖记录
pub struct LcovBranchHit {
    line: u32,
    block: u32,
    branch: u32,
    taken: u64,
    executed: bool,
}

/// `FN:<line>,<name>` 函数定义记录
pub struct LcovFnDef {
    line: u32,
    name: []const u8,
}

/// `FNDA:<hits>,<name>` 函数命中记录
pub struct LcovFnHit {
    hits: u64,
    name: []const u8,
}

/// 零拷贝单条 LCOV 解析记录
pub struct LcovRecord {
    kind: LcovRecordKind,
    text: []const u8,
    count: u32,
    line_hit: LcovLineHit,
    branch_hit: LcovBranchHit,
    fn_def: LcovFnDef,
    fn_hit: LcovFnHit,
}

/// 单个源文件的 LCOV 覆盖率汇总
pub struct LcovFileSummary {
    test_name: []const u8,
    source_file: []const u8,
    lines_found: u32,
    lines_hit: u32,
    branches_found: u32,
    branches_hit: u32,
    functions_found: u32,
    functions_hit: u32,

    /// 行覆盖率百分比（0.0..100.0；`lines_found == 0` 返回 `0.0`）
    fn lineCoveragePct(): f64 {
        return calcPct(_.lines_hit, _.lines_found);
    }

    /// 分支覆盖率百分比（0.0..100.0；`branches_found == 0` 返回 `0.0`）
    fn branchCoveragePct(): f64 {
        return calcPct(_.branches_hit, _.branches_found);
    }

    /// 函数覆盖率百分比（0.0..100.0；`functions_found == 0` 返回 `0.0`）
    fn fnCoveragePct(): f64 {
        return calcPct(_.functions_hit, _.functions_found);
    }
}

/// 整个 `.info` 报告的总覆盖率汇总
pub struct LcovTotals {
    file_count: u32,
    lines_found: u32,
    lines_hit: u32,
    branches_found: u32,
    branches_hit: u32,
    functions_found: u32,
    functions_hit: u32,

    fn lineCoveragePct(): f64 {
        return calcPct(_.lines_hit, _.lines_found);
    }

    fn branchCoveragePct(): f64 {
        return calcPct(_.branches_hit, _.branches_found);
    }

    fn fnCoveragePct(): f64 {
        return calcPct(_.functions_hit, _.functions_found);
    }
}

/// 零堆分配 LCOV 逐行记录流式迭代器（兼容 `\n` 与 `\r\n`）
pub struct LcovRecordIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(text: []const u8): LcovRecordIter {
        return LcovRecordIter {
            data: text,
            pos: 0,
        };
    }

    /// 读取下一条非空 LCOV 记录；到达输入末尾时返回 `null`
    fn next(*_): LcovRecord? {
        while (_.pos < _.data.len) {
            const start = _.pos;
            let end = start;
            while (end < _.data.len && _.data[end] != '\n') {
                end += 1;
            }
            _.pos = if (end < _.data.len) end + 1 else end;

            let line_end = end;
            if (line_end > start && _.data[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (line_end == start) {
                continue; // 跳过空行
            }

            return parseLcovLine(_.data[start..line_end]);
        }
        return null;
    }
}

/// 零堆分配 LCOV 文件级覆盖率聚合迭代器（按 `end_of_record` 分段，缺失 `LF`/`LH` 自动回退 `DA` 统计）
pub struct LcovFileIter {
    rec_iter: LcovRecordIter,

    static fn init(text: []const u8): LcovFileIter {
        return LcovFileIter {
            rec_iter: LcovRecordIter.init(text),
        };
    }

    /// 聚合并返回下一个源文件的 `LcovFileSummary`；到达流尾返回 `null`
    fn next(*_): LcovFileSummary? {
        const empty = _.rec_iter.data[0..0];
        let test_name: []const u8 = empty;
        let source_file: []const u8 = empty;
        let seen_any: bool = false;

        let lf: u32 = 0;
        let lh: u32 = 0;
        let has_lf: bool = false;
        let has_lh: bool = false;
        let da_found: u32 = 0;
        let da_hit: u32 = 0;

        let brf: u32 = 0;
        let brh: u32 = 0;
        let has_brf: bool = false;
        let has_brh: bool = false;
        let brda_found: u32 = 0;
        let brda_hit: u32 = 0;

        let fnf: u32 = 0;
        let fnh: u32 = 0;
        let has_fnf: bool = false;
        let has_fnh: bool = false;
        let fn_found_calc: u32 = 0;
        let fn_hit_calc: u32 = 0;

        while (rec := _.rec_iter.next()) {
            if (rec.kind == LcovRecordKind.end_of_record) {
                if (seen_any) {
                    return LcovFileSummary {
                        test_name: test_name,
                        source_file: source_file,
                        lines_found: if (has_lf) lf else da_found,
                        lines_hit: if (has_lh) lh else da_hit,
                        branches_found: if (has_brf) brf else brda_found,
                        branches_hit: if (has_brh) brh else brda_hit,
                        functions_found: if (has_fnf) fnf else fn_found_calc,
                        functions_hit: if (has_fnh) fnh else fn_hit_calc,
                    };
                }
                continue;
            }

            if (rec.kind == LcovRecordKind.test_name) {
                test_name = rec.text;
                seen_any = true;
            } else if (rec.kind == LcovRecordKind.source_file) {
                source_file = rec.text;
                seen_any = true;
            } else if (rec.kind == LcovRecordKind.line_data) {
                seen_any = true;
                da_found += 1;
                if (rec.line_hit.hits > 0) {
                    da_hit += 1;
                }
            } else if (rec.kind == LcovRecordKind.line_found) {
                seen_any = true;
                lf = rec.count;
                has_lf = true;
            } else if (rec.kind == LcovRecordKind.line_hit) {
                seen_any = true;
                lh = rec.count;
                has_lh = true;
            } else if (rec.kind == LcovRecordKind.branch_data) {
                seen_any = true;
                brda_found += 1;
                if (rec.branch_hit.executed && rec.branch_hit.taken > 0) {
                    brda_hit += 1;
                }
            } else if (rec.kind == LcovRecordKind.branch_found) {
                seen_any = true;
                brf = rec.count;
                has_brf = true;
            } else if (rec.kind == LcovRecordKind.branch_hit) {
                seen_any = true;
                brh = rec.count;
                has_brh = true;
            } else if (rec.kind == LcovRecordKind.fn_def) {
                seen_any = true;
                fn_found_calc += 1;
            } else if (rec.kind == LcovRecordKind.fn_data) {
                seen_any = true;
                if (rec.fn_hit.hits > 0) {
                    fn_hit_calc += 1;
                }
            } else if (rec.kind == LcovRecordKind.fn_found) {
                seen_any = true;
                fnf = rec.count;
                has_fnf = true;
            } else if (rec.kind == LcovRecordKind.fn_hit) {
                seen_any = true;
                fnh = rec.count;
                has_fnh = true;
            }
        }

        if (seen_any) {
            return LcovFileSummary {
                test_name: test_name,
                source_file: source_file,
                lines_found: if (has_lf) lf else da_found,
                lines_hit: if (has_lh) lh else da_hit,
                branches_found: if (has_brf) brf else brda_found,
                branches_hit: if (has_brh) brh else brda_hit,
                functions_found: if (has_fnf) fnf else fn_found_calc,
                functions_hit: if (has_fnh) fnh else fn_hit_calc,
            };
        }
        return null;
    }
}

fn calcPct(hit: u32, found: u32): f64 {
    if (found == 0) return 0.0;
    return ((hit as f64) * 100.0) / (found as f64);
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, ch: u8, start: usize): usize? {
    let i = start;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return null;
        if (acc > 1844674407370955161) return null;
        acc = acc * 10 + ((ch - '0') as u64);
        i += 1;
    }
    return acc;
}

fn parseU32(s: []const u8): u32? {
    const v = parseU64(s) ?? return null;
    if (v > 0xFFFFFFFF) return null;
    return v as u32;
}

fn makeEmptyRecord(kind: LcovRecordKind, text: []const u8): LcovRecord {
    const empty = text[0..0];
    return LcovRecord {
        kind: kind,
        text: text,
        count: 0,
        line_hit: LcovLineHit { line: 0, hits: 0 },
        branch_hit: LcovBranchHit { line: 0, block: 0, branch: 0, taken: 0, executed: false },
        fn_def: LcovFnDef { line: 0, name: empty },
        fn_hit: LcovFnHit { hits: 0, name: empty },
    };
}

fn parseLcovLine(line: []const u8): LcovRecord {
    if (bytesEql(line, "end_of_record")) {
        return makeEmptyRecord(LcovRecordKind.end_of_record, line);
    }

    const colon = findByte(line, ':', 0) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
    const tag = line[0..colon];
    const body = line[colon + 1 .. line.len];

    if (bytesEql(tag, "TN")) {
        return makeEmptyRecord(LcovRecordKind.test_name, body);
    }
    if (bytesEql(tag, "SF")) {
        return makeEmptyRecord(LcovRecordKind.source_file, body);
    }

    if (bytesEql(tag, "LF")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.line_found, body);
        rec.count = v;
        return rec;
    }
    if (bytesEql(tag, "LH")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.line_hit, body);
        rec.count = v;
        return rec;
    }
    if (bytesEql(tag, "FNF")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.fn_found, body);
        rec.count = v;
        return rec;
    }
    if (bytesEql(tag, "FNH")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.fn_hit, body);
        rec.count = v;
        return rec;
    }
    if (bytesEql(tag, "BRF")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.branch_found, body);
        rec.count = v;
        return rec;
    }
    if (bytesEql(tag, "BRH")) {
        const v = parseU32(body) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.branch_hit, body);
        rec.count = v;
        return rec;
    }

    if (bytesEql(tag, "DA")) {
        const c1 = findByte(body, ',', 0) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const line_no = parseU32(body[0..c1]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const rest = body[c1 + 1 .. body.len];
        const hits_slice = if (c2 := findByte(rest, ',', 0)) rest[0..c2] else rest;
        const hits = parseU64(hits_slice) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        let rec = makeEmptyRecord(LcovRecordKind.line_data, body);
        rec.line_hit = LcovLineHit { line: line_no, hits: hits };
        return rec;
    }

    if (bytesEql(tag, "BRDA")) {
        const c1 = findByte(body, ',', 0) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const c2 = findByte(body, ',', c1 + 1) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const c3 = findByte(body, ',', c2 + 1) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);

        const line_no = parseU32(body[0..c1]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const block_no = parseU32(body[c1 + 1 .. c2]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const branch_no = parseU32(body[c2 + 1 .. c3]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const taken_str = body[c3 + 1 .. body.len];

        let taken: u64 = 0;
        let executed: bool = false;
        if (bytesEql(taken_str, "-")) {
            executed = false;
            taken = 0;
        } else {
            taken = parseU64(taken_str) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
            executed = true;
        }

        let rec = makeEmptyRecord(LcovRecordKind.branch_data, body);
        rec.branch_hit = LcovBranchHit {
            line: line_no,
            block: block_no,
            branch: branch_no,
            taken: taken,
            executed: executed,
        };
        return rec;
    }

    if (bytesEql(tag, "FN")) {
        const c1 = findByte(body, ',', 0) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const line_no = parseU32(body[0..c1]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const name = body[c1 + 1 .. body.len];
        let rec = makeEmptyRecord(LcovRecordKind.fn_def, body);
        rec.fn_def = LcovFnDef { line: line_no, name: name };
        return rec;
    }

    if (bytesEql(tag, "FNDA")) {
        const c1 = findByte(body, ',', 0) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const hits = parseU64(body[0..c1]) ?? return makeEmptyRecord(LcovRecordKind.unknown, line);
        const name = body[c1 + 1 .. body.len];
        let rec = makeEmptyRecord(LcovRecordKind.fn_data, body);
        rec.fn_hit = LcovFnHit { hits: hits, name: name };
        return rec;
    }

    return makeEmptyRecord(LcovRecordKind.unknown, line);
}

/// 单遍汇总整份 LCOV `.info` 报告的文件数与总行/分支/函数覆盖率
pub fn summarizeLcov(text: []const u8): LcovTotals {
    let totals = LcovTotals {
        file_count: 0,
        lines_found: 0,
        lines_hit: 0,
        branches_found: 0,
        branches_hit: 0,
        functions_found: 0,
        functions_hit: 0,
    };

    let it = LcovFileIter.init(text);
    while (f := it.next()) {
        totals.file_count += 1;
        totals.lines_found += f.lines_found;
        totals.lines_hit += f.lines_hit;
        totals.branches_found += f.branches_found;
        totals.branches_hit += f.branches_hit;
        totals.functions_found += f.functions_found;
        totals.functions_hit += f.functions_hit;
    }
    return totals;
}
