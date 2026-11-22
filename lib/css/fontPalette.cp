// Source: Absorbed from /data/dev/bun/src/css/rules/font_palette_values.cp | Synced: 2026-11-22T08:17:13Z
// Pure-CP zero-allocation W3C CSS Fonts Level 4 `@font-palette-values` rule
// parser, validator (generic font-family & `currentColor` rejection, non-negative
// palette indices), lookup helper, and canonical/minified serializer.

import { isValidDashedIdent } from "./propRule.cp";

pub const MAX_PALETTE_OVERRIDES: usize = 32;

/// CSS `@font-palette-values` 规则解析、校验与序列化错误集
pub err CssFontPaletteError {
    EmptyInput,
    ExpectedAtFontPaletteValues,
    InvalidPaletteName,
    MissingBlock,
    InvalidDeclaration,
    GenericFontFamilyNotAllowed,
    InvalidBasePalette,
    InvalidOverrideColor,
    TooManyOverrideColors,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0c;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

fn skipWsAndComments(s: string, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isSpace(s[i])) {
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

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

fn appendU16(buf: []u8, pos: usize, val: u16): usize! {
    let tmp: [8]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            n += 1;
        }
    }
    if (pos + n > buf.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) {
        buf[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// 解析非负 `u16` 整数（允许可选 `+` 前缀，拒绝负数、小数与溢出）
fn parseNonNegU16(raw: string): u16? {
    const s = trimSpaces(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: u32 = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + ((s[i] - '0') as u32);
        if (acc > 65535) {
            return null;
        }
        i += 1;
    }
    return acc as u16;
}

/// 判断是否为 CSS Fonts Level 4 泛型字体族关键字（`@font-palette-values` 禁止）
fn isGenericFontFamily(name: string): bool {
    return eqlIgnoreCaseAscii(name, "serif") ||
        eqlIgnoreCaseAscii(name, "sans-serif") ||
        eqlIgnoreCaseAscii(name, "monospace") ||
        eqlIgnoreCaseAscii(name, "cursive") ||
        eqlIgnoreCaseAscii(name, "fantasy") ||
        eqlIgnoreCaseAscii(name, "system-ui") ||
        eqlIgnoreCaseAscii(name, "ui-serif") ||
        eqlIgnoreCaseAscii(name, "ui-sans-serif") ||
        eqlIgnoreCaseAscii(name, "ui-monospace") ||
        eqlIgnoreCaseAscii(name, "ui-rounded") ||
        eqlIgnoreCaseAscii(name, "emoji") ||
        eqlIgnoreCaseAscii(name, "math") ||
        eqlIgnoreCaseAscii(name, "fangsong");
}

fn isValidAbsoluteColor(raw: string): bool {
    const v = trimSpaces(raw);
    if (v.len == 0) {
        return false;
    }
    // W3C CSS Fonts 4 §4.3.3 & Bun font_palette_values.cp: `currentColor` is explicitly forbidden
    if (eqlIgnoreCaseAscii(v, "currentcolor")) {
        return false;
    }
    // Hex color: #rgb / #rgba / #rrggbb / #rrggbbaa
    if (v[0] == '#') {
        const hex_len = v.len - 1;
        if (hex_len != 3 && hex_len != 4 && hex_len != 6 && hex_len != 8) {
            return false;
        }
        let i: usize = 1;
        while (i < v.len) {
            if (!isHexDigit(v[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
    // Functional colors: rgb(...), rgba(...), hsl(...), hsla(...), hwb(...), lab(...), lch(...), oklab(...), oklch(...), color(...)
    if (v.len >= 6 && v[v.len - 1] == ')') {
        if (startsWithIgnoreCaseAscii(v, "rgb(") ||
            startsWithIgnoreCaseAscii(v, "rgba(") ||
            startsWithIgnoreCaseAscii(v, "hsl(") ||
            startsWithIgnoreCaseAscii(v, "hsla(") ||
            startsWithIgnoreCaseAscii(v, "hwb(") ||
            startsWithIgnoreCaseAscii(v, "lab(") ||
            startsWithIgnoreCaseAscii(v, "lch(") ||
            startsWithIgnoreCaseAscii(v, "oklab(") ||
            startsWithIgnoreCaseAscii(v, "oklch(") ||
            startsWithIgnoreCaseAscii(v, "color("))
        {
            return true;
        }
    }
    // Named colors (absolute)
    return eqlIgnoreCaseAscii(v, "transparent") ||
        eqlIgnoreCaseAscii(v, "black") ||
        eqlIgnoreCaseAscii(v, "white") ||
        eqlIgnoreCaseAscii(v, "red") ||
        eqlIgnoreCaseAscii(v, "green") ||
        eqlIgnoreCaseAscii(v, "blue") ||
        eqlIgnoreCaseAscii(v, "yellow") ||
        eqlIgnoreCaseAscii(v, "cyan") ||
        eqlIgnoreCaseAscii(v, "magenta") ||
        eqlIgnoreCaseAscii(v, "orange") ||
        eqlIgnoreCaseAscii(v, "purple") ||
        eqlIgnoreCaseAscii(v, "pink") ||
        eqlIgnoreCaseAscii(v, "gray") ||
        eqlIgnoreCaseAscii(v, "grey") ||
        eqlIgnoreCaseAscii(v, "silver") ||
        eqlIgnoreCaseAscii(v, "maroon") ||
        eqlIgnoreCaseAscii(v, "navy") ||
        eqlIgnoreCaseAscii(v, "teal") ||
        eqlIgnoreCaseAscii(v, "olive") ||
        eqlIgnoreCaseAscii(v, "lime") ||
        eqlIgnoreCaseAscii(v, "aqua") ||
        eqlIgnoreCaseAscii(v, "fuchsia") ||
        eqlIgnoreCaseAscii(v, "rebeccapurple") ||
        eqlIgnoreCaseAscii(v, "coral") ||
        eqlIgnoreCaseAscii(v, "gold") ||
        eqlIgnoreCaseAscii(v, "indigo") ||
        eqlIgnoreCaseAscii(v, "violet") ||
        eqlIgnoreCaseAscii(v, "crimson") ||
        eqlIgnoreCaseAscii(v, "tomato") ||
        eqlIgnoreCaseAscii(v, "salmon");
}

/// `base-palette` 描述符类型
pub enum BasePaletteKind: u8 {
    light = 0,
    dark = 1,
    index = 2,
}

/// W3C CSS Fonts Level 4 `base-palette` 描述符值（`light` | `dark` | `<non-negative-integer>`）
pub struct BasePalette {
    kind: BasePaletteKind,
    index: u16 = 0,

    static fn parse(input: string): BasePalette! {
        const s = trimSpaces(input);
        if (s.len == 0) {
            return err.InvalidBasePalette;
        }
        if (eqlIgnoreCaseAscii(s, "light")) {
            return BasePalette{ kind: .light, index: 0 };
        }
        if (eqlIgnoreCaseAscii(s, "dark")) {
            return BasePalette{ kind: .dark, index: 0 };
        }
        if (idx := parseNonNegU16(s)) {
            return BasePalette{ kind: .index, index: idx };
        }
        return err.InvalidBasePalette;
    }

    fn eql(other: BasePalette): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == .index) {
            return _.index == other.index;
        }
        return true;
    }

    fn toCss(buf: []u8): string! {
        return switch (_.kind) {
            .light => {
                const pos = appendBytes(buf, 0, "light")?;
                return buf[0..pos];
            },
            .dark => {
                const pos = appendBytes(buf, 0, "dark")?;
                return buf[0..pos];
            },
            .index => {
                const pos = appendU16(buf, 0, _.index)?;
                return buf[0..pos];
            },
        };
    }
}

/// W3C CSS Fonts Level 4 `override-colors` 单条调色板槽位覆写（`<integer [0,∞]> <absolute-color>`）
pub struct PaletteOverrideColor {
    index: u16,
    color: string,

    static fn parse(input: string): PaletteOverrideColor! {
        const s = trimSpaces(input);
        if (s.len == 0) {
            return err.InvalidOverrideColor;
        }
        let split_pos: usize = 0;
        while (split_pos < s.len && !isSpace(s[split_pos])) {
            split_pos += 1;
        }
        if (split_pos == 0 || split_pos >= s.len) {
            return err.InvalidOverrideColor;
        }
        const idx_str = s[0..split_pos];
        const color_str = trimSpaces(s[split_pos..]);
        const idx = parseNonNegU16(idx_str) ?? return err.InvalidOverrideColor;
        if (!isValidAbsoluteColor(color_str)) {
            return err.InvalidOverrideColor;
        }
        return PaletteOverrideColor{
            index: idx,
            color: color_str,
        };
    }

    fn eql(other: PaletteOverrideColor): bool {
        return _.index == other.index && std.mem.eql(u8, _.color, other.color);
    }

    fn toCss(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendU16(buf, pos, _.index)?;
        pos = appendBytes(buf, pos, " ")?;
        pos = appendBytes(buf, pos, _.color)?;
        return buf[0..pos];
    }
}

/// W3C CSS Fonts Level 4 `@font-palette-values` 彩色字体调色板规则
pub struct CssFontPaletteValuesRule {
    /// 调色板名称（`<dashed-ident>`，如 `"--brand-palette"`）
    name: string,
    /// 目标字体族名称（不允许泛型字体族）
    font_family: string? = null,
    /// 基础调色板（`light` / `dark` / 槽位索引）
    base_palette: BasePalette? = null,
    /// 覆写颜色条目固定数组（零堆分配）
    override_colors: [MAX_PALETTE_OVERRIDES]PaletteOverrideColor = undefined,
    override_len: usize = 0,

    /// 解析完整 `@font-palette-values --name { ... }` 规则
    static fn parse(input: string): CssFontPaletteValuesRule! {
        const trimmed = trimSpaces(input);
        if (trimmed.len == 0) {
            return err.EmptyInput;
        }

        let pos = skipWsAndComments(trimmed, 0);
        if (pos >= trimmed.len) {
            return err.EmptyInput;
        }

        const at_kw = "@font-palette-values";
        if (!startsWithIgnoreCaseAscii(trimmed[pos..], at_kw)) {
            return err.ExpectedAtFontPaletteValues;
        }
        pos += at_kw.len;
        if (pos >= trimmed.len || (!isSpace(trimmed[pos]) && !(trimmed[pos] == '/' && pos + 1 < trimmed.len && trimmed[pos + 1] == '*'))) {
            return err.ExpectedAtFontPaletteValues;
        }

        pos = skipWsAndComments(trimmed, pos);
        const name_start = pos;
        while (pos < trimmed.len && !isSpace(trimmed[pos]) && trimmed[pos] != '{' && trimmed[pos] != '/') {
            pos += 1;
        }
        const name = trimmed[name_start..pos];
        if (!isValidDashedIdent(name)) {
            return err.InvalidPaletteName;
        }

        pos = skipWsAndComments(trimmed, pos);
        if (pos >= trimmed.len || trimmed[pos] != '{') {
            return err.MissingBlock;
        }
        const body_start = pos + 1;

        let i = body_start;
        let brace_depth: usize = 1;
        let in_quote: u8 = 0;
        let body_end: usize? = null;
        while (i < trimmed.len) {
            const c = trimmed[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < trimmed.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) { in_quote = 0; }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < trimmed.len && trimmed[i + 1] == '*') {
                i = skipWsAndComments(trimmed, i);
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '{') {
                brace_depth += 1;
            } else if (c == '}') {
                brace_depth -= 1;
                if (brace_depth == 0) {
                    body_end = i;
                    break;
                }
            }
            i += 1;
        }
        const close_pos = body_end ?? return err.MissingBlock;
        if (skipWsAndComments(trimmed, close_pos + 1) != trimmed.len) {
            return err.MissingBlock;
        }

        let rule = CssFontPaletteValuesRule{
            name: name,
            font_family: null,
            base_palette: null,
            override_colors: undefined,
            override_len: 0,
        };

        const body = trimmed[body_start..close_pos];
        let bpos: usize = 0;
        while (bpos < body.len) {
            bpos = skipWsAndComments(body, bpos);
            if (bpos >= body.len) {
                break;
            }
            if (body[bpos] == ';') {
                bpos += 1;
                continue;
            }

            const decl_start = bpos;
            let paren_depth: usize = 0;
            let q: u8 = 0;
            while (bpos < body.len) {
                const ch = body[bpos];
                if (q != 0) {
                    if (ch == '\\' && bpos + 1 < body.len) {
                        bpos += 2;
                        continue;
                    }
                    if (ch == q) { q = 0; }
                    bpos += 1;
                    continue;
                }
                if (ch == '/' && bpos + 1 < body.len && body[bpos + 1] == '*') {
                    bpos = skipWsAndComments(body, bpos);
                    continue;
                }
                if (ch == '"' || ch == '\'') {
                    q = ch;
                } else if (ch == '(') {
                    paren_depth += 1;
                } else if (ch == ')' && paren_depth > 0) {
                    paren_depth -= 1;
                } else if (paren_depth == 0 && ch == ';') {
                    break;
                }
                bpos += 1;
            }
            if (q != 0 || paren_depth != 0) {
                return err.InvalidDeclaration;
            }
            const decl = trimSpaces(body[decl_start..bpos]);
            if (bpos < body.len && body[bpos] == ';') {
                bpos += 1;
            }
            if (decl.len == 0) {
                continue;
            }

            let colon_idx: usize? = null;
            let k: usize = 0;
            while (k < decl.len) {
                if (decl[k] == ':') {
                    colon_idx = k;
                    break;
                }
                k += 1;
            }
            const cpos = colon_idx ?? return err.InvalidDeclaration;
            const desc_name = trimSpaces(decl[0..cpos]);
            const desc_val = trimSpaces(decl[cpos + 1 ..]);
            if (desc_val.len == 0) {
                return err.InvalidDeclaration;
            }

            if (eqlIgnoreCaseAscii(desc_name, "font-family")) {
                // 不带引号的泛型字体族必须拒绝；引号包裹的自定义名称合法
                let unquoted = desc_val;
                let was_quoted = false;
                if (desc_val.len >= 2 && ((desc_val[0] == '"' && desc_val[desc_val.len - 1] == '"') || (desc_val[0] == '\'' && desc_val[desc_val.len - 1] == '\''))) {
                    unquoted = desc_val[1 .. desc_val.len - 1];
                    was_quoted = true;
                }
                if (unquoted.len == 0) {
                    return err.InvalidDeclaration;
                }
                if (!was_quoted && isGenericFontFamily(unquoted)) {
                    return err.GenericFontFamilyNotAllowed;
                }
                rule.font_family = desc_val;
            } else if (eqlIgnoreCaseAscii(desc_name, "base-palette")) {
                rule.base_palette = BasePalette.parse(desc_val)?;
            } else if (eqlIgnoreCaseAscii(desc_name, "override-colors")) {
                // 逗号分隔列表（忽略括号内逗号，如 rgb(10, 20, 30)）；重复声明按 last-wins 重置
                rule.override_len = 0;
                let opos: usize = 0;
                while (opos <= desc_val.len) {
                    const item_start = opos;
                    let depth: usize = 0;
                    while (opos < desc_val.len) {
                        const c = desc_val[opos];
                        if (c == '(') {
                            depth += 1;
                        } else if (c == ')' && depth > 0) {
                            depth -= 1;
                        } else if (depth == 0 && c == ',') {
                            break;
                        }
                        opos += 1;
                    }
                    if (depth != 0) {
                        return err.InvalidOverrideColor;
                    }
                    const item_str = trimSpaces(desc_val[item_start..opos]);
                    const entry = PaletteOverrideColor.parse(item_str)?;
                    if (rule.override_len >= MAX_PALETTE_OVERRIDES) {
                        return err.TooManyOverrideColors;
                    }
                    rule.override_colors[rule.override_len] = entry;
                    rule.override_len += 1;
                    if (opos < desc_val.len && desc_val[opos] == ',') {
                        opos += 1;
                        if (opos == desc_val.len) {
                            return err.InvalidOverrideColor;
                        }
                    } else {
                        break;
                    }
                }
            } else {
                return err.InvalidDeclaration;
            }
        }

        return rule;
    }

    fn overrideItems(): []const PaletteOverrideColor {
        return _.override_colors[0.._.override_len];
    }

    /// 按调色板索引查询覆写颜色（同索引多次出现按规范取最后一次）
    fn findOverride(index: u16): string? {
        let found: string? = null;
        let i: usize = 0;
        while (i < _.override_len) {
            if (_.override_colors[i].index == index) {
                found = _.override_colors[i].color;
            }
            i += 1;
        }
        return found;
    }

    fn eql(other: CssFontPaletteValuesRule): bool {
        if (!std.mem.eql(u8, _.name, other.name)) {
            return false;
        }
        if (a := _.font_family) {
            if (b := other.font_family) {
                if (!std.mem.eql(u8, a, b)) { return false; }
            } else {
                return false;
            }
        } else if (other.font_family != null) {
            return false;
        }
        if (a := _.base_palette) {
            if (b := other.base_palette) {
                if (!a.eql(b)) { return false; }
            } else {
                return false;
            }
        } else if (other.base_palette != null) {
            return false;
        }
        if (_.override_len != other.override_len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.override_len) {
            if (!_.override_colors[i].eql(other.override_colors[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 规范格式化输出（多行缩进）
    fn toCss(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "@font-palette-values ")?;
        pos = appendBytes(buf, pos, _.name)?;
        pos = appendBytes(buf, pos, " {\n")?;
        if (ff := _.font_family) {
            pos = appendBytes(buf, pos, "  font-family: ")?;
            pos = appendBytes(buf, pos, ff)?;
            pos = appendBytes(buf, pos, ";\n")?;
        }
        if (bp := _.base_palette) {
            pos = appendBytes(buf, pos, "  base-palette: ")?;
            const bp_s = bp.toCss(buf[pos..])?;
            pos += bp_s.len;
            pos = appendBytes(buf, pos, ";\n")?;
        }
        if (_.override_len > 0) {
            pos = appendBytes(buf, pos, "  override-colors: ")?;
            let i: usize = 0;
            while (i < _.override_len) {
                if (i > 0) {
                    pos = appendBytes(buf, pos, ", ")?;
                }
                const ov_s = _.override_colors[i].toCss(buf[pos..])?;
                pos += ov_s.len;
                i += 1;
            }
            pos = appendBytes(buf, pos, ";\n")?;
        }
        pos = appendBytes(buf, pos, "}")?;
        return buf[0..pos];
    }

    /// 压缩单行输出（对齐 Bun `FontPaletteValuesRule.toCss` `dest.minify = true`）
    fn toCssMinified(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "@font-palette-values ")?;
        pos = appendBytes(buf, pos, _.name)?;
        pos = appendBytes(buf, pos, "{")?;
        let need_semi = false;
        if (ff := _.font_family) {
            pos = appendBytes(buf, pos, "font-family:")?;
            pos = appendBytes(buf, pos, ff)?;
            need_semi = true;
        }
        if (bp := _.base_palette) {
            if (need_semi) {
                pos = appendBytes(buf, pos, ";")?;
            }
            pos = appendBytes(buf, pos, "base-palette:")?;
            const bp_s = bp.toCss(buf[pos..])?;
            pos += bp_s.len;
            need_semi = true;
        }
        if (_.override_len > 0) {
            if (need_semi) {
                pos = appendBytes(buf, pos, ";")?;
            }
            pos = appendBytes(buf, pos, "override-colors:")?;
            let i: usize = 0;
            while (i < _.override_len) {
                if (i > 0) {
                    pos = appendBytes(buf, pos, ",")?;
                }
                const ov_s = _.override_colors[i].toCss(buf[pos..])?;
                pos += ov_s.len;
                i += 1;
            }
        }
        pos = appendBytes(buf, pos, "}")?;
        return buf[0..pos];
    }
}
