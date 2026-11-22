// Source: Absorbed from /data/dev/bun/src/css/rules/unknown.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation W3C CSS Syntax Level 3 generic / unknown `@-rule` parser,
// prelude tokenizer, standard-vs-extension classifier (e.g. `@tailwind`, `@apply`,
// `@theme`, `@position-try`, `@view-transition`, `@-moz-document`), canonical &
// minified serializer, and stylesheet extractor with source line/column tracking.

/// 样式表批量提取未知 `@-rule` 的最大条目数
pub const CSS_UNKNOWN_AT_MAX_RULES: usize = 32;

/// 单条规则 prelude 支持的最大顶层 token 数
pub const CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS: usize = 32;

/// 未知 `@-rule` 解析、校验与序列化错误集
pub err CssUnknownAtRuleError {
    InvalidAtRule,
    UnterminatedStatement,
    UnclosedBlock,
    UnbalancedDelimiters,
    TrailingContent,
    TooManyRules,
    TooManyPreludeTokens,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_' || c == '-' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
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

fn skipWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
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

fn trimWsAndComments(s: []const u8): []const u8 {
    let start = skipWsAndComments(s, 0);
    let end = s.len;
    while (end > start) {
        if (isWs(s[end - 1])) {
            end -= 1;
            continue;
        }
        if (end >= start + 4 && s[end - 2] == '*' && s[end - 1] == '/') {
            let j = end - 2;
            let found = false;
            while (j >= start + 2) {
                if (s[j - 2] == '/' && s[j - 1] == '*') {
                    end = j - 2;
                    found = true;
                    break;
                }
                j -= 1;
            }
            if (found) {
                continue;
            }
        }
        break;
    }
    return s[start..end];
}

fn findMatchingBrace(s: []const u8, open_pos: usize): usize? {
    if (open_pos >= s.len || s[open_pos] != '{') {
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
        if (c == '{') {
            depth += 1;
        } else if (c == '}') {
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

fn writeByte(out: []u8, pos: *usize, b: u8): usize! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

fn writeLowerSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = toLowerAscii(s[i]);
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

fn validateDelimiters(s: []const u8): bool {
    let paren: usize = 0;
    let bracket: usize = 0;
    let brace: usize = 0;
    let in_quote: u8 = 0;
    let i: usize = 0;
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
                return false;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == '(') {
            paren += 1;
        } else if (c == ')') {
            if (paren == 0) return false;
            paren -= 1;
        } else if (c == '[') {
            bracket += 1;
        } else if (c == ']') {
            if (bracket == 0) return false;
            bracket -= 1;
        } else if (c == '{') {
            brace += 1;
        } else if (c == '}') {
            if (brace == 0) return false;
            brace -= 1;
        }
        i += 1;
    }
    return in_quote == 0 && paren == 0 && bracket == 0 && brace == 0;
}

/// 判断给定 at-keyword（不含 `@`）是否属于标准已知 CSS `@-rule`
pub fn isStandardKnownAtRule(rule_name: []const u8): bool {
    return eqlAsciiIgnoreCase(rule_name, "charset") ||
        eqlAsciiIgnoreCase(rule_name, "import") ||
        eqlAsciiIgnoreCase(rule_name, "namespace") ||
        eqlAsciiIgnoreCase(rule_name, "media") ||
        eqlAsciiIgnoreCase(rule_name, "supports") ||
        eqlAsciiIgnoreCase(rule_name, "layer") ||
        eqlAsciiIgnoreCase(rule_name, "scope") ||
        eqlAsciiIgnoreCase(rule_name, "keyframes") ||
        eqlAsciiIgnoreCase(rule_name, "-webkit-keyframes") ||
        eqlAsciiIgnoreCase(rule_name, "-moz-keyframes") ||
        eqlAsciiIgnoreCase(rule_name, "-o-keyframes") ||
        eqlAsciiIgnoreCase(rule_name, "font-face") ||
        eqlAsciiIgnoreCase(rule_name, "font-palette-values") ||
        eqlAsciiIgnoreCase(rule_name, "page") ||
        eqlAsciiIgnoreCase(rule_name, "counter-style") ||
        eqlAsciiIgnoreCase(rule_name, "property") ||
        eqlAsciiIgnoreCase(rule_name, "container") ||
        eqlAsciiIgnoreCase(rule_name, "custom-media") ||
        eqlAsciiIgnoreCase(rule_name, "starting-style") ||
        eqlAsciiIgnoreCase(rule_name, "viewport") ||
        eqlAsciiIgnoreCase(rule_name, "-ms-viewport") ||
        eqlAsciiIgnoreCase(rule_name, "-webkit-viewport") ||
        eqlAsciiIgnoreCase(rule_name, "-o-viewport") ||
        eqlAsciiIgnoreCase(rule_name, "-moz-viewport") ||
        eqlAsciiIgnoreCase(rule_name, "nest");
}

/// 未知 `@-rule` 形态（语句型 `;` vs 块型 `{...}`）
pub enum CssUnknownAtRuleKind: u8 {
    Statement = 0,
    Block = 1,
}

/// Prelude 顶层词法切分结果（保护括号函数与引号字符串不被拆断）
pub struct CssUnknownPreludeTokens {
    items: [CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS][]const u8 = [""; CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    static fn parse(prelude: []const u8): CssUnknownPreludeTokens! {
        let toks = CssUnknownPreludeTokens{};
        let i: usize = 0;
        while (i < prelude.len) {
            i = skipWsAndComments(prelude, i);
            if (i >= prelude.len) {
                break;
            }
            const start = i;
            let paren: usize = 0;
            let bracket: usize = 0;
            let in_quote: u8 = 0;
            while (i < prelude.len) {
                const c = prelude[i];
                if (in_quote != 0) {
                    if (c == '\\' && i + 1 < prelude.len) {
                        i += 2;
                        continue;
                    }
                    if (c == in_quote) {
                        in_quote = 0;
                    }
                    i += 1;
                    continue;
                }
                if (c == '/' && i + 1 < prelude.len && prelude[i + 1] == '*') {
                    if (paren == 0 && bracket == 0) {
                        break;
                    }
                    i += 2;
                    while (i + 1 < prelude.len && !(prelude[i] == '*' && prelude[i + 1] == '/')) {
                        i += 1;
                    }
                    if (i + 1 < prelude.len) {
                        i += 2;
                    } else {
                        return err.UnbalancedDelimiters;
                    }
                    continue;
                }
                if (c == '"' || c == '\'') {
                    in_quote = c;
                    i += 1;
                    continue;
                }
                if (c == '(') {
                    paren += 1;
                    i += 1;
                    continue;
                }
                if (c == ')') {
                    if (paren == 0) {
                        return err.UnbalancedDelimiters;
                    }
                    paren -= 1;
                    i += 1;
                    continue;
                }
                if (c == '[') {
                    bracket += 1;
                    i += 1;
                    continue;
                }
                if (c == ']') {
                    if (bracket == 0) {
                        return err.UnbalancedDelimiters;
                    }
                    bracket -= 1;
                    i += 1;
                    continue;
                }
                if (paren == 0 && bracket == 0 && isWs(c)) {
                    break;
                }
                i += 1;
            }
            if (in_quote != 0 || paren != 0 || bracket != 0) {
                return err.UnbalancedDelimiters;
            }
            if (toks.len >= CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS) {
                return err.TooManyPreludeTokens;
            }
            toks.items[toks.len] = prelude[start..i];
            toks.len += 1;
        }
        return toks;
    }
}

/// 未知 / 扩展 CSS `@-rule` 通用 AST 模型（对齐 Bun `UnknownAtRule`）
pub struct CssUnknownAtRule {
    rule_name: []const u8 = "",
    prelude: []const u8 = "",
    block: ([]const u8)? = null,
    line: u32 = 1,
    column: u32 = 1,

    static fn statement(rule_name: []const u8, prelude: []const u8): CssUnknownAtRule {
        return CssUnknownAtRule{
            rule_name: rule_name,
            prelude: trimWsAndComments(prelude),
            block: null,
            line: 1,
            column: 1,
        };
    }

    static fn withBlock(rule_name: []const u8, prelude: []const u8, block: []const u8): CssUnknownAtRule {
        return CssUnknownAtRule{
            rule_name: rule_name,
            prelude: trimWsAndComments(prelude),
            block: trimWsAndComments(block),
            line: 1,
            column: 1,
        };
    }

    /// 内部解析器：从 `input[start..]` 解析单条 `@-rule` 并通过 `out_end` 返回消费后的末尾偏移
    static fn parseAt(
        input: []const u8,
        start: usize,
        line: u32,
        column: u32,
        out_end: *usize,
    ): CssUnknownAtRule! {
        let pos = skipWsAndComments(input, start);
        if (pos >= input.len || input[pos] != '@') {
            return err.InvalidAtRule;
        }
        pos += 1;
        if (pos >= input.len || !isIdentStart(input[pos])) {
            return err.InvalidAtRule;
        }
        const name_start = pos;
        while (pos < input.len && isIdentChar(input[pos])) {
            pos += 1;
        }
        const kw = input[name_start..pos];
        if (kw.len == 0) {
            return err.InvalidAtRule;
        }

        const prelude_start = pos;
        let paren: usize = 0;
        let bracket: usize = 0;
        let in_quote: u8 = 0;
        while (pos < input.len) {
            const c = input[pos];
            if (in_quote != 0) {
                if (c == '\\' && pos + 1 < input.len) {
                    pos += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                pos += 1;
                continue;
            }
            if (c == '/' && pos + 1 < input.len && input[pos + 1] == '*') {
                pos += 2;
                while (pos + 1 < input.len && !(input[pos] == '*' && input[pos + 1] == '/')) {
                    pos += 1;
                }
                if (pos + 1 < input.len) {
                    pos += 2;
                } else {
                    return err.UnclosedBlock;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                pos += 1;
                continue;
            }
            if (c == '(') {
                paren += 1;
                pos += 1;
                continue;
            }
            if (c == ')') {
                if (paren == 0) {
                    return err.UnbalancedDelimiters;
                }
                paren -= 1;
                pos += 1;
                continue;
            }
            if (c == '[') {
                bracket += 1;
                pos += 1;
                continue;
            }
            if (c == ']') {
                if (bracket == 0) {
                    return err.UnbalancedDelimiters;
                }
                bracket -= 1;
                pos += 1;
                continue;
            }
            if (paren == 0 && bracket == 0) {
                if (c == ';') {
                    const pre = trimWsAndComments(input[prelude_start..pos]);
                    out_end.* = pos + 1;
                    return CssUnknownAtRule{
                        rule_name: kw,
                        prelude: pre,
                        block: null,
                        line: line,
                        column: column,
                    };
                }
                if (c == '{') {
                    const close_pos = findMatchingBrace(input, pos) ?? return err.UnclosedBlock;
                    const body = trimWsAndComments(input[pos + 1 .. close_pos]);
                    if (!validateDelimiters(body)) {
                        return err.UnbalancedDelimiters;
                    }
                    const pre = trimWsAndComments(input[prelude_start..pos]);
                    out_end.* = close_pos + 1;
                    return CssUnknownAtRule{
                        rule_name: kw,
                        prelude: pre,
                        block: body,
                        line: line,
                        column: column,
                    };
                }
                if (c == '}') {
                    return err.UnbalancedDelimiters;
                }
            }
            pos += 1;
        }
        if (in_quote != 0 || paren != 0 || bracket != 0) {
            return err.UnbalancedDelimiters;
        }
        return err.UnterminatedStatement;
    }

    /// 解析单条完整的未知/通用 `@-rule`（不允许规则结束后存在非注释/非空白尾随字符）
    static fn parse(input: []const u8): CssUnknownAtRule! {
        let end_pos: usize = 0;
        const rule = CssUnknownAtRule.parseAt(input, 0, 1, 1, *end_pos)?;
        const tail = skipWsAndComments(input, end_pos);
        if (tail < input.len) {
            return err.TrailingContent;
        }
        return rule;
    }

    fn kind(): CssUnknownAtRuleKind {
        if (_.block != null) {
            return .Block;
        }
        return .Statement;
    }

    fn hasBlock(): bool {
        return _.block != null;
    }

    fn hasPrelude(): bool {
        return _.prelude.len > 0;
    }

    /// 是否为供应商前缀扩展规则（以单个 `-` 开头且非 `--` 自定义前缀）
    fn isVendorPrefixed(): bool {
        return _.rule_name.len >= 2 && _.rule_name[0] == '-' && _.rule_name[1] != '-';
    }

    fn isKnownStandard(): bool {
        return isStandardKnownAtRule(_.rule_name);
    }

    fn preludeTokens(): CssUnknownPreludeTokens! {
        return CssUnknownPreludeTokens.parse(_.prelude);
    }

    fn eql(other: CssUnknownAtRule): bool {
        if (!eqlAsciiIgnoreCase(_.rule_name, other.rule_name)) {
            return false;
        }
        if (!bytesEql(_.prelude, other.prelude)) {
            return false;
        }
        if (b1 := _.block) {
            if (b2 := other.block) {
                return bytesEql(b1, b2);
            }
            return false;
        }
        return other.block == null;
    }

    /// 美化规范序列化（对齐 Bun `UnknownAtRule.toCss`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        * = writeByte(out, *pos, '@')?;
        * = writeLowerSlice(out, *pos, _.rule_name)?;
        if (_.prelude.len > 0) {
            * = writeByte(out, *pos, ' ')?;
            * = writeSlice(out, *pos, _.prelude)?;
        }
        if (blk := _.block) {
            if (blk.len == 0) {
                * = writeSlice(out, *pos, " {}")?;
            } else {
                * = writeSlice(out, *pos, " {\n  ")?;
                * = writeSlice(out, *pos, blk)?;
                * = writeSlice(out, *pos, "\n}")?;
            }
        } else {
            * = writeByte(out, *pos, ';')?;
        }
        return pos;
    }

    /// 压缩序列化
    fn toCssMinified(out: []u8): usize! {
        let pos: usize = 0;
        * = writeByte(out, *pos, '@')?;
        * = writeLowerSlice(out, *pos, _.rule_name)?;
        if (_.prelude.len > 0) {
            * = writeByte(out, *pos, ' ')?;
            * = writeSlice(out, *pos, _.prelude)?;
        }
        if (blk := _.block) {
            * = writeByte(out, *pos, '{')?;
            * = writeSlice(out, *pos, blk)?;
            * = writeByte(out, *pos, '}')?;
        } else {
            * = writeByte(out, *pos, ';')?;
        }
        return pos;
    }
}

fn advanceLineCol(s: []const u8, start_idx: usize, end_idx: usize, line: *u32, col: *u32) {
    let i = start_idx;
    while (i < end_idx && i < s.len) {
        if (s[i] == '\n') {
            line.* += 1;
            col.* = 1;
        } else {
            col.* += 1;
        }
        i += 1;
    }
}

/// 从完整样式表批量提取的未知 / 通用 `@-rule` 集合
pub struct CssUnknownAtRuleList {
    rules: [CSS_UNKNOWN_AT_MAX_RULES]CssUnknownAtRule = [CssUnknownAtRule{}; CSS_UNKNOWN_AT_MAX_RULES],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssUnknownAtRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.rules[idx];
    }

    static fn extractInternal(css: []const u8, only_unknown: bool): CssUnknownAtRuleList! {
        let list = CssUnknownAtRuleList{};
        let i: usize = 0;
        let in_quote: u8 = 0;
        let brace_depth: usize = 0;
        let line: u32 = 1;
        let col: u32 = 1;

        while (i < css.len) {
            const c = css[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < css.len) {
                    advanceLineCol(css, i, i + 2, *line, *col);
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                advanceLineCol(css, i, i + 1, *line, *col);
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < css.len && css[i + 1] == '*') {
                let end = i + 2;
                while (end + 1 < css.len && !(css[end] == '*' && css[end + 1] == '/')) {
                    end += 1;
                }
                if (end + 1 < css.len) {
                    end += 2;
                } else {
                    end = css.len;
                }
                advanceLineCol(css, i, end, *line, *col);
                i = end;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                advanceLineCol(css, i, i + 1, *line, *col);
                i += 1;
                continue;
            }
            if (c == '{') {
                brace_depth += 1;
                advanceLineCol(css, i, i + 1, *line, *col);
                i += 1;
                continue;
            }
            if (c == '}') {
                if (brace_depth > 0) {
                    brace_depth -= 1;
                }
                advanceLineCol(css, i, i + 1, *line, *col);
                i += 1;
                continue;
            }
            if (c == '@' && brace_depth == 0) {
                let end_pos: usize = 0;
                const rule = CssUnknownAtRule.parseAt(css, i, line, col, *end_pos)?;
                if (!only_unknown || !rule.isKnownStandard()) {
                    if (list.len >= CSS_UNKNOWN_AT_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.rules[list.len] = rule;
                    list.len += 1;
                }
                advanceLineCol(css, i, end_pos, *line, *col);
                i = end_pos;
                continue;
            }
            advanceLineCol(css, i, i + 1, *line, *col);
            i += 1;
        }
        return list;
    }

    /// 仅提取顶层未知 / 扩展 `@-rule`（自动跳过已知标准 `@media`/`@layer`/`@import` 等以及字符串与注释）
    static fn extractUnknownFromStylesheet(css: []const u8): CssUnknownAtRuleList! {
        return CssUnknownAtRuleList.extractInternal(css, true);
    }

    /// 提取样式表中全部顶层 `@-rule`（含标准与未知扩展规则）
    static fn extractAllAtRulesFromStylesheet(css: []const u8): CssUnknownAtRuleList! {
        return CssUnknownAtRuleList.extractInternal(css, false);
    }
}
