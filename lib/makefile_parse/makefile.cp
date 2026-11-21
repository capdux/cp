// Source: Absorbed from /data/dev/bun/src/cli.zig | Synced: 2026-11-21T15:33:58Z (Makefile Target & Variable Parser)

/// GNU Make 变量赋值运算符枚举
pub enum MakeAssignOp: u8 {
    recursive = 0,
    simple = 1,
    conditional = 2,
    append = 3,

    fn asStr(): string {
        return switch (_) {
            .recursive => "=",
            .simple => ":=",
            .conditional => "?=",
            .append => "+=",
        };
    }
}

/// 顶层 Makefile 变量赋值条目（零拷贝切片）
pub struct MakeVar {
    name: []const u8 = "",
    op: MakeAssignOp = .recursive,
    value: []const u8 = "",
    line_no: usize = 0,
}

/// 顶层 Makefile 目标与依赖规则条目（零拷贝切片）
pub struct MakeRule {
    target: []const u8 = "",
    deps_raw: []const u8 = "",
    line_no: usize = 0,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
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

fn stripBom(s: []const u8): []const u8 {
    if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        return s[3..s.len];
    }
    return s;
}

fn stripInlineComment(line: []const u8): []const u8 {
    let i: usize = 0;
    while (i < line.len) {
        if (line[i] == '\\' && i + 1 < line.len && line[i + 1] == '#') {
            i += 2;
            continue;
        }
        if (line[i] == '#') {
            return line[0..i];
        }
        i += 1;
    }
    return line;
}

fn hasHorizSpace(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (isHorizSpace(s[i])) { return true; }
        i += 1;
    }
    return false;
}

fn stripKeywordPrefix(s: []const u8, kw: []const u8): []const u8 {
    if (s.len > kw.len && eqlBytes(s[0..kw.len], kw) && isHorizSpace(s[kw.len])) {
        return trimHoriz(s[kw.len..s.len]);
    }
    return s;
}

fn tryParseVarLine(clean_line: []const u8, line_no: usize): MakeVar? {
    let s = stripKeywordPrefix(clean_line, "export");
    s = stripKeywordPrefix(s, "override");

    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == ':' && i + 2 < s.len && s[i + 1] == ':' && s[i + 2] == '=') {
            const name = trimHoriz(s[0..i]);
            if (name.len == 0 || hasHorizSpace(name)) { return null; }
            return MakeVar{
                name: name,
                op: .simple,
                value: trimHoriz(s[(i + 3)..s.len]),
                line_no: line_no,
            };
        }
        if (c == ':' && i + 1 < s.len && s[i + 1] == '=') {
            const name = trimHoriz(s[0..i]);
            if (name.len == 0 || hasHorizSpace(name)) { return null; }
            return MakeVar{
                name: name,
                op: .simple,
                value: trimHoriz(s[(i + 2)..s.len]),
                line_no: line_no,
            };
        }
        if (c == '?' && i + 1 < s.len && s[i + 1] == '=') {
            const name = trimHoriz(s[0..i]);
            if (name.len == 0 || hasHorizSpace(name)) { return null; }
            return MakeVar{
                name: name,
                op: .conditional,
                value: trimHoriz(s[(i + 2)..s.len]),
                line_no: line_no,
            };
        }
        if (c == '+' && i + 1 < s.len && s[i + 1] == '=') {
            const name = trimHoriz(s[0..i]);
            if (name.len == 0 || hasHorizSpace(name)) { return null; }
            return MakeVar{
                name: name,
                op: .append,
                value: trimHoriz(s[(i + 2)..s.len]),
                line_no: line_no,
            };
        }
        if (c == '=') {
            const name = trimHoriz(s[0..i]);
            if (name.len == 0 || hasHorizSpace(name)) { return null; }
            return MakeVar{
                name: name,
                op: .recursive,
                value: trimHoriz(s[(i + 1)..s.len]),
                line_no: line_no,
            };
        }
        if (c == ':') {
            // 遇到普通规则冒号，非变量赋值行
            return null;
        }
        i += 1;
    }
    return null;
}

fn tryParseRuleLine(clean_line: []const u8, line_no: usize): MakeRule? {
    if (tryParseVarLine(clean_line, line_no) != null) {
        return null;
    }

    let colon_idx: usize? = null;
    let i: usize = 0;
    while (i < clean_line.len) {
        if (clean_line[i] == ':') {
            colon_idx = i;
            break;
        }
        i += 1;
    }

    const ci = colon_idx ?? return null;
    const target = trimHoriz(clean_line[0..ci]);
    if (target.len == 0) { return null; }

    let deps_start = ci + 1;
    if (deps_start < clean_line.len && clean_line[deps_start] == ':') {
        deps_start += 1; // 兼容双冒号规则 target:: deps
    }
    let deps_end = clean_line.len;
    let k = deps_start;
    while (k < clean_line.len) {
        if (clean_line[k] == ';') {
            deps_end = k; // 剥离同行内联 recipe
            break;
        }
        k += 1;
    }

    return MakeRule{
        target: target,
        deps_raw: trimHoriz(clean_line[deps_start..deps_end]),
        line_no: line_no,
    };
}

/// 零堆分配 Makefile 规则迭代器（自动忽略 `\t` recipe 行、空行、`#` 注释与变量赋值行）
pub struct MakeRuleIter {
    src: []const u8,
    pos: usize = 0,
    line_no: usize = 1,

    static fn init(content: []const u8): MakeRuleIter {
        return MakeRuleIter{
            src: stripBom(content),
            pos: 0,
            line_no: 1,
        };
    }

    fn next(*_): MakeRule? {
        while (_.pos < _.src.len) {
            const cur_line = _.line_no;
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') { _.pos += 1; }
            let end = _.pos;
            if (end > start && _.src[end - 1] == '\r') { end -= 1; }
            if (_.pos < _.src.len && _.src[_.pos] == '\n') { _.pos += 1; }
            _.line_no += 1;

            const raw = _.src[start..end];
            if (raw.len == 0 || raw[0] == '\t') {
                continue;
            }
            const clean = trimHoriz(stripInlineComment(raw));
            if (clean.len == 0) { continue; }

            if (rule := tryParseRuleLine(clean, cur_line)) {
                return rule;
            }
        }
        return null;
    }
}

/// 零堆分配 Makefile 顶层变量赋值迭代器（支持 `=` / `:=` / `::=` / `?=` / `+=`）
pub struct MakeVarIter {
    src: []const u8,
    pos: usize = 0,
    line_no: usize = 1,

    static fn init(content: []const u8): MakeVarIter {
        return MakeVarIter{
            src: stripBom(content),
            pos: 0,
            line_no: 1,
        };
    }

    fn next(*_): MakeVar? {
        while (_.pos < _.src.len) {
            const cur_line = _.line_no;
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') { _.pos += 1; }
            let end = _.pos;
            if (end > start && _.src[end - 1] == '\r') { end -= 1; }
            if (_.pos < _.src.len && _.src[_.pos] == '\n') { _.pos += 1; }
            _.line_no += 1;

            const raw = _.src[start..end];
            if (raw.len == 0 || raw[0] == '\t') {
                continue;
            }
            const clean = trimHoriz(stripInlineComment(raw));
            if (clean.len == 0) { continue; }

            if (v := tryParseVarLine(clean, cur_line)) {
                return v;
            }
        }
        return null;
    }
}

/// 扫描全部 `.PHONY: ...` 规则行判定 `target` 是否为伪目标
pub fn isPhonyTarget(content: []const u8, target: []const u8): bool {
    if (target.len == 0) { return false; }
    let it = MakeRuleIter.init(content);
    while (rule := it.next()) {
        if (!eqlBytes(rule.target, ".PHONY")) {
            continue;
        }
        const deps = rule.deps_raw;
        let i: usize = 0;
        while (i < deps.len) {
            while (i < deps.len && isHorizSpace(deps[i])) { i += 1; }
            if (i >= deps.len) { break; }
            const start = i;
            while (i < deps.len && !isHorizSpace(deps[i])) { i += 1; }
            if (eqlBytes(deps[start..i], target)) {
                return true;
            }
        }
    }
    return false;
}
