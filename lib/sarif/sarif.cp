// Origin: Pure-CP Stdlib (Zero-Alloc OASIS SARIF 2.1.0 Static Analysis Parser) | Created: 2026-11-22

/// SARIF 2.1.0 告警严重级别（规范缺省为 `warning`）
pub enum SarifLevel: u8 {
    none = 0,
    note = 1,
    warning = 2,
    error_level = 3,
}

/// SARIF 2.1.0 顶层报告头（零拷贝切片）
pub struct SarifHeader {
    version: []const u8,
    schema_uri: []const u8,
    runs_slice: []const u8,
}

/// 单次静态分析运行块（`runs[i]`）零拷贝视图
pub struct SarifRun {
    tool_name: []const u8,
    tool_version: []const u8,
    info_uri: []const u8,
    results_slice: []const u8,

    /// 返回当前运行块中全部告警结果的零分配迭代器
    fn results(): SarifResultIter {
        return SarifResultIter.init(_.results_slice);
    }
}

/// 单条静态分析告警结果（`results[j]`）零拷贝视图
pub struct SarifResult {
    rule_id: []const u8,
    level: SarifLevel,
    message: []const u8,
    artifact_uri: []const u8,
    start_line: u32,
    start_col: u32,
    end_line: u32,
}

/// SARIF 报告单遍统计汇总
pub struct SarifStats {
    run_count: u32,
    total_results: u32,
    error_count: u32,
    warning_count: u32,
    note_count: u32,
    none_count: u32,

    /// 是否包含至少一条 `error` 级别告警
    fn hasErrors(): bool {
        return _.error_count > 0;
    }
}

/// `runs` 数组零分配迭代器
pub struct SarifRunIter {
    arr: []const u8,
    pos: usize,
    started: bool,
    had_error: bool,

    static fn init(runs_slice: []const u8): SarifRunIter {
        return SarifRunIter {
            arr: runs_slice,
            pos: 0,
            started: false,
            had_error: false,
        };
    }

    fn next(*_): SarifRun? {
        const elem = nextArrayElement(_.arr, *_.pos, *_.started, *_.had_error) ?? return null;
        return parseRunObject(elem);
    }
}

/// `results` 数组零分配迭代器
pub struct SarifResultIter {
    arr: []const u8,
    pos: usize,
    started: bool,
    had_error: bool,

    static fn init(results_slice: []const u8): SarifResultIter {
        return SarifResultIter {
            arr: results_slice,
            pos: 0,
            started: false,
            had_error: false,
        };
    }

    fn next(*_): SarifResult? {
        const elem = nextArrayElement(_.arr, *_.pos, *_.started, *_.had_error) ?? return null;
        return parseResultObject(elem);
    }
}

// ---------------------------------------------------------------------------
// 内部零分配 JSON 扫描内核（无递归、防转义引号与深嵌套越界）
// ---------------------------------------------------------------------------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn skipWs(s: []const u8, start: usize): usize {
    let i: usize = start;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    return i;
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

struct JsonStrSpan {
    inner: []const u8,
    next_pos: usize,
}

fn scanJsonString(s: []const u8, start: usize): JsonStrSpan? {
    if (start >= s.len || s[start] != '"') {
        return null;
    }
    let i: usize = start + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            if (i + 1 >= s.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (c == '"') {
            return JsonStrSpan {
                inner: s[start + 1..i],
                next_pos: i + 1,
            };
        }
        i += 1;
    }
    return null;
}

fn skipJsonValue(s: []const u8, start: usize): usize? {
    const p = skipWs(s, start);
    if (p >= s.len) {
        return null;
    }
    const c = s[p];
    if (c == '"') {
        const sp = scanJsonString(s, p) ?? return null;
        return sp.next_pos;
    }
    if (c == '{' || c == '[') {
        let depth: u32 = 0;
        let i: usize = p;
        while (i < s.len) {
            const ch = s[i];
            if (ch == '"') {
                const sp = scanJsonString(s, i) ?? return null;
                i = sp.next_pos;
                continue;
            }
            if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                if (depth == 0) {
                    return null;
                }
                depth -= 1;
                if (depth == 0) {
                    return i + 1;
                }
            }
            i += 1;
        }
        return null;
    }
    if (startsWith(s[p..s.len], "true")) {
        return p + 4;
    }
    if (startsWith(s[p..s.len], "false")) {
        return p + 5;
    }
    if (startsWith(s[p..s.len], "null")) {
        return p + 4;
    }
    // JSON 数字
    let i: usize = p;
    if (s[i] == '-') {
        i += 1;
    }
    const d_start = i;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        i += 1;
    }
    if (i == d_start) {
        return null;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            i += 1;
        }
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) {
            i += 1;
        }
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            i += 1;
        }
    }
    return i;
}

fn findObjectField(obj: []const u8, target_key: []const u8): ([]const u8)? {
    let i = skipWs(obj, 0);
    if (i >= obj.len || obj[i] != '{') {
        return null;
    }
    i = skipWs(obj, i + 1);
    if (i < obj.len && obj[i] == '}') {
        return null;
    }

    while (i < obj.len) {
        const key_sp = scanJsonString(obj, i) ?? return null;
        i = skipWs(obj, key_sp.next_pos);
        if (i >= obj.len || obj[i] != ':') {
            return null;
        }
        const val_start = skipWs(obj, i + 1);
        const val_end = skipJsonValue(obj, val_start) ?? return null;

        if (bytesEq(key_sp.inner, target_key)) {
            return obj[val_start..val_end];
        }

        i = skipWs(obj, val_end);
        if (i >= obj.len) {
            return null;
        }
        if (obj[i] == ',') {
            i = skipWs(obj, i + 1);
            continue;
        }
        if (obj[i] == '}') {
            return null;
        }
        return null;
    }
    return null;
}

fn nextArrayElement(arr: []const u8, pos: *usize, started: *bool, had_error: *bool): ([]const u8)? {
    if (arr.len == 0) {
        return null;
    }
    if (!started.*) {
        const p = skipWs(arr, 0);
        if (p >= arr.len || arr[p] != '[') {
            had_error.* = true;
            pos.* = arr.len;
            return null;
        }
        started.* = true;
        pos.* = skipWs(arr, p + 1);
    }

    const p = skipWs(arr, pos.*);
    if (p >= arr.len) {
        had_error.* = true;
        return null;
    }
    if (arr[p] == ']') {
        pos.* = arr.len;
        return null;
    }

    const elem_end_opt = skipJsonValue(arr, p);
    if (elem_end_opt == null) {
        had_error.* = true;
        pos.* = arr.len;
        return null;
    }
    const elem_end = elem_end_opt ?? return null;
    const elem = arr[p..elem_end];

    const after = skipWs(arr, elem_end);
    if (after >= arr.len) {
        had_error.* = true;
        pos.* = arr.len;
        return null;
    }
    if (arr[after] == ',') {
        pos.* = skipWs(arr, after + 1);
    } else if (arr[after] == ']') {
        pos.* = after;
    } else {
        had_error.* = true;
        pos.* = arr.len;
        return null;
    }

    return elem;
}

fn extractStringField(obj: []const u8, key: []const u8): []const u8 {
    if (val := findObjectField(obj, key)) {
        if (sp := scanJsonString(val, 0)) {
            return sp.inner;
        }
    }
    return obj[0..0];
}

fn extractU32Field(obj: []const u8, key: []const u8): u32 {
    if (val := findObjectField(obj, key)) {
        if (val.len == 0) return 0;
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < val.len && val[i] >= '0' && val[i] <= '9') {
            acc = acc * 10 + ((val[i] - '0') as u64);
            if (acc > 4294967295) return 0;
            i += 1;
        }
        if (i == val.len && i > 0) {
            return acc as u32;
        }
    }
    return 0;
}

fn parseSarifLevelStr(s: []const u8): SarifLevel {
    if (bytesEq(s, "error")) return .error_level;
    if (bytesEq(s, "warning")) return .warning;
    if (bytesEq(s, "note")) return .note;
    if (bytesEq(s, "none")) return .none;
    return .warning;
}

fn parseRunObject(obj: []const u8): SarifRun? {
    const p = skipWs(obj, 0);
    if (p >= obj.len || obj[p] != '{') {
        return null;
    }

    const empty = obj[0..0];
    const tool_opt = findObjectField(obj, "tool");
    const driver_opt = if (tool_opt != null) findObjectField(tool_opt ?? empty, "driver") else null;
    const driver = driver_opt ?? empty;

    const tool_name = if (driver_opt != null) extractStringField(driver, "name") else empty;
    const tool_version = if (driver_opt != null) extractStringField(driver, "version") else empty;
    const info_uri = if (driver_opt != null) extractStringField(driver, "informationUri") else empty;

    const res_opt = findObjectField(obj, "results");
    const res_raw = res_opt ?? empty;
    const results_slice = if (res_raw.len >= 2 && res_raw[0] == '[') res_raw else empty;

    return SarifRun {
        tool_name: tool_name,
        tool_version: tool_version,
        info_uri: info_uri,
        results_slice: results_slice,
    };
}

fn parseResultObject(obj: []const u8): SarifResult? {
    const p = skipWs(obj, 0);
    if (p >= obj.len || obj[p] != '{') {
        return null;
    }

    const empty = obj[0..0];
    const rule_id = extractStringField(obj, "ruleId");
    const level_str = extractStringField(obj, "level");
    const level: SarifLevel = if (level_str.len == 0) .warning else parseSarifLevelStr(level_str);

    const msg_opt = findObjectField(obj, "message");
    const message = if (msg_opt != null) extractStringField(msg_opt ?? empty, "text") else empty;

    const locs_opt = findObjectField(obj, "locations");
    let lp: usize = 0;
    let lstarted: bool = false;
    let lerr: bool = false;
    const loc0_opt = if (locs_opt != null) nextArrayElement(locs_opt ?? empty, *lp, *lstarted, *lerr) else null;
    const phys_opt = if (loc0_opt != null) findObjectField(loc0_opt ?? empty, "physicalLocation") else null;
    const phys = phys_opt ?? empty;
    const art_opt = if (phys_opt != null) findObjectField(phys, "artifactLocation") else null;
    const reg_opt = if (phys_opt != null) findObjectField(phys, "region") else null;
    const reg = reg_opt ?? empty;

    const artifact_uri = if (art_opt != null) extractStringField(art_opt ?? empty, "uri") else empty;
    const start_line: u32 = if (reg_opt != null) extractU32Field(reg, "startLine") else 0;
    const start_col: u32 = if (reg_opt != null) extractU32Field(reg, "startColumn") else 0;
    const end_line: u32 = if (reg_opt != null) extractU32Field(reg, "endLine") else 0;

    return SarifResult {
        rule_id: rule_id,
        level: level,
        message: message,
        artifact_uri: artifact_uri,
        start_line: start_line,
        start_col: start_col,
        end_line: end_line,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 嗅探字节流是否为合法 SARIF 2.x JSON 报告（顶层 JSON 对象完整且含 `"runs"` 数组或 `"version": "2..."`）
pub fn isSarif(data: []const u8): bool {
    const p = skipWs(data, 0);
    if (p >= data.len || data[p] != '{') {
        return false;
    }
    const end_opt = skipJsonValue(data, p);
    if (end_opt == null) {
        return false;
    }
    const root = data[p..end_opt ?? return false];
    if (runs := findObjectField(root, "runs")) {
        if (runs.len >= 2 && runs[0] == '[') {
            return true;
        }
    }
    const ver = extractStringField(root, "version");
    return startsWith(ver, "2.");
}

/// 零分配解析 SARIF 2.1.0 报告顶层头（提取 `version`、`$schema` 与 `runs` 数组切片）
pub fn parseSarifHeader(data: []const u8): SarifHeader? {
    const p = skipWs(data, 0);
    if (p >= data.len || data[p] != '{') {
        return null;
    }
    const end_pos = skipJsonValue(data, p) ?? return null;
    const root = data[p..end_pos];

    const runs = findObjectField(root, "runs") ?? return null;
    if (runs.len < 2 || runs[0] != '[') {
        return null;
    }

    return SarifHeader {
        version: extractStringField(root, "version"),
        schema_uri: extractStringField(root, "$schema"),
        runs_slice: runs,
    };
}

/// 单遍零分配统计 SARIF 报告的全部运行块与按级别分类的告警数量
pub fn summarizeSarif(data: []const u8): SarifStats {
    let stats = SarifStats {
        run_count: 0,
        total_results: 0,
        error_count: 0,
        warning_count: 0,
        note_count: 0,
        none_count: 0,
    };

    const hdr = parseSarifHeader(data) ?? return stats;
    let run_it = SarifRunIter.init(hdr.runs_slice);
    while (true) {
        const run_opt = run_it.next();
        if (run_opt == null) {
            break;
        }
        const run = run_opt ?? break;
        stats.run_count += 1;

        let res_it = run.results();
        while (true) {
            const res_opt = res_it.next();
            if (res_opt == null) {
                break;
            }
            const res = res_opt ?? break;
            stats.total_results += 1;
            match (res.level) {
                .error_level => { stats.error_count += 1; },
                .warning => { stats.warning_count += 1; },
                .note => { stats.note_count += 1; },
                .none => { stats.none_count += 1; },
            }
        }
    }

    return stats;
}
