// Source: Absorbed from /data/dev/bun/src/css/rules/starting_style.cp | Synced: 2026-11-22T07:50:00Z
// Pure-CP zero-allocation W3C CSS Transitions Level 2 (`@starting-style`) rule parser,
// nested declaration/rule inspector, canonical & minified serializer, parent-selector
// nesting lifter (flatten nested `@starting-style` to top-level), and stylesheet extractor.

/// 单条 `@starting-style` 规则内支持的最大子条目（声明/子规则）数
pub const CSS_STARTING_STYLE_MAX_ENTRIES: usize = 32;

/// 样式表批量提取 `@starting-style` 规则的最大条目数
pub const CSS_STARTING_STYLE_MAX_RULES: usize = 32;

/// CSS `@starting-style` 规则解析、校验、展平与序列化错误集
pub err CssStartingStyleError {
    InvalidPrelude,
    UnclosedBlock,
    ForbiddenNestedRule,
    InvalidDeclaration,
    EmptySelector,
    TooManyEntries,
    TooManyRules,
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

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c >= 0x80;
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

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn isForbiddenNestedAtKeyword(kw: []const u8): bool {
    return eqlAsciiIgnoreCase(kw, "import") ||
        eqlAsciiIgnoreCase(kw, "charset") ||
        eqlAsciiIgnoreCase(kw, "namespace");
}

fn writeSubstitutedSelector(out: []u8, start: usize, parent: []const u8, child: []const u8): usize! {
    let pos = start;
    let has_amp = false;
    let i: usize = 0;
    while (i < child.len) {
        if (child[i] == '&') {
            has_amp = true;
            pos = writeBytes(out, pos, parent)?;
        } else {
            pos = writeByte(out, pos, child[i])?;
        }
        i += 1;
    }
    if (!has_amp) {
        // 无 '&' 占位符时按 CSS Nesting 后代组合器拼接：`<parent> <child>`
        pos = writeBytes(out, start, parent)?;
        pos = writeByte(out, pos, ' ')?;
        pos = writeBytes(out, pos, child)?;
    }
    return pos;
}

/// `@starting-style` 块内子条目类别
pub enum CssStartingStyleEntryKind: u8 {
    declaration = 0,
    style_rule = 1,
    nested_at_rule = 2,
}

/// `@starting-style` 块内的单条声明或嵌套子规则
pub struct CssStartingStyleEntry {
    kind: CssStartingStyleEntryKind,
    /// `declaration`: 属性名；`style_rule`: 选择器；`nested_at_rule`: `@...` prelude
    head: []const u8,
    /// `declaration`: 属性值（不含 `!important`）；`style_rule`/`nested_at_rule`: 块内正文
    body: []const u8,
    /// 仅 `declaration` 有效，表示末尾带 `!important`
    important: bool,

    /// 空条目
    static fn empty(): CssStartingStyleEntry {
        return CssStartingStyleEntry{
            kind: .declaration,
            head: "",
            body: "",
            important: false,
        };
    }

    /// 语义等价比较
    fn eql(other: CssStartingStyleEntry): bool {
        return _.kind == other.kind &&
            _.important == other.important &&
            bytesEql(_.head, other.head) &&
            bytesEql(_.body, other.body);
    }
}

/// W3C CSS Transitions Level 2 `@starting-style` 规则节点（对应上游 `StartingStyleRule`）
pub struct CssStartingStyleRule {
    entries: [CSS_STARTING_STYLE_MAX_ENTRIES]CssStartingStyleEntry,
    entry_count: usize,
    raw_body: []const u8,
    line: u32,
    column: u32,

    /// 构造空 `@starting-style {}` 规则
    static fn empty(): CssStartingStyleRule {
        return CssStartingStyleRule{
            entries: [CssStartingStyleEntry.empty(); CSS_STARTING_STYLE_MAX_ENTRIES],
            entry_count: 0,
            raw_body: "",
            line: 0,
            column: 0,
        };
    }

    /// 解析单条 `@starting-style { ... }` 规则（严格要求 prelude 为空，拦截非法嵌套 `@import`/`@charset`/`@namespace`）
    static fn parse(input: []const u8): CssStartingStyleRule! {
        let i = skipWsAndComments(input, 0);
        if (i >= input.len || input[i] != '@') {
            return err.InvalidPrelude;
        }
        i += 1;
        const kw_start = i;
        while (i < input.len && isIdentChar(input[i])) {
            i += 1;
        }
        if (!eqlAsciiIgnoreCase(input[kw_start..i], "starting-style")) {
            return err.InvalidPrelude;
        }

        // W3C 规范：`@starting-style` 不接受任何 prelude 参数，关键字后必须直接跟 `{`
        i = skipWsAndComments(input, i);
        if (i >= input.len) {
            return err.UnclosedBlock;
        }
        if (input[i] != '{') {
            return err.InvalidPrelude;
        }
        const close_opt = findMatchingBrace(input, i);
        if (close_pos := close_opt) {
            // 闭合 `}` 之后只允许空白或注释
            if (skipWsAndComments(input, close_pos + 1) != input.len) {
                return err.InvalidPrelude;
            }
            const body = input[i + 1..close_pos];
            let rule = CssStartingStyleRule.empty();
            rule.raw_body = trimWs(body);
            parseStartingStyleBody(&rule, body)?;
            return rule;
        }
        return err.UnclosedBlock;
    }

    /// 块内是否无任何声明或子规则
    fn isEmpty(): bool {
        return _.entry_count == 0;
    }

    /// 是否包含直接属性声明（用于识别嵌套在父选择器内部的 `@starting-style` 形态）
    fn hasDirectDeclarations(): bool {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].kind == .declaration) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 返回子条目总数
    fn count(): usize {
        return _.entry_count;
    }

    /// 按索引读取子条目
    fn get(idx: usize): CssStartingStyleEntry? {
        if (idx >= _.entry_count) {
            return null;
        }
        return _.entries[idx];
    }

    /// 语义等价比较
    fn eql(other: CssStartingStyleRule): bool {
        if (_.entry_count != other.entry_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.entry_count) {
            if (!_.entries[i].eql(other.entries[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 规范序列化为可读 `@starting-style { ... }`（对应上游 `StartingStyleRule.toCss`）
    fn toCss(out: []u8): usize! {
        let pos = writeBytes(out, 0, "@starting-style {")?;
        if (_.entry_count == 0) {
            pos = writeByte(out, pos, '}')?;
            return pos;
        }
        let i: usize = 0;
        while (i < _.entry_count) {
            const e = _.entries[i];
            pos = writeByte(out, pos, ' ')?;
            if (e.kind == .declaration) {
                pos = writeBytes(out, pos, e.head)?;
                pos = writeBytes(out, pos, ": ")?;
                pos = writeBytes(out, pos, e.body)?;
                if (e.important) {
                    pos = writeBytes(out, pos, " !important")?;
                }
                pos = writeByte(out, pos, ';')?;
            } else {
                pos = writeBytes(out, pos, e.head)?;
                pos = writeBytes(out, pos, " { ")?;
                pos = writeBytes(out, pos, e.body)?;
                pos = writeBytes(out, pos, " }")?;
            }
            i += 1;
        }
        pos = writeBytes(out, pos, " }")?;
        return pos;
    }

    /// 压缩序列化 `@starting-style{...}`（零多余空白）
    fn toCssMinified(out: []u8): usize! {
        let pos = writeBytes(out, 0, "@starting-style{")?;
        let i: usize = 0;
        while (i < _.entry_count) {
            const e = _.entries[i];
            if (e.kind == .declaration) {
                pos = writeBytes(out, pos, e.head)?;
                pos = writeByte(out, pos, ':')?;
                pos = writeBytes(out, pos, e.body)?;
                if (e.important) {
                    pos = writeBytes(out, pos, "!important")?;
                }
                if (i + 1 < _.entry_count) {
                    pos = writeByte(out, pos, ';')?;
                }
            } else {
                pos = writeBytes(out, pos, e.head)?;
                pos = writeByte(out, pos, '{')?;
                pos = writeBytes(out, pos, e.body)?;
                pos = writeByte(out, pos, '}')?;
            }
            i += 1;
        }
        pos = writeByte(out, pos, '}')?;
        return pos;
    }

    /// 将嵌套在 `parent_selector` 内的 `@starting-style` 规则展平提升为顶层 `@starting-style { <parent> { ... } ... }`
    fn liftForParent(parent_selector: []const u8, out: []u8): usize! {
        const parent = trimWs(parent_selector);
        if (parent.len == 0) {
            return err.EmptySelector;
        }

        let pos = writeBytes(out, 0, "@starting-style {")?;
        let has_any = false;

        // 1) 汇总所有直接声明到 `<parent> { ... }`
        if (_.hasDirectDeclarations()) {
            has_any = true;
            pos = writeByte(out, pos, ' ')?;
            pos = writeBytes(out, pos, parent)?;
            pos = writeBytes(out, pos, " {")?;
            let i: usize = 0;
            while (i < _.entry_count) {
                const e = _.entries[i];
                if (e.kind == .declaration) {
                    pos = writeByte(out, pos, ' ')?;
                    pos = writeBytes(out, pos, e.head)?;
                    pos = writeBytes(out, pos, ": ")?;
                    pos = writeBytes(out, pos, e.body)?;
                    if (e.important) {
                        pos = writeBytes(out, pos, " !important")?;
                    }
                    pos = writeByte(out, pos, ';')?;
                }
                i += 1;
            }
            pos = writeBytes(out, pos, " }")?;
        }

        // 2) 展开所有嵌套子规则（含 `&` 替换或后代组合器拼接）
        let i: usize = 0;
        while (i < _.entry_count) {
            const e = _.entries[i];
            if (e.kind == .style_rule) {
                has_any = true;
                pos = writeByte(out, pos, ' ')?;
                pos = writeSubstitutedSelector(out, pos, parent, e.head)?;
                pos = writeBytes(out, pos, " { ")?;
                pos = writeBytes(out, pos, e.body)?;
                pos = writeBytes(out, pos, " }")?;
            } else if (e.kind == .nested_at_rule) {
                has_any = true;
                pos = writeByte(out, pos, ' ')?;
                pos = writeBytes(out, pos, e.head)?;
                pos = writeBytes(out, pos, " { ")?;
                pos = writeBytes(out, pos, parent)?;
                pos = writeBytes(out, pos, " { ")?;
                pos = writeBytes(out, pos, e.body)?;
                pos = writeBytes(out, pos, " } }")?;
            }
            i += 1;
        }

        if (has_any) {
            pos = writeBytes(out, pos, " }")?;
        } else {
            pos = writeByte(out, pos, '}')?;
        }
        return pos;
    }
}

fn parseStartingStyleBody(rule: *CssStartingStyleRule, body: []const u8): usize! {
    let i: usize = 0;
    while (i < body.len) {
        i = skipWsAndComments(body, i);
        if (i >= body.len) {
            break;
        }
        if (body[i] == ';') {
            i += 1;
            continue;
        }

        const item_start = i;
        let in_quote: u8 = 0;
        let paren_depth: usize = 0;
        let delim: u8 = 0;
        let delim_pos: usize = body.len;

        while (i < body.len) {
            const c = body[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < body.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < body.len && body[i + 1] == '*') {
                i += 2;
                while (i + 1 < body.len && !(body[i] == '*' && body[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < body.len) {
                    i += 2;
                } else {
                    return err.UnclosedBlock;
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
                if (paren_depth > 0) {
                    paren_depth -= 1;
                }
            } else if (paren_depth == 0 && (c == ';' || c == '{')) {
                delim = c;
                delim_pos = i;
                break;
            }
            i += 1;
        }
        if (in_quote != 0) {
            return err.UnclosedBlock;
        }

        if (delim == '{') {
            const head = trimWs(body[item_start..delim_pos]);
            const close_opt = findMatchingBrace(body, delim_pos);
            if (close_pos := close_opt) {
                const inner = trimWs(body[delim_pos + 1..close_pos]);
                if (head.len == 0) {
                    return err.InvalidPrelude;
                }
                if (rule.entry_count >= CSS_STARTING_STYLE_MAX_ENTRIES) {
                    return err.TooManyEntries;
                }
                if (head[0] == '@') {
                    let k: usize = 1;
                    while (k < head.len && isIdentChar(head[k])) {
                        k += 1;
                    }
                    if (isForbiddenNestedAtKeyword(head[1..k])) {
                        return err.ForbiddenNestedRule;
                    }
                    rule.entries[rule.entry_count] = CssStartingStyleEntry{
                        kind: .nested_at_rule,
                        head: head,
                        body: inner,
                        important: false,
                    };
                } else {
                    rule.entries[rule.entry_count] = CssStartingStyleEntry{
                        kind: .style_rule,
                        head: head,
                        body: inner,
                        important: false,
                    };
                }
                rule.entry_count += 1;
                i = close_pos + 1;
                continue;
            }
            return err.UnclosedBlock;
        }

        // 语句形态：检查是否为非法 `@import` / `@charset` / `@namespace` 或普通属性声明
        const stmt = trimWs(body[item_start..delim_pos]);
        if (delim == ';') {
            i = delim_pos + 1;
        }
        if (stmt.len == 0) {
            continue;
        }
        if (stmt[0] == '@') {
            let k: usize = 1;
            while (k < stmt.len && isIdentChar(stmt[k])) {
                k += 1;
            }
            if (isForbiddenNestedAtKeyword(stmt[1..k])) {
                return err.ForbiddenNestedRule;
            }
            return err.InvalidDeclaration;
        }

        // 查找冒号拆分 `prop: value`
        let colon_pos: usize = stmt.len;
        let scan: usize = 0;
        while (scan < stmt.len) {
            if (stmt[scan] == ':') {
                colon_pos = scan;
                break;
            }
            scan += 1;
        }
        if (colon_pos == stmt.len) {
            return err.InvalidDeclaration;
        }
        const prop = trimWs(stmt[0..colon_pos]);
        let val = trimWs(stmt[colon_pos + 1..]);
        if (prop.len == 0 || val.len == 0) {
            return err.InvalidDeclaration;
        }
        let important = false;
        if (val.len >= 10 && eqlAsciiIgnoreCase(val[val.len - 10..], "!important")) {
            important = true;
            val = trimWs(val[0..val.len - 10]);
            if (val.len == 0) {
                return err.InvalidDeclaration;
            }
        }
        if (rule.entry_count >= CSS_STARTING_STYLE_MAX_ENTRIES) {
            return err.TooManyEntries;
        }
        rule.entries[rule.entry_count] = CssStartingStyleEntry{
            kind: .declaration,
            head: prop,
            body: val,
            important: important,
        };
        rule.entry_count += 1;
    }
    return rule.entry_count;
}

/// 样式表顶层 `@starting-style` 规则批量提取列表
pub struct CssStartingStyleRuleList {
    items: [CSS_STARTING_STYLE_MAX_RULES]CssStartingStyleRule,
    len: usize,

    /// 扫描完整样式表，提取所有顶层 `@starting-style` 规则并记录 1-based 行列号
    static fn extractFromStylesheet(css: []const u8): CssStartingStyleRuleList! {
        let list = CssStartingStyleRuleList{
            items: [CssStartingStyleRule.empty(); CSS_STARTING_STYLE_MAX_RULES],
            len: 0,
        };

        let i: usize = 0;
        let line: u32 = 1;
        let col: u32 = 1;

        while (i < css.len) {
            // 跳过空白与注释
            while (i < css.len) {
                if (isWs(css[i])) {
                    if (css[i] == '\n') {
                        line += 1;
                        col = 1;
                    } else {
                        col += 1;
                    }
                    i += 1;
                    continue;
                }
                if (i + 1 < css.len && css[i] == '/' && css[i + 1] == '*') {
                    i += 2;
                    col += 2;
                    while (i + 1 < css.len && !(css[i] == '*' && css[i + 1] == '/')) {
                        if (css[i] == '\n') {
                            line += 1;
                            col = 1;
                        } else {
                            col += 1;
                        }
                        i += 1;
                    }
                    if (i + 1 < css.len) {
                        i += 2;
                        col += 2;
                    } else {
                        i = css.len;
                    }
                    continue;
                }
                break;
            }
            if (i >= css.len) {
                break;
            }

            const stmt_start = i;
            const stmt_line = line;
            const stmt_col = col;

            let in_quote: u8 = 0;
            let brace_depth: usize = 0;
            while (i < css.len) {
                const c = css[i];
                if (c == '\n') {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }
                if (in_quote != 0) {
                    if (c == '\\' && i + 1 < css.len) {
                        i += 2;
                        col += 1;
                        continue;
                    }
                    if (c == in_quote) {
                        in_quote = 0;
                    }
                    i += 1;
                    continue;
                }
                if (c == '/' && i + 1 < css.len && css[i + 1] == '*') {
                    i += 2;
                    col += 1;
                    while (i + 1 < css.len && !(css[i] == '*' && css[i + 1] == '/')) {
                        if (css[i] == '\n') {
                            line += 1;
                            col = 1;
                        } else {
                            col += 1;
                        }
                        i += 1;
                    }
                    if (i + 1 < css.len) {
                        i += 2;
                        col += 2;
                    } else {
                        i = css.len;
                    }
                    continue;
                }
                if (c == '"' || c == '\'') {
                    in_quote = c;
                    i += 1;
                    continue;
                }
                if (c == '{') {
                    brace_depth += 1;
                } else if (c == '}') {
                    if (brace_depth > 0) {
                        brace_depth -= 1;
                        if (brace_depth == 0) {
                            i += 1;
                            break;
                        }
                    }
                } else if (c == ';' && brace_depth == 0) {
                    i += 1;
                    break;
                }
                i += 1;
            }

            const slice = css[stmt_start..i];
            if (slice.len > 1 && slice[0] == '@') {
                let k: usize = 1;
                while (k < slice.len && isIdentChar(slice[k])) {
                    k += 1;
                }
                if (eqlAsciiIgnoreCase(slice[1..k], "starting-style")) {
                    let rule = CssStartingStyleRule.parse(slice)?;
                    rule.line = stmt_line;
                    rule.column = stmt_col;
                    if (list.len >= CSS_STARTING_STYLE_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.items[list.len] = rule;
                    list.len += 1;
                }
            }
        }

        return list;
    }

    /// 返回提取到的 `@starting-style` 规则数
    fn count(): usize {
        return _.len;
    }

    /// 按索引读取规则
    fn get(idx: usize): CssStartingStyleRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }
}
