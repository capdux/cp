// Source: Absorbed from /data/dev/bun/src/css/rules/tailwind.cp | Synced: 2026-11-22T12:14:24Z
// Pure-CP zero-allocation Tailwind CSS `@tailwind` directive parser (base / components /
// utilities / variants), `@apply` utility list parser (arbitrary-value bracket safe),
// cascade-order validator, duplicate-layer detector, canonical serializer, and
// stylesheet extractor with source line/column tracking.

/// 样式表批量提取 `@tailwind` 指令的最大数量
pub const CSS_TAILWIND_MAX_RULES: usize = 32;

/// 单条 `@apply` 指令支持的最大实用类名数量
pub const CSS_TAILWIND_MAX_APPLY_CLASSES: usize = 32;

/// Tailwind CSS 指令解析与序列化错误集
pub err CssTailwindError {
    InvalidAtRule,
    UnknownStyleName,
    UnterminatedDirective,
    UnexpectedTokens,
    EmptyApplyList,
    TooManyRules,
    TooManyApplyClasses,
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

/// `@tailwind` 标准注入层枚举（对齐 Bun `TailwindStyleName`）
pub enum CssTailwindStyleName: u8 {
    Base = 0,
    Components = 1,
    Utilities = 2,
    Variants = 3,

    static fn fromStr(raw_name: []const u8): CssTailwindStyleName? {
        if (eqlAsciiIgnoreCase(raw_name, "base")) return .Base;
        if (eqlAsciiIgnoreCase(raw_name, "components")) return .Components;
        if (eqlAsciiIgnoreCase(raw_name, "utilities")) return .Utilities;
        if (eqlAsciiIgnoreCase(raw_name, "variants")) return .Variants;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Base => "base",
            .Components => "components",
            .Utilities => "utilities",
            .Variants => "variants",
        };
    }

    /// 返回标准级联顺序权重 (`base=0 < components=1 < utilities=2 < variants=3`)
    fn cascadeOrder(): u8 {
        return switch (_) {
            .Base => 0,
            .Components => 1,
            .Utilities => 2,
            .Variants => 3,
        };
    }
}

/// `@tailwind <style_name>;` 指令模型（对齐 Bun `TailwindAtRule`）
pub struct CssTailwindAtRule {
    style_name: CssTailwindStyleName = .Base,
    line: u32 = 1,
    column: u32 = 1,

    static fn of(style_name: CssTailwindStyleName): CssTailwindAtRule {
        return CssTailwindAtRule{
            style_name: style_name,
            line: 1,
            column: 1,
        };
    }

    static fn parseAt(
        input: []const u8,
        start: usize,
        line: u32,
        column: u32,
        out_end: *usize,
    ): CssTailwindAtRule! {
        let pos = skipWsAndComments(input, start);
        if (pos >= input.len || input[pos] != '@') {
            return err.InvalidAtRule;
        }
        pos += 1;
        const kw_start = pos;
        while (pos < input.len && isIdentChar(input[pos])) {
            pos += 1;
        }
        const kw = input[kw_start..pos];
        if (!eqlAsciiIgnoreCase(kw, "tailwind")) {
            return err.InvalidAtRule;
        }

        pos = skipWsAndComments(input, pos);
        if (pos >= input.len) {
            return err.UnterminatedDirective;
        }
        const layer_start = pos;
        while (pos < input.len && isIdentChar(input[pos])) {
            pos += 1;
        }
        const layer_str = input[layer_start..pos];
        if (layer_str.len == 0) {
            return err.UnknownStyleName;
        }
        const layer = CssTailwindStyleName.fromStr(layer_str) ?? return err.UnknownStyleName;

        pos = skipWsAndComments(input, pos);
        if (pos >= input.len) {
            return err.UnterminatedDirective;
        }
        if (input[pos] != ';') {
            return err.UnexpectedTokens;
        }
        out_end.* = pos + 1;
        return CssTailwindAtRule{
            style_name: layer,
            line: line,
            column: column,
        };
    }

    /// 解析单条完整的 `@tailwind <base|components|utilities|variants>;` 指令
    static fn parse(input: []const u8): CssTailwindAtRule! {
        let end_pos: usize = 0;
        const rule = CssTailwindAtRule.parseAt(input, 0, 1, 1, *end_pos)?;
        const tail = skipWsAndComments(input, end_pos);
        if (tail < input.len) {
            return err.UnexpectedTokens;
        }
        return rule;
    }

    fn cascadeOrder(): u8 {
        return _.style_name.cascadeOrder();
    }

    fn eql(other: CssTailwindAtRule): bool {
        return _.style_name == other.style_name;
    }

    /// 规范序列化（对齐 Bun `TailwindAtRule.toCss`：`@tailwind <layer>;`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, "@tailwind ")?;
        * = writeSlice(out, *pos, _.style_name.asStr())?;
        * = writeByte(out, *pos, ';')?;
        return pos;
    }
}

/// Tailwind `@apply <utility-classes...> [!important];` 解析模型（方括号任意值安全）
pub struct CssTailwindApplyRule {
    classes: [CSS_TAILWIND_MAX_APPLY_CLASSES][]const u8 = [""; CSS_TAILWIND_MAX_APPLY_CLASSES],
    class_count: usize = 0,
    important: bool = false,

    static fn parse(input: []const u8): CssTailwindApplyRule! {
        let pos = skipWsAndComments(input, 0);
        if (pos >= input.len || input[pos] != '@') {
            return err.InvalidAtRule;
        }
        pos += 1;
        const kw_start = pos;
        while (pos < input.len && isIdentChar(input[pos])) {
            pos += 1;
        }
        if (!eqlAsciiIgnoreCase(input[kw_start..pos], "apply")) {
            return err.InvalidAtRule;
        }

        let rule = CssTailwindApplyRule{};
        let found_semi = false;
        while (pos < input.len) {
            pos = skipWsAndComments(input, pos);
            if (pos >= input.len) {
                break;
            }
            if (input[pos] == ';') {
                found_semi = true;
                pos += 1;
                break;
            }
            if (input[pos] == '{' || input[pos] == '}') {
                return err.UnexpectedTokens;
            }
            const tok_start = pos;
            let bracket: usize = 0;
            let paren: usize = 0;
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
                if (c == '"' || c == '\'') {
                    in_quote = c;
                } else if (c == '[') {
                    bracket += 1;
                } else if (c == ']') {
                    if (bracket == 0) return err.UnexpectedTokens;
                    bracket -= 1;
                } else if (c == '(') {
                    paren += 1;
                } else if (c == ')') {
                    if (paren == 0) return err.UnexpectedTokens;
                    paren -= 1;
                } else if (bracket == 0 && paren == 0 && (isWs(c) || c == ';' || c == '{' || c == '}')) {
                    break;
                }
                pos += 1;
            }
            if (in_quote != 0 || bracket != 0 || paren != 0) {
                return err.UnexpectedTokens;
            }
            const tok = input[tok_start..pos];
            if (eqlAsciiIgnoreCase(tok, "!important")) {
                rule.important = true;
                continue;
            }
            if (rule.important) {
                // !important 必须位于末尾
                return err.UnexpectedTokens;
            }
            if (rule.class_count >= CSS_TAILWIND_MAX_APPLY_CLASSES) {
                return err.TooManyApplyClasses;
            }
            rule.classes[rule.class_count] = tok;
            rule.class_count += 1;
        }
        if (!found_semi) {
            return err.UnterminatedDirective;
        }
        const tail = skipWsAndComments(input, pos);
        if (tail < input.len) {
            return err.UnexpectedTokens;
        }
        if (rule.class_count == 0) {
            return err.EmptyApplyList;
        }
        return rule;
    }

    fn count(): usize {
        return _.class_count;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.class_count) {
            return null;
        }
        return _.classes[idx];
    }

    fn contains(cls: []const u8): bool {
        let i: usize = 0;
        while (i < _.class_count) {
            if (bytesEql(_.classes[i], cls)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, "@apply")?;
        let i: usize = 0;
        while (i < _.class_count) {
            * = writeByte(out, *pos, ' ')?;
            * = writeSlice(out, *pos, _.classes[i])?;
            i += 1;
        }
        if (_.important) {
            * = writeSlice(out, *pos, " !important")?;
        }
        * = writeByte(out, *pos, ';')?;
        return pos;
    }
}

/// 样式表 `@tailwind` 指令批量提取与级联顺序诊断器
pub struct CssTailwindDirectiveList {
    rules: [CSS_TAILWIND_MAX_RULES]CssTailwindAtRule = [CssTailwindAtRule{}; CSS_TAILWIND_MAX_RULES],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssTailwindAtRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.rules[idx];
    }

    fn hasLayer(layer: CssTailwindStyleName): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.rules[i].style_name == layer) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn hasDuplicateLayers(): bool {
        let seen: u8 = 0;
        let i: usize = 0;
        while (i < _.len) {
            const bit: u8 = @as(u8, 1) << @intCast(_.rules[i].cascadeOrder());
            if ((seen & bit) != 0) {
                return true;
            }
            seen |= bit;
            i += 1;
        }
        return false;
    }

    /// 校验所有提取出的 `@tailwind` 指令是否严格满足 `base < components < utilities < variants` 规范级联顺序
    fn isCanonicalOrder(): bool {
        if (_.len <= 1) {
            return true;
        }
        let i: usize = 1;
        while (i < _.len) {
            if (_.rules[i].cascadeOrder() <= _.rules[i - 1].cascadeOrder()) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    static fn extractFromStylesheet(css: []const u8): CssTailwindDirectiveList! {
        let list = CssTailwindDirectiveList{};
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
                let k = i + 1;
                while (k < css.len && isIdentChar(css[k])) {
                    k += 1;
                }
                const kw = css[i + 1 .. k];
                if (eqlAsciiIgnoreCase(kw, "tailwind")) {
                    let end_pos: usize = 0;
                    const rule = CssTailwindAtRule.parseAt(css, i, line, col, *end_pos)?;
                    if (list.len >= CSS_TAILWIND_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.rules[list.len] = rule;
                    list.len += 1;
                    advanceLineCol(css, i, end_pos, *line, *col);
                    i = end_pos;
                    continue;
                }
            }
            advanceLineCol(css, i, i + 1, *line, *col);
            i += 1;
        }
        return list;
    }
}
