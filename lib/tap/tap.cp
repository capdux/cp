// Origin: Pure-CP Stdlib (Zero-Alloc TAP v12/v13/v14 Test Protocol Parser) | Created: 2026-11-22

/// TAP 事件类型枚举
pub enum TapEventKind: u8 {
    version = 0,
    plan = 1,
    test_point = 2,
    bail_out = 3,
    yaml_block = 4,
    comment = 5,
    pragma = 6,
}

/// TAP 测试点/计划指令类型
pub enum TapDirective: u8 {
    none = 0,
    skip = 1,
    todo = 2,
}

/// 零拷贝 TAP 事件对象
pub struct TapEvent {
    kind: TapEventKind,
    version: u32,
    plan_start: u32,
    plan_end: u32,
    ok: bool,
    test_number: u32,
    has_number: bool,
    description: []const u8,
    directive: TapDirective,
    reason: []const u8,
    raw_text: []const u8,
}

/// TAP 流单遍汇总统计
pub struct TapSummary {
    version: u32,
    planned: u32,
    has_plan: bool,
    total: u32,
    passed: u32,
    failed: u32,
    skipped: u32,
    todo_passed: u32,
    todo_failed: u32,
    bailed_out: bool,

    /// 按 TAP 规范判定整体是否通过：`!bailed_out && failed == 0 && (!has_plan || total == planned)`
    fn allPassed(): bool {
        if (_.bailed_out || _.failed > 0) {
            return false;
        }
        if (_.has_plan && _.total != _.planned) {
            return false;
        }
        return true;
    }
}

/// 零分配 TAP v12/v13/v14 流式事件迭代器
pub struct TapIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): TapIter {
        return TapIter { data: data, pos: 0 };
    }

    fn next(*_): TapEvent? {
        while (_.pos < _.data.len) {
            const lv = nextLine(_.data, _.pos) ?? return null;
            _.pos = lv.next_pos;

            const trimmed = trimAscii(lv.line);
            if (trimmed.len == 0) {
                continue;
            }

            // 1. 缩进 YAML 诊断块（`  ---` ... `  ...`）
            const has_indent = lv.line.len > 0 && (lv.line[0] == ' ' || lv.line[0] == '\t');
            if (has_indent && bytesEq(trimmed, "---")) {
                const block_start = lv.line_start;
                let block_end = lv.line_end;
                while (_.pos < _.data.len) {
                    const ylv = nextLine(_.data, _.pos) ?? break;
                    _.pos = ylv.next_pos;
                    block_end = ylv.line_end;
                    if (bytesEq(trimAscii(ylv.line), "...")) {
                        break;
                    }
                }
                return makeRawEvent(.yaml_block, _.data[block_start..block_end]);
            }

            // 2. 版本行：TAP version N
            if (ev_ver := tryParseVersion(trimmed)) {
                return ev_ver;
            }

            // 3. 提前终止：Bail out! [reason]
            if (ev_bail := tryParseBailOut(trimmed)) {
                return ev_bail;
            }

            // 4. Pragma：pragma +strict / -strict
            if (ev_pragma := tryParsePragma(trimmed)) {
                return ev_pragma;
            }

            // 5. 注释：# ...
            if (trimmed[0] == '#') {
                const cmt = trimLeadingSpace(trimmed[1..trimmed.len]);
                return makeRawEvent(.comment, cmt);
            }

            // 6. 计划行：1..N [# SKIP reason]
            if (ev_plan := tryParsePlan(trimmed)) {
                return ev_plan;
            }

            // 7. 测试点：ok / not ok
            if (ev_tp := tryParseTestPoint(trimmed)) {
                return ev_tp;
            }

            // 未知诊断文本按 comment 透传（符合 TAP 容错惯例）
            return makeRawEvent(.comment, trimmed);
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 内部行切分与大小写不敏感匹配辅助函数
// ---------------------------------------------------------------------------

struct LineView {
    line: []const u8,
    line_start: usize,
    line_end: usize,
    next_pos: usize,
}

fn nextLine(data: []const u8, start: usize): LineView? {
    if (start >= data.len) {
        return null;
    }
    let i: usize = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let end = i;
    if (end > start && data[end - 1] == '\r') {
        end -= 1;
    }
    const next_pos: usize = if (i < data.len) i + 1 else i;
    return LineView {
        line: data[start..end],
        line_start: start,
        line_end: end,
        next_pos: next_pos,
    };
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn trimLeadingSpace(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return s[i..s.len];
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) return null;
        acc = acc * 10 + ((s[i] - '0') as u64);
        if (acc > 4294967295) return null;
        i += 1;
    }
    return acc as u32;
}

fn makeRawEvent(kind: TapEventKind, raw: []const u8): TapEvent {
    return TapEvent {
        kind: kind,
        version: 0,
        plan_start: 0,
        plan_end: 0,
        ok: false,
        test_number: 0,
        has_number: false,
        description: raw[0..0],
        directive: .none,
        reason: raw[0..0],
        raw_text: raw,
    };
}

fn tryParseVersion(line: []const u8): TapEvent? {
    if (!startsWithIgnoreCase(line, "TAP version ")) {
        return null;
    }
    const ver_str = trimAscii(line[12..line.len]);
    const ver = parseU32(ver_str) ?? return null;
    let ev = makeRawEvent(.version, line);
    ev.version = ver;
    return ev;
}

fn tryParseBailOut(line: []const u8): TapEvent? {
    if (!startsWithIgnoreCase(line, "Bail out!")) {
        return null;
    }
    if (line.len > 9 && !isSpace(line[9])) {
        return null;
    }
    const reason = if (line.len > 9) trimAscii(line[9..line.len]) else line[line.len..line.len];
    let ev = makeRawEvent(.bail_out, reason);
    ev.reason = reason;
    return ev;
}

fn tryParsePragma(line: []const u8): TapEvent? {
    if (!startsWithIgnoreCase(line, "pragma ")) {
        return null;
    }
    const body = trimAscii(line[7..line.len]);
    if (body.len == 0) {
        return null;
    }
    return makeRawEvent(.pragma, body);
}

fn tryParsePlan(line: []const u8): TapEvent? {
    let i: usize = 0;
    while (i < line.len && isDigit(line[i])) {
        i += 1;
    }
    if (i == 0 || i + 2 > line.len || line[i] != '.' || line[i + 1] != '.') {
        return null;
    }
    const start_num = parseU32(line[0..i]) ?? return null;
    i += 2;
    const end_start = i;
    while (i < line.len && isDigit(line[i])) {
        i += 1;
    }
    if (i == end_start) {
        return null;
    }
    const end_num = parseU32(line[end_start..i]) ?? return null;

    let directive: TapDirective = .none;
    let reason: []const u8 = line[line.len..line.len];

    if (i < line.len) {
        if (!isSpace(line[i])) {
            return null;
        }
        const rest = trimAscii(line[i..line.len]);
        if (rest.len > 0) {
            if (rest[0] != '#') {
                return null;
            }
            const after_hash = trimLeadingSpace(rest[1..rest.len]);
            if (startsWithIgnoreCase(after_hash, "SKIP")) {
                directive = .skip;
                reason = trimAscii(after_hash[4..after_hash.len]);
            } else {
                reason = after_hash;
            }
        }
    }

    return TapEvent {
        kind: .plan,
        version: 0,
        plan_start: start_num,
        plan_end: end_num,
        ok: false,
        test_number: 0,
        has_number: false,
        description: line[0..0],
        directive: directive,
        reason: reason,
        raw_text: line,
    };
}

fn stripLeadingDash(s: []const u8): []const u8 {
    const t = trimAscii(s);
    if (t.len >= 2 && t[0] == '-' && isSpace(t[1])) {
        return trimAscii(t[2..t.len]);
    }
    if (t.len == 1 && t[0] == '-') {
        return t[1..1];
    }
    return t;
}

fn tryParseTestPoint(line: []const u8): TapEvent? {
    let ok: bool = false;
    let pos: usize = 0;

    if (line.len >= 2 && line[0] == 'o' && line[1] == 'k' && (line.len == 2 || isSpace(line[2]))) {
        ok = true;
        pos = 2;
    } else if (
        line.len >= 6 &&
        line[0] == 'n' && line[1] == 'o' && line[2] == 't' &&
        line[3] == ' ' && line[4] == 'o' && line[5] == 'k' &&
        (line.len == 6 || isSpace(line[6]))
    ) {
        ok = false;
        pos = 6;
    } else {
        return null;
    }

    while (pos < line.len && isSpace(line[pos])) {
        pos += 1;
    }

    // 可选测试编号
    let has_number: bool = false;
    let test_number: u32 = 0;
    if (pos < line.len && isDigit(line[pos])) {
        let num_end = pos;
        while (num_end < line.len && isDigit(line[num_end])) {
            num_end += 1;
        }
        if (num_end == line.len || isSpace(line[num_end])) {
            if (num := parseU32(line[pos..num_end])) {
                has_number = true;
                test_number = num;
                pos = num_end;
            }
        }
    }

    const rest = trimAscii(line[pos..line.len]);

    // 扫描未转义 '#'（忽略 `\#`）以提取 `# SKIP` / `# TODO` 指令
    let directive: TapDirective = .none;
    let reason: []const u8 = rest[rest.len..rest.len];
    let desc_raw: []const u8 = rest;

    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == '\\' && i + 1 < rest.len && rest[i + 1] == '#') {
            i += 2;
            continue;
        }
        if (rest[i] == '#') {
            const after = trimLeadingSpace(rest[i + 1..rest.len]);
            if (
                startsWithIgnoreCase(after, "SKIP") &&
                (after.len == 4 || isSpace(after[4]) || after[4] == ':')
            ) {
                directive = .skip;
                const r_start: usize = if (after.len > 4 && after[4] == ':') 5 else 4;
                reason = trimAscii(after[r_start..after.len]);
                desc_raw = rest[0..i];
                break;
            }
            if (
                startsWithIgnoreCase(after, "TODO") &&
                (after.len == 4 || isSpace(after[4]) || after[4] == ':')
            ) {
                directive = .todo;
                const r_start: usize = if (after.len > 4 && after[4] == ':') 5 else 4;
                reason = trimAscii(after[r_start..after.len]);
                desc_raw = rest[0..i];
                break;
            }
        }
        i += 1;
    }

    return TapEvent {
        kind: .test_point,
        version: 0,
        plan_start: 0,
        plan_end: 0,
        ok: ok,
        test_number: test_number,
        has_number: has_number,
        description: stripLeadingDash(desc_raw),
        directive: directive,
        reason: reason,
        raw_text: line,
    };
}

// ---------------------------------------------------------------------------
// 公开汇总函数
// ---------------------------------------------------------------------------

/// 单遍零分配汇总整个 TAP 测试结果流
pub fn summarizeTap(data: []const u8): TapSummary {
    let summary = TapSummary {
        version: 12,
        planned: 0,
        has_plan: false,
        total: 0,
        passed: 0,
        failed: 0,
        skipped: 0,
        todo_passed: 0,
        todo_failed: 0,
        bailed_out: false,
    };

    let it = TapIter.init(data);
    while (true) {
        const ev_opt = it.next();
        if (ev_opt == null) {
            break;
        }
        const ev = ev_opt ?? break;

        match (ev.kind) {
            .version => {
                summary.version = ev.version;
            },
            .plan => {
                summary.has_plan = true;
                if (ev.plan_end >= ev.plan_start && ev.plan_start > 0) {
                    summary.planned = (ev.plan_end - ev.plan_start) + 1;
                } else {
                    summary.planned = 0;
                }
            },
            .bail_out => {
                summary.bailed_out = true;
            },
            .test_point => {
                summary.total += 1;
                if (ev.directive == .skip) {
                    summary.skipped += 1;
                } else if (ev.directive == .todo) {
                    if (ev.ok) {
                        summary.todo_passed += 1;
                    } else {
                        summary.todo_failed += 1;
                    }
                } else {
                    if (ev.ok) {
                        summary.passed += 1;
                    } else {
                        summary.failed += 1;
                    }
                }
            },
            .yaml_block => {},
            .comment => {},
            .pragma => {},
        }
    }

    return summary;
}
