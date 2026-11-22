// Source: Absorbed from /data/dev/bun/src/css/rules/scope.cp | Synced: 2026-11-22T04:08:13Z
// Pure-CP zero-allocation W3C CSS Cascading and Inheritance Level 6 `@scope` rule parser,
// selector-list splitter, stylesheet extractor, and pretty/minified CSS serializer.

/// 单个 `<scope-start>` 或 `<scope-end>` 列表允许的最大选择器数量
pub const CSS_SCOPE_MAX_SELECTORS: usize = 16;

/// 样式表批量提取 `@scope` 规则的最大条目数
pub const CSS_SCOPE_MAX_RULES: usize = 32;

/// CSS `@scope` 规则解析、序列化与提取错误集
pub err CssScopeError {
    InvalidPrelude,
    UnclosedParenthesis,
    UnclosedBlock,
    EmptySelector,
    TooManySelectors,
    UnexpectedToken,
    BufferTooSmall,
    TooManyRules,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c >= 0x80;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

/// 跳过从 `pos` 开始的空白与 `/* ... */` 注释，返回新游标位置
fn skipWsAndComments(s: []const u8, start_pos: usize): usize {
    let i = start_pos;
    while (i < s.len) {
        if (isWs(s[i])) {
            i += 1;
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i = s.len;
            }
            continue;
        }
        break;
    }
    return i;
}

/// 从 `open_pos`（必须是 `open_ch`，如 `'('` 或 `'{'`）出发查找配对闭合字符位置，
/// 正确跳过字符串字面量（单/双引号与转义）和 `/* ... */` 注释。
fn findMatchingDelim(s: []const u8, open_pos: usize, open_ch: u8, close_ch: u8): usize? {
    if (open_pos >= s.len || s[open_pos] != open_ch) {
        return null;
    }
    let depth: usize = 0;
    let i = open_pos;
    let in_quote: u8 = 0;
    while (i < s.len) {
        const c = s[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) {
                in_quote = 0;
            }
            i += 1;
            continue;
        }
        if (c == '/' && i + 1 < s.len && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                return null;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            in_quote = c;
            i += 1;
            continue;
        }
        if (c == open_ch) {
            depth += 1;
        } else if (c == close_ch) {
            if (depth == 0) {
                return null;
            }
            depth -= 1;
            if (depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

/// `@scope` 选择器列表（`<scope-start>` 或 `<scope-end>`，零堆分配固定容量）
pub struct ScopeSelectorList {
    items: [CSS_SCOPE_MAX_SELECTORS][]const u8 = [""; CSS_SCOPE_MAX_SELECTORS],
    len: usize = 0,

    /// 按顶层逗号拆分选择器列表（忽略圆括号、方括号、引号与注释内部的逗号）
    static fn parse(raw: []const u8): ScopeSelectorList! {
        const trimmed = trimWs(raw);
        if (trimmed.len == 0) {
            return err.EmptySelector;
        }
        let list = ScopeSelectorList{};
        let seg_start: usize = 0;
        let i: usize = 0;
        let paren_depth: usize = 0;
        let bracket_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i < trimmed.len) {
            const c = trimmed[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < trimmed.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < trimmed.len && trimmed[i + 1] == '*') {
                i += 2;
                while (i + 1 < trimmed.len && !(trimmed[i] == '*' && trimmed[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < trimmed.len) {
                    i += 2;
                } else {
                    i = trimmed.len;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')') {
                if (paren_depth == 0) {
                    return err.UnexpectedToken;
                }
                paren_depth -= 1;
            } else if (c == '[') {
                bracket_depth += 1;
            } else if (c == ']') {
                if (bracket_depth == 0) {
                    return err.UnexpectedToken;
                }
                bracket_depth -= 1;
            } else if (c == ',' && paren_depth == 0 && bracket_depth == 0) {
                const seg = trimWs(trimmed[seg_start..i]);
                if (seg.len == 0) {
                    return err.EmptySelector;
                }
                if (list.len >= CSS_SCOPE_MAX_SELECTORS) {
                    return err.TooManySelectors;
                }
                list.items[list.len] = seg;
                list.len += 1;
                seg_start = i + 1;
            }
            i += 1;
        }

        if (paren_depth != 0 || bracket_depth != 0 || in_quote != 0) {
            return err.UnclosedParenthesis;
        }

        const tail = trimWs(trimmed[seg_start..trimmed.len]);
        if (tail.len == 0) {
            return err.EmptySelector;
        }
        if (list.len >= CSS_SCOPE_MAX_SELECTORS) {
            return err.TooManySelectors;
        }
        list.items[list.len] = tail;
        list.len += 1;
        return list;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    fn eql(other: ScopeSelectorList): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!bytesEql(_.items[i], other.items[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn writeCss(out: []u8, pos: usize, minify: bool): usize! {
        let cur = pos;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                cur = appendChar(out, cur, ',')?;
                if (!minify) {
                    cur = appendChar(out, cur, ' ')?;
                }
            }
            cur = appendBytes(out, cur, _.items[i])?;
            i += 1;
        }
        return cur;
    }
}

/// CSS Cascading Level 6 `@scope` 规则（对齐 Bun `ScopeRule`）
///
/// 语法：`@scope [(<scope-start>)]? [to (<scope-end>)]? { <stylesheet> }`
pub struct ScopeRule {
    /// 是否声明了 `<scope-start>` 作用域根
    has_start: bool = false,
    /// `<scope-start>` 作用域根选择器列表
    start: ScopeSelectorList = ScopeSelectorList{},
    /// 是否声明了 `<scope-end>` 作用域下限
    has_end: bool = false,
    /// `<scope-end>` 作用域下限选择器列表
    end: ScopeSelectorList = ScopeSelectorList{},
    /// 规则体内部声明或嵌套样式表文本（已裁剪首尾空白）
    body: []const u8 = "",

    /// 解析完整 `@scope [(<scope-start>)]? [to (<scope-end>)]? { <body> }` 规则
    static fn parse(raw: []const u8): ScopeRule! {
        const trimmed = trimWs(raw);
        let pos = skipWsAndComments(trimmed, 0);
        if (pos + 6 > trimmed.len || !eqlAsciiIgnoreCase(trimmed[pos .. pos + 6], "@scope")) {
            return err.InvalidPrelude;
        }
        pos += 6;
        // 防止将 `@scoped` 等非 `@scope` 规则误判为 `@scope`
        if (pos < trimmed.len && isIdentChar(trimmed[pos])) {
            return err.InvalidPrelude;
        }

        let rule = ScopeRule{};
        pos = skipWsAndComments(trimmed, pos);

        // 1. 可选的 `(<scope-start>)`
        if (pos < trimmed.len && trimmed[pos] == '(') {
            if (close_paren := findMatchingDelim(trimmed, pos, '(', ')')) {
                rule.start = ScopeSelectorList.parse(trimmed[pos + 1 .. close_paren])?;
                rule.has_start = true;
                pos = skipWsAndComments(trimmed, close_paren + 1);
            } else {
                return err.UnclosedParenthesis;
            }
        }

        // 2. 可选的 `to (<scope-end>)`
        if (pos + 2 <= trimmed.len && eqlAsciiIgnoreCase(trimmed[pos .. pos + 2], "to")) {
            const after_to = pos + 2;
            if (after_to < trimmed.len && !isIdentChar(trimmed[after_to])) {
                pos = skipWsAndComments(trimmed, after_to);
                if (pos >= trimmed.len || trimmed[pos] != '(') {
                    return err.InvalidPrelude;
                }
                if (close_paren := findMatchingDelim(trimmed, pos, '(', ')')) {
                    rule.end = ScopeSelectorList.parse(trimmed[pos + 1 .. close_paren])?;
                    rule.has_end = true;
                    pos = skipWsAndComments(trimmed, close_paren + 1);
                } else {
                    return err.UnclosedParenthesis;
                }
            }
        }

        // 3. 必需的 `{ <body> }`
        if (pos >= trimmed.len || trimmed[pos] != '{') {
            return err.InvalidPrelude;
        }
        const close_brace = findMatchingDelim(trimmed, pos, '{', '}') ?? return err.UnclosedBlock;
        const trailing = skipWsAndComments(trimmed, close_brace + 1);
        if (trailing != trimmed.len) {
            return err.UnexpectedToken;
        }

        rule.body = trimWs(trimmed[pos + 1 .. close_brace]);
        return rule;
    }

    /// 是否为隐式作用域（未显式声明 `<scope-start>`，以父级 `<style>` 元素为根）
    fn isImplicit(): bool {
        return !_.has_start;
    }

    /// 是否为甜甜圈作用域（同时具备 `<scope-start>` 根和 `<scope-end>` 限制）
    fn isDonut(): bool {
        return _.has_start && _.has_end;
    }

    fn hasStart(): bool {
        return _.has_start;
    }

    fn hasEnd(): bool {
        return _.has_end;
    }

    fn startCount(): usize {
        if (!_.has_start) {
            return 0;
        }
        return _.start.count();
    }

    fn endCount(): usize {
        if (!_.has_end) {
            return 0;
        }
        return _.end.count();
    }

    fn eql(other: ScopeRule): bool {
        if (_.has_start != other.has_start || _.has_end != other.has_end) {
            return false;
        }
        if (_.has_start && !_.start.eql(other.start)) {
            return false;
        }
        if (_.has_end && !_.end.eql(other.end)) {
            return false;
        }
        return bytesEql(_.body, other.body);
    }

    /// 序列化为美化 CSS（对齐 Bun `ScopeRule.toCss` 非压缩模式）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "@scope")?;
        if (_.has_start) {
            pos = appendBytes(out, pos, " (")?;
            pos = _.start.writeCss(out, pos, false)?;
            pos = appendChar(out, pos, ')')?;
        }
        if (_.has_end) {
            pos = appendBytes(out, pos, " to (")?;
            pos = _.end.writeCss(out, pos, false)?;
            pos = appendChar(out, pos, ')')?;
        }
        if (_.body.len == 0) {
            pos = appendBytes(out, pos, " {}")?;
            return pos;
        }
        pos = appendBytes(out, pos, " {\n  ")?;
        pos = appendBytes(out, pos, _.body)?;
        pos = appendBytes(out, pos, "\n}")?;
        return pos;
    }

    /// 序列化为压缩 CSS（对齐 Bun `ScopeRule.toCss` minify 模式，保留 `to (` 前置空格避免词法粘连）
    fn toCssMinified(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "@scope")?;
        if (_.has_start) {
            pos = appendChar(out, pos, '(')?;
            pos = _.start.writeCss(out, pos, true)?;
            pos = appendChar(out, pos, ')')?;
        }
        if (_.has_end) {
            pos = appendBytes(out, pos, " to (")?;
            pos = _.end.writeCss(out, pos, true)?;
            pos = appendChar(out, pos, ')')?;
        }
        pos = appendChar(out, pos, '{')?;
        pos = appendBytes(out, pos, _.body)?;
        pos = appendChar(out, pos, '}')?;
        return pos;
    }
}

/// 样式表 `@scope` 规则批量提取结果容器（固定容量，零堆分配）
pub struct ScopeRuleList {
    rules: [CSS_SCOPE_MAX_RULES]ScopeRule = [ScopeRule{}; CSS_SCOPE_MAX_RULES],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): ScopeRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.rules[idx];
    }

    /// 扫描完整样式表源码并提取所有 `@scope ... { ... }` 规则块（跳过字符串与注释）
    static fn extractFromStylesheet(css_src: []const u8): ScopeRuleList! {
        let out = ScopeRuleList{};
        let i: usize = 0;
        let in_quote: u8 = 0;

        while (i < css_src.len) {
            const c = css_src[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < css_src.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < css_src.len && css_src[i + 1] == '*') {
                i += 2;
                while (i + 1 < css_src.len && !(css_src[i] == '*' && css_src[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < css_src.len) {
                    i += 2;
                } else {
                    break;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }
            if (c == '@' && i + 6 <= css_src.len && eqlAsciiIgnoreCase(css_src[i .. i + 6], "@scope")) {
                const next_idx = i + 6;
                if (next_idx < css_src.len && isIdentChar(css_src[next_idx])) {
                    i += 1;
                    continue;
                }
                // 扫描到匹配的块起始 `{`
                let scan = next_idx;
                let brace_pos: usize = css_src.len;
                let scan_quote: u8 = 0;
                let scan_paren: usize = 0;
                while (scan < css_src.len) {
                    const sc = css_src[scan];
                    if (scan_quote != 0) {
                        if (sc == '\\' && scan + 1 < css_src.len) {
                            scan += 2;
                            continue;
                        }
                        if (sc == scan_quote) {
                            scan_quote = 0;
                        }
                        scan += 1;
                        continue;
                    }
                    if (sc == '/' && scan + 1 < css_src.len && css_src[scan + 1] == '*') {
                        scan += 2;
                        while (scan + 1 < css_src.len && !(css_src[scan] == '*' && css_src[scan + 1] == '/')) {
                            scan += 1;
                        }
                        if (scan + 1 < css_src.len) {
                            scan += 2;
                        } else {
                            break;
                        }
                        continue;
                    }
                    if (sc == '"' || sc == '\'') {
                        scan_quote = sc;
                        scan += 1;
                        continue;
                    }
                    if (sc == '(') {
                        scan_paren += 1;
                    } else if (sc == ')' && scan_paren > 0) {
                        scan_paren -= 1;
                    } else if (sc == '{' && scan_paren == 0) {
                        brace_pos = scan;
                        break;
                    } else if (sc == ';' && scan_paren == 0) {
                        break;
                    }
                    scan += 1;
                }
                if (brace_pos < css_src.len) {
                    const end_brace = findMatchingDelim(css_src, brace_pos, '{', '}') ?? return err.UnclosedBlock;
                    const parsed = ScopeRule.parse(css_src[i .. end_brace + 1])?;
                    if (out.len >= CSS_SCOPE_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    out.rules[out.len] = parsed;
                    out.len += 1;
                    i = end_brace + 1;
                    continue;
                }
            }
            i += 1;
        }
        return out;
    }
}
