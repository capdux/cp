// Origin: Pure-CP Stdlib (Zero-Alloc Visual Studio / .NET TRX XML Test Parser) | Created: 2026-11-22

/// TRX 测试结果状态枚举
pub enum TrxOutcome: u8 {
    passed = 0,
    failed = 1,
    not_executed = 2,
    inconclusive = 3,
    timeout = 4,
    aborted = 5,
    error_outcome = 6,
    unknown = 7,
}

/// `<TestRun>` 与 `<Times>` 顶层运行元数据（零拷贝切片）
pub struct TrxRunHeader {
    id: []const u8,
    name: []const u8,
    run_user: []const u8,
    creation: []const u8,
    queuing: []const u8,
    start: []const u8,
    finish: []const u8,
}

/// 单条 `<UnitTestResult>` 零拷贝视图
pub struct TrxResult {
    test_name: []const u8,
    execution_id: []const u8,
    test_id: []const u8,
    computer_name: []const u8,
    duration: []const u8,
    outcome: TrxOutcome,
    error_message: []const u8,
    stack_trace: []const u8,
    stdout_text: []const u8,

    /// 将 `"HH:MM:SS.fffffff"`（或 `"D.HH:MM:SS.fffffff"`）耗时字符串精确解析为毫秒
    fn durationMs(): f64 {
        return parseTrxDurationMs(_.duration);
    }
}

/// `<ResultSummary>` 与 `<Counters>` 测试汇总（缺失 `<Counters>` 时自动回退实算）
pub struct TrxSummary {
    outcome: TrxOutcome,
    raw_outcome: []const u8,
    total: u32,
    executed: u32,
    passed: u32,
    failed: u32,
    error_count: u32,
    timeout: u32,
    aborted: u32,
    inconclusive: u32,
    not_executed: u32,

    /// 判断整次测试运行是否全部通过（无失败/错误/超时/中止/未决）
    fn allPassed(): bool {
        if (_.failed > 0 || _.error_count > 0 || _.timeout > 0 || _.aborted > 0 || _.inconclusive > 0) {
            return false;
        }
        if (
            _.outcome == .failed ||
            _.outcome == .error_outcome ||
            _.outcome == .timeout ||
            _.outcome == .aborted ||
            _.outcome == .inconclusive
        ) {
            return false;
        }
        return true;
    }
}

/// 零分配 `<UnitTestResult>` 顺序迭代器
pub struct TrxResultIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): TrxResultIter {
        return TrxResultIter { data: data, pos: 0 };
    }

    fn next(*_): TrxResult? {
        const tag_opt = findOpenTag(_.data, _.pos, "UnitTestResult");
        if (tag_opt == null) {
            _.pos = _.data.len;
            return null;
        }
        const tag = tag_opt ?? return null;
        const empty = _.data[0..0];

        let body: []const u8 = empty;
        if (tag.is_self_closing) {
            _.pos = tag.tag_end;
        } else {
            const close_opt = findCloseTag(_.data, tag.tag_end, "UnitTestResult");
            if (close_opt == null) {
                _.pos = _.data.len;
                return null;
            }
            const close_start = close_opt ?? return null;
            body = _.data[tag.tag_end..close_start];
            _.pos = close_start + 17; // "</UnitTestResult>".len
        }

        const attrs = tag.attrs;
        const outcome_str = findAttr(attrs, "outcome");

        return TrxResult {
            test_name: findAttr(attrs, "testName"),
            execution_id: findAttr(attrs, "executionId"),
            test_id: findAttr(attrs, "testId"),
            computer_name: findAttr(attrs, "computerName"),
            duration: findAttr(attrs, "duration"),
            outcome: parseTrxOutcome(outcome_str),
            error_message: extractElementText(body, "Message"),
            stack_trace: extractElementText(body, "StackTrace"),
            stdout_text: extractElementText(body, "StdOut"),
        };
    }
}

// ---------------------------------------------------------------------------
// 内部零分配 XML 扫描与时间解析辅助函数
// ---------------------------------------------------------------------------

struct OpenTag {
    tag_start: usize,
    attrs: []const u8,
    tag_end: usize,
    is_self_closing: bool,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn bytesEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLower(a[i]) != toLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn parseU32Str(s: []const u8): u32 {
    if (s.len == 0) return 0;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        acc = acc * 10 + ((s[i] - '0') as u64);
        if (acc > 4294967295) return 0;
        i += 1;
    }
    if (i != s.len) return 0;
    return acc as u32;
}

fn parseTrxOutcome(s: []const u8): TrxOutcome {
    if (bytesEqIgnoreCase(s, "Passed")) return .passed;
    if (bytesEqIgnoreCase(s, "Failed")) return .failed;
    if (bytesEqIgnoreCase(s, "NotExecuted")) return .not_executed;
    if (bytesEqIgnoreCase(s, "Inconclusive")) return .inconclusive;
    if (bytesEqIgnoreCase(s, "Timeout")) return .timeout;
    if (bytesEqIgnoreCase(s, "Aborted")) return .aborted;
    if (bytesEqIgnoreCase(s, "Error")) return .error_outcome;
    return .unknown;
}

fn skipXmlProlog(data: []const u8): usize {
    let i: usize = 0;
    // 可选 UTF-8 BOM
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        i = 3;
    }
    while (i < data.len) {
        while (i < data.len && isWs(data[i])) {
            i += 1;
        }
        if (i + 2 <= data.len && data[i] == '<' && data[i + 1] == '?') {
            // 跳过 <?xml ... ?>
            i += 2;
            while (i + 1 < data.len && !(data[i] == '?' && data[i + 1] == '>')) {
                i += 1;
            }
            if (i + 1 >= data.len) return data.len;
            i += 2;
            continue;
        }
        if (i + 4 <= data.len && startsWith(data[i..data.len], "<!--")) {
            i += 4;
            while (i + 2 < data.len && !(data[i] == '-' && data[i + 1] == '-' && data[i + 2] == '>')) {
                i += 1;
            }
            if (i + 2 >= data.len) return data.len;
            i += 3;
            continue;
        }
        break;
    }
    return i;
}

fn isTagBoundary(c: u8): bool {
    return isWs(c) || c == '>' || c == '/';
}

fn findOpenTag(data: []const u8, start: usize, name: []const u8): OpenTag? {
    let i: usize = start;
    while (i < data.len) {
        if (data[i] != '<') {
            i += 1;
            continue;
        }
        // 跳过 XML 注释
        if (i + 4 <= data.len && startsWith(data[i..data.len], "<!--")) {
            i += 4;
            while (i + 2 < data.len && !(data[i] == '-' && data[i + 1] == '-' && data[i + 2] == '>')) {
                i += 1;
            }
            if (i + 2 >= data.len) return null;
            i += 3;
            continue;
        }
        if (i + 1 + name.len < data.len && startsWith(data[i + 1..data.len], name)) {
            const after_name = i + 1 + name.len;
            if (isTagBoundary(data[after_name])) {
                // 扫描到匹配的 '>'（跳过引号内字符）
                let j: usize = after_name;
                let quote: u8 = 0;
                while (j < data.len) {
                    const ch = data[j];
                    if (quote != 0) {
                        if (ch == quote) {
                            quote = 0;
                        }
                    } else {
                        if (ch == '"' || ch == '\'') {
                            quote = ch;
                        } else if (ch == '>') {
                            const self_close = j > after_name && data[j - 1] == '/';
                            const attrs_end = if (self_close) j - 1 else j;
                            return OpenTag {
                                tag_start: i,
                                attrs: data[after_name..attrs_end],
                                tag_end: j + 1,
                                is_self_closing: self_close,
                            };
                        }
                    }
                    j += 1;
                }
                return null;
            }
        }
        i += 1;
    }
    return null;
}

fn findCloseTag(data: []const u8, start: usize, name: []const u8): usize? {
    let i: usize = start;
    const close_len = 2 + name.len + 1; // "</" + name + ">"
    while (i + close_len <= data.len) {
        if (data[i] == '<' && data[i + 1] == '/' && startsWith(data[i + 2..data.len], name)) {
            const gt_pos = i + 2 + name.len;
            if (data[gt_pos] == '>') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn findAttr(attrs: []const u8, target: []const u8): []const u8 {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) break;

        const name_start = i;
        while (i < attrs.len && !isWs(attrs[i]) && attrs[i] != '=' && attrs[i] != '/') {
            i += 1;
        }
        const attr_name = attrs[name_start..i];

        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            // 畸形无值属性，安全跳过
            continue;
        }
        i += 1;
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || (attrs[i] != '"' && attrs[i] != '\'')) {
            return attrs[0..0];
        }
        const q = attrs[i];
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != q) {
            i += 1;
        }
        if (i >= attrs.len) {
            // 未闭合引号
            return attrs[0..0];
        }
        const val = attrs[val_start..i];
        i += 1;

        if (attr_name.len == target.len && startsWith(attr_name, target)) {
            return val;
        }
    }
    return attrs[0..0];
}

fn extractElementText(body: []const u8, tag_name: []const u8): []const u8 {
    const empty = body[0..0];
    const open_opt = findOpenTag(body, 0, tag_name);
    if (open_opt == null) return empty;
    const open = open_opt ?? return empty;
    if (open.is_self_closing) return empty;
    const close_opt = findCloseTag(body, open.tag_end, tag_name);
    if (close_opt == null) return empty;
    const close_pos = close_opt ?? return empty;
    return body[open.tag_end..close_pos];
}

fn parseTrxDurationMs(s: []const u8): f64 {
    if (s.len < 8) return 0.0;
    let i: usize = 0;

    // 读取第一段整数（可能是天 D 或小时 HH）
    let p1: u64 = 0;
    let p1_digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        p1 = p1 * 10 + ((s[i] - '0') as u64);
        p1_digits += 1;
        i += 1;
    }
    if (p1_digits == 0 || i >= s.len) return 0.0;

    let days: u64 = 0;
    let hours: u64 = 0;
    if (s[i] == '.') {
        days = p1;
        i += 1;
        let h_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            hours = hours * 10 + ((s[i] - '0') as u64);
            h_digits += 1;
            i += 1;
        }
        if (h_digits == 0 || i >= s.len || s[i] != ':') return 0.0;
    } else if (s[i] == ':') {
        hours = p1;
    } else {
        return 0.0;
    }

    i += 1; // 跳过 ':'
    let mins: u64 = 0;
    let m_digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        mins = mins * 10 + ((s[i] - '0') as u64);
        m_digits += 1;
        i += 1;
    }
    if (m_digits == 0 || i >= s.len || s[i] != ':') return 0.0;

    i += 1; // 跳过 ':'
    let secs: u64 = 0;
    let s_digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        secs = secs * 10 + ((s[i] - '0') as u64);
        s_digits += 1;
        i += 1;
    }
    if (s_digits == 0) return 0.0;

    let frac_sec: f64 = 0.0;
    if (i < s.len) {
        if (s[i] != '.') return 0.0;
        i += 1;
        let scale: f64 = 0.1;
        let f_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            frac_sec += ((s[i] - '0') as f64) * scale;
            scale *= 0.1;
            f_digits += 1;
            i += 1;
        }
        if (f_digits == 0 || i != s.len) return 0.0;
    }

    const total_secs: f64 =
        ((days * 86400 + hours * 3600 + mins * 60 + secs) as f64) + frac_sec;
    return total_secs * 1000.0;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 嗅探字节流是否为合法 Visual Studio / .NET TRX XML 文档（跳过可选 BOM/声明/注释后匹配 `<TestRun`）
pub fn isTrx(data: []const u8): bool {
    const start = skipXmlProlog(data);
    if (start + 8 >= data.len) {
        return false;
    }
    if (!startsWith(data[start..data.len], "<TestRun")) {
        return false;
    }
    return isTagBoundary(data[start + 8]);
}

/// 解析 `<TestRun>` 与 `<Times>` 顶层运行元数据
pub fn parseTrxHeader(data: []const u8): TrxRunHeader? {
    if (!isTrx(data)) {
        return null;
    }
    const run_tag = findOpenTag(data, 0, "TestRun") ?? return null;
    const empty = data[0..0];

    const times_opt = findOpenTag(data, run_tag.tag_end, "Times");
    const times_attrs = if (times_opt != null) (times_opt ?? run_tag).attrs else empty;

    return TrxRunHeader {
        id: findAttr(run_tag.attrs, "id"),
        name: findAttr(run_tag.attrs, "name"),
        run_user: findAttr(run_tag.attrs, "runUser"),
        creation: if (times_opt != null) findAttr(times_attrs, "creation") else empty,
        queuing: if (times_opt != null) findAttr(times_attrs, "queuing") else empty,
        start: if (times_opt != null) findAttr(times_attrs, "start") else empty,
        finish: if (times_opt != null) findAttr(times_attrs, "finish") else empty,
    };
}

/// 解析 `<ResultSummary>` 与 `<Counters>` 汇总；若缺失 `<Counters>` 则自动回退单遍扫描 `<UnitTestResult>` 实算
pub fn parseTrxSummary(data: []const u8): TrxSummary? {
    if (!isTrx(data)) {
        return null;
    }
    const empty = data[0..0];

    const summary_opt = findOpenTag(data, 0, "ResultSummary");
    const raw_outcome = if (summary_opt != null)
        findAttr((summary_opt ?? OpenTag { tag_start: 0, attrs: empty, tag_end: 0, is_self_closing: true }).attrs, "outcome")
    else
        empty;
    const outcome = parseTrxOutcome(raw_outcome);

    const counters_start: usize = if (summary_opt != null)
        (summary_opt ?? OpenTag { tag_start: 0, attrs: empty, tag_end: 0, is_self_closing: true }).tag_end
    else
        0;
    const counters_opt = findOpenTag(data, counters_start, "Counters");

    if (counters_opt != null) {
        const ca = (counters_opt ?? OpenTag { tag_start: 0, attrs: empty, tag_end: 0, is_self_closing: true }).attrs;
        return TrxSummary {
            outcome: outcome,
            raw_outcome: raw_outcome,
            total: parseU32Str(findAttr(ca, "total")),
            executed: parseU32Str(findAttr(ca, "executed")),
            passed: parseU32Str(findAttr(ca, "passed")),
            failed: parseU32Str(findAttr(ca, "failed")),
            error_count: parseU32Str(findAttr(ca, "error")),
            timeout: parseU32Str(findAttr(ca, "timeout")),
            aborted: parseU32Str(findAttr(ca, "aborted")),
            inconclusive: parseU32Str(findAttr(ca, "inconclusive")),
            not_executed: parseU32Str(findAttr(ca, "notExecuted")),
        };
    }

    // 缺失 <Counters> 时自动回退单遍扫描 <UnitTestResult> 实算
    let sum = TrxSummary {
        outcome: outcome,
        raw_outcome: raw_outcome,
        total: 0,
        executed: 0,
        passed: 0,
        failed: 0,
        error_count: 0,
        timeout: 0,
        aborted: 0,
        inconclusive: 0,
        not_executed: 0,
    };

    let it = TrxResultIter.init(data);
    while (true) {
        const r_opt = it.next();
        if (r_opt == null) break;
        const r = r_opt ?? break;
        sum.total += 1;
        match (r.outcome) {
            .passed => {
                sum.executed += 1;
                sum.passed += 1;
            },
            .failed => {
                sum.executed += 1;
                sum.failed += 1;
            },
            .error_outcome => {
                sum.executed += 1;
                sum.error_count += 1;
            },
            .timeout => {
                sum.executed += 1;
                sum.timeout += 1;
            },
            .aborted => {
                sum.executed += 1;
                sum.aborted += 1;
            },
            .inconclusive => {
                sum.executed += 1;
                sum.inconclusive += 1;
            },
            .not_executed => {
                sum.not_executed += 1;
            },
            .unknown => {},
        }
    }

    return sum;
}
