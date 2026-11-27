// Source: Absorbed from /data/dev/bun/src/css/rules/property.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation W3C CSS Properties and Values API Level 1 `@property`
// rule parser, `<syntax>` validator, computationally-independent `initial-value`
// checker, and canonical/minified serializer.

import { Multiplier, SyntaxType, SyntaxComponent } from "./syntax.cp";

pub const MAX_PROPERTY_SYNTAX_COMPONENTS: usize = 16;

/// CSS `@property` 规则解析、校验与序列化错误集
pub err CssPropertyRuleError {
    EmptyInput,
    ExpectedAtProperty,
    InvalidPropertyName,
    MissingBlock,
    InvalidDeclaration,
    MissingSyntax,
    InvalidSyntax,
    MissingInherits,
    InvalidInherits,
    MissingInitialValue,
    InvalidInitialValue,
    TooManySyntaxComponents,
    BufferTooSmall,
}

type CssPropVec16 = @Vector(16, u8);

fn makeCssPropClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\n'] = 1;
    lut['\r'] = 1;
    lut[0x0c] = 1;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] |= 2 | 4 | 16;
        }
        if ((i >= 'a' && i <= 'f') || (i >= 'A' && i <= 'F')) {
            lut[i] |= 4;
        }
        if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || i >= 0x80 || i == '_') {
            lut[i] |= 8 | 16;
        }
        if (i == '-') {
            lut[i] |= 16;
        }
    }
    return lut;
}

fn makeCssPropLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const CSS_PROP_CLASS_LUT: [u8; 256] = makeCssPropClassLut();
const CSS_PROP_LOWER_LUT: [u8; 256] = makeCssPropLowerLut();

fn isSpace(c: u8): bool {
    return (CSS_PROP_CLASS_LUT[@as(usize, c)] & 1) != 0;
}

fn isDigit(c: u8): bool {
    return (CSS_PROP_CLASS_LUT[@as(usize, c)] & 2) != 0;
}

fn isHexDigit(c: u8): bool {
    return (CSS_PROP_CLASS_LUT[@as(usize, c)] & 4) != 0;
}

fn isIdentStart(c: u8): bool {
    return (CSS_PROP_CLASS_LUT[@as(usize, c)] & 8) != 0;
}

fn isNameCodePoint(c: u8): bool {
    return (CSS_PROP_CLASS_LUT[@as(usize, c)] & 16) != 0;
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

fn trimLeftSpaces(s: string): string {
    let i: usize = 0;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return s[i..];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: CssPropVec16 = @splat(65 as u8);
    const hi_Z: CssPropVec16 = @splat(90 as u8);
    const case_bit: CssPropVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: CssPropVec16 = a[i .. i + 16][0..16].*;
        const vb: CssPropVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < a.len) {
        if (CSS_PROP_LOWER_LUT[@as(usize, a[i])] != CSS_PROP_LOWER_LUT[@as(usize, b[i])]) {
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

/// 跳过前导空白与 CSS `/* ... */` 块注释
fn skipWsAndComments(s: string, start: usize): usize {
    let i = start;
    const v_star: CssPropVec16 = @splat('*' as u8);
    while (i < s.len) {
        if (isSpace(s[i])) {
            i += 1;
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 16 < s.len) {
                const v: CssPropVec16 = s[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_star)) break;
                i += 16;
            }
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

/// 校验 CSS `<dashed-ident>`（必须以 `--` 开头，至少 3 字节，且后续均为合法 CSS 名称字符）
pub fn isValidDashedIdent(name: string): bool {
    if (name.len < 3) {
        return false;
    }
    if (name[0] != '-' || name[1] != '-') {
        return false;
    }
    let i: usize = 2;
    while (i < name.len) {
        if (!isNameCodePoint(name[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 检查值是否为 CSS-wide 关键字（`initial` / `inherit` / `unset` / `revert` / `revert-layer`）
fn isCssWideKeyword(v: string): bool {
    return eqlIgnoreCaseAscii(v, "initial") ||
        eqlIgnoreCaseAscii(v, "inherit") ||
        eqlIgnoreCaseAscii(v, "unset") ||
        eqlIgnoreCaseAscii(v, "revert") ||
        eqlIgnoreCaseAscii(v, "revert-layer");
}

/// 检查值中是否包含非引号内的 `var(` 或 `env(`（注册属性 initial-value 必须计算独立）
fn containsVarOrEnvRef(v: string): bool {
    let i: usize = 0;
    let in_quote: u8 = 0;
    while (i < v.len) {
        const c = v[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < v.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) {
                in_quote = 0;
            }
            i += 1;
            continue;
        }
        if (c == '"' || c == '\'') {
            in_quote = c;
            i += 1;
            continue;
        }
        const prev_ok = i == 0 || !isNameCodePoint(v[i - 1]);
        if (prev_ok) {
            if (startsWithIgnoreCaseAscii(v[i..], "var(") || startsWithIgnoreCaseAscii(v[i..], "env(")) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

/// 解析数字前缀，返回消费的字节数（0 表示失败）
fn parseNumberPrefix(s: string, allow_float: bool): usize {
    let i: usize = 0;
    if (i < s.len && (s[i] == '+' || s[i] == '-')) {
        i += 1;
    }
    let int_digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        int_digits += 1;
        i += 1;
    }
    let frac_digits: usize = 0;
    if (allow_float && i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_digits += 1;
            i += 1;
        }
        if (frac_digits == 0) {
            return 0;
        }
    }
    if (int_digits == 0 && frac_digits == 0) {
        return 0;
    }
    if (allow_float && i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        const exp_start = i;
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) {
            i += 1;
        }
        let exp_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0) {
            return exp_start;
        }
    }
    return i;
}

fn isZeroNumber(num_str: string): bool {
    let i: usize = 0;
    if (i < num_str.len && (num_str[i] == '+' || num_str[i] == '-')) {
        i += 1;
    }
    let saw_digit = false;
    while (i < num_str.len) {
        const c = num_str[i];
        if (c == '0') {
            saw_digit = true;
        } else if (c == '.') {
            // decimal point allowed
        } else if (c == 'e' || c == 'E') {
            break;
        } else {
            return false;
        }
        i += 1;
    }
    return saw_digit;
}

fn isAbsoluteLengthUnit(u: string): bool {
    return eqlIgnoreCaseAscii(u, "px") ||
        eqlIgnoreCaseAscii(u, "cm") ||
        eqlIgnoreCaseAscii(u, "mm") ||
        eqlIgnoreCaseAscii(u, "q") ||
        eqlIgnoreCaseAscii(u, "in") ||
        eqlIgnoreCaseAscii(u, "pc") ||
        eqlIgnoreCaseAscii(u, "pt");
}

fn isAngleUnit(u: string): bool {
    return eqlIgnoreCaseAscii(u, "deg") ||
        eqlIgnoreCaseAscii(u, "grad") ||
        eqlIgnoreCaseAscii(u, "rad") ||
        eqlIgnoreCaseAscii(u, "turn");
}

fn isTimeUnit(u: string): bool {
    return eqlIgnoreCaseAscii(u, "s") || eqlIgnoreCaseAscii(u, "ms");
}

fn isResolutionUnit(u: string): bool {
    return eqlIgnoreCaseAscii(u, "dpi") ||
        eqlIgnoreCaseAscii(u, "dpcm") ||
        eqlIgnoreCaseAscii(u, "dppx") ||
        eqlIgnoreCaseAscii(u, "x");
}

fn isHexColor(v: string): bool {
    if (v.len < 4 || v[0] != '#') {
        return false;
    }
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

fn isColorFunction(v: string): bool {
    if (v.len < 6 || v[v.len - 1] != ')') {
        return false;
    }
    return startsWithIgnoreCaseAscii(v, "rgb(") ||
        startsWithIgnoreCaseAscii(v, "rgba(") ||
        startsWithIgnoreCaseAscii(v, "hsl(") ||
        startsWithIgnoreCaseAscii(v, "hsla(") ||
        startsWithIgnoreCaseAscii(v, "hwb(") ||
        startsWithIgnoreCaseAscii(v, "lab(") ||
        startsWithIgnoreCaseAscii(v, "lch(") ||
        startsWithIgnoreCaseAscii(v, "oklab(") ||
        startsWithIgnoreCaseAscii(v, "oklch(") ||
        startsWithIgnoreCaseAscii(v, "color(") ||
        startsWithIgnoreCaseAscii(v, "color-mix(");
}

fn isNamedColor(v: string): bool {
    return eqlIgnoreCaseAscii(v, "transparent") ||
        eqlIgnoreCaseAscii(v, "currentcolor") ||
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

fn isQuotedString(v: string): bool {
    if (v.len < 2) {
        return false;
    }
    const q = v[0];
    if ((q != '"' && q != '\'') || v[v.len - 1] != q) {
        return false;
    }
    let i: usize = 1;
    while (i + 1 < v.len) {
        if (v[i] == '\\') {
            if (i + 2 >= v.len) {
                return false;
            }
            i += 2;
            continue;
        }
        if (v[i] == q || v[i] == '\n' || v[i] == '\r') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isUrlValue(v: string): bool {
    if (v.len < 5 || v[v.len - 1] != ')') {
        return false;
    }
    return startsWithIgnoreCaseAscii(v, "url(");
}

fn isImageValue(v: string): bool {
    if (isUrlValue(v)) {
        return true;
    }
    if (v.len < 10 || v[v.len - 1] != ')') {
        return false;
    }
    return startsWithIgnoreCaseAscii(v, "linear-gradient(") ||
        startsWithIgnoreCaseAscii(v, "radial-gradient(") ||
        startsWithIgnoreCaseAscii(v, "conic-gradient(") ||
        startsWithIgnoreCaseAscii(v, "repeating-linear-gradient(") ||
        startsWithIgnoreCaseAscii(v, "repeating-radial-gradient(") ||
        startsWithIgnoreCaseAscii(v, "repeating-conic-gradient(") ||
        startsWithIgnoreCaseAscii(v, "image-set(");
}

fn isTransformFunction(v: string): bool {
    if (v.len < 7 || v[v.len - 1] != ')') {
        return false;
    }
    return startsWithIgnoreCaseAscii(v, "translate(") ||
        startsWithIgnoreCaseAscii(v, "translatex(") ||
        startsWithIgnoreCaseAscii(v, "translatey(") ||
        startsWithIgnoreCaseAscii(v, "translatez(") ||
        startsWithIgnoreCaseAscii(v, "translate3d(") ||
        startsWithIgnoreCaseAscii(v, "scale(") ||
        startsWithIgnoreCaseAscii(v, "scalex(") ||
        startsWithIgnoreCaseAscii(v, "scaley(") ||
        startsWithIgnoreCaseAscii(v, "scalez(") ||
        startsWithIgnoreCaseAscii(v, "scale3d(") ||
        startsWithIgnoreCaseAscii(v, "rotate(") ||
        startsWithIgnoreCaseAscii(v, "rotatex(") ||
        startsWithIgnoreCaseAscii(v, "rotatey(") ||
        startsWithIgnoreCaseAscii(v, "rotatez(") ||
        startsWithIgnoreCaseAscii(v, "rotate3d(") ||
        startsWithIgnoreCaseAscii(v, "skew(") ||
        startsWithIgnoreCaseAscii(v, "skewx(") ||
        startsWithIgnoreCaseAscii(v, "skewy(") ||
        startsWithIgnoreCaseAscii(v, "matrix(") ||
        startsWithIgnoreCaseAscii(v, "matrix3d(") ||
        startsWithIgnoreCaseAscii(v, "perspective(");
}

fn isValidCustomIdent(v: string): bool {
    if (v.len == 0 || isCssWideKeyword(v) || eqlIgnoreCaseAscii(v, "default")) {
        return false;
    }
    if (v.len >= 2 && v[0] == '-' && v[1] == '-') {
        return false;
    }
    if (!isIdentStart(v[0]) && !(v[0] == '-' && v.len > 1 && isIdentStart(v[1]))) {
        return false;
    }
    let i: usize = 1;
    while (i < v.len) {
        if (!isNameCodePoint(v[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 校验单个非列表原子值是否符合给定的 `SyntaxComponent` 类型
fn matchesSingleComponent(comp: SyntaxComponent, raw_val: string): bool {
    const v = trimSpaces(raw_val);
    if (v.len == 0) {
        return false;
    }
    return switch (comp.kind) {
        .integer => {
            const n = parseNumberPrefix(v, false);
            return n > 0 && n == v.len;
        },
        .number => {
            const n = parseNumberPrefix(v, true);
            return n > 0 && n == v.len;
        },
        .percentage => {
            const n = parseNumberPrefix(v, true);
            return n > 0 && n + 1 == v.len && v[n] == '%';
        },
        .length => {
            const n = parseNumberPrefix(v, true);
            if (n == 0) { return false; }
            if (n == v.len) { return isZeroNumber(v); }
            return isAbsoluteLengthUnit(v[n..]);
        },
        .length_percentage => {
            const n = parseNumberPrefix(v, true);
            if (n == 0) { return false; }
            if (n == v.len) { return isZeroNumber(v); }
            const unit = v[n..];
            return (unit.len == 1 && unit[0] == '%') || isAbsoluteLengthUnit(unit);
        },
        .angle => {
            const n = parseNumberPrefix(v, true);
            if (n == 0) { return false; }
            if (n == v.len) { return isZeroNumber(v); }
            return isAngleUnit(v[n..]);
        },
        .time => {
            const n = parseNumberPrefix(v, true);
            if (n == 0) { return false; }
            return n < v.len && isTimeUnit(v[n..]);
        },
        .resolution => {
            const n = parseNumberPrefix(v, true);
            if (n == 0 || v[0] == '-') { return false; }
            return n < v.len && isResolutionUnit(v[n..]);
        },
        .color => isHexColor(v) || isColorFunction(v) || isNamedColor(v),
        .string_type => isQuotedString(v),
        .url => isUrlValue(v),
        .image => isImageValue(v),
        .transform_function => isTransformFunction(v),
        .transform_list => matchesSpaceSeparatedTransforms(v),
        .custom_ident => isValidCustomIdent(v),
        .literal => std.mem.eql(u8, v, comp.literal),
    };
}

/// 顶层空白切分（忽略括号/引号内部空白），用于 `+` 乘数与 `<transform-list>`
fn matchesSpaceSeparated(comp: SyntaxComponent, v: string): bool {
    let i: usize = 0;
    let count: usize = 0;
    while (i < v.len) {
        while (i < v.len && isSpace(v[i])) {
            i += 1;
        }
        if (i >= v.len) {
            break;
        }
        const start = i;
        let depth: usize = 0;
        let in_quote: u8 = 0;
        while (i < v.len) {
            const c = v[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < v.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) { in_quote = 0; }
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                depth += 1;
            } else if (c == ')' && depth > 0) {
                depth -= 1;
            } else if (depth == 0 && isSpace(c)) {
                break;
            }
            i += 1;
        }
        if (depth != 0 || in_quote != 0) {
            return false;
        }
        const tok = v[start..i];
        if (!matchesSingleComponent(comp, tok)) {
            return false;
        }
        count += 1;
    }
    return count > 0;
}

fn matchesSpaceSeparatedTransforms(v: string): bool {
    const tf_comp = SyntaxComponent{ kind: .transform_function, literal: "", multiplier: .none };
    return matchesSpaceSeparated(tf_comp, v);
}

/// 顶层逗号切分（忽略括号/引号内部逗号），用于 `#` 乘数
fn matchesCommaSeparated(comp: SyntaxComponent, v: string): bool {
    let i: usize = 0;
    let count: usize = 0;
    while (i <= v.len) {
        const start = i;
        let depth: usize = 0;
        let in_quote: u8 = 0;
        while (i < v.len) {
            const c = v[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < v.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) { in_quote = 0; }
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                depth += 1;
            } else if (c == ')' && depth > 0) {
                depth -= 1;
            } else if (depth == 0 && c == ',') {
                break;
            }
            i += 1;
        }
        if (depth != 0 || in_quote != 0) {
            return false;
        }
        const item = trimSpaces(v[start..i]);
        if (item.len == 0 || !matchesSingleComponent(comp, item)) {
            return false;
        }
        count += 1;
        if (i < v.len && v[i] == ',') {
            i += 1;
            if (i == v.len) {
                return false; // trailing comma invalid
            }
        } else {
            break;
        }
    }
    return count > 0;
}

/// 零堆分配固定容量 CSS `<syntax>` 定义（用于 `@property` 规则）
pub struct PropertySyntax {
    is_universal: bool = false,
    components: [MAX_PROPERTY_SYNTAX_COMPONENTS]SyntaxComponent = undefined,
    len: usize = 0,

    /// 解析 `<syntax>` 内部字符串（不含外层引号），如 `"*"`、`"<color>"`、`"<length>+ | auto"`
    static fn parse(input: string): PropertySyntax! {
        const trimmed = trimSpaces(input);
        if (trimmed.len == 0) {
            return err.InvalidSyntax;
        }

        let res = PropertySyntax{
            is_universal: false,
            components: undefined,
            len: 0,
        };

        if (std.mem.eql(u8, trimmed, "*")) {
            res.is_universal = true;
            return res;
        }

        let rem = trimmed;
        while (true) {
            rem = trimLeftSpaces(rem);
            if (rem.len == 0) {
                return err.InvalidSyntax;
            }

            let kind: SyntaxType = .length;
            let lit: string = "";

            if (rem[0] == '<') {
                let close_idx: usize? = null;
                let j: usize = 1;
                while (j < rem.len) {
                    if (rem[j] == '>') {
                        close_idx = j;
                        break;
                    }
                    j += 1;
                }
                const end_pos = close_idx ?? return err.InvalidSyntax;
                const type_name = rem[1..end_pos];
                if (parsed_kind := SyntaxType.fromTypeName(type_name)) {
                    kind = parsed_kind;
                } else {
                    return err.InvalidSyntax;
                }
                rem = rem[end_pos + 1 ..];
            } else if (isIdentStart(rem[0]) || (rem[0] == '-' && rem.len > 1 && (isIdentStart(rem[1]) || rem[1] == '-'))) {
                let end_pos: usize = 0;
                while (end_pos < rem.len && isNameCodePoint(rem[end_pos])) {
                    end_pos += 1;
                }
                kind = .literal;
                lit = rem[0..end_pos];
                rem = rem[end_pos..];
            } else {
                return err.InvalidSyntax;
            }

            let mult: Multiplier = .none;
            if (rem.len > 0 && (rem[0] == '+' || rem[0] == '#')) {
                if (!kind.allowsMultiplier()) {
                    return err.InvalidSyntax;
                }
                mult = if (rem[0] == '+') .space else .comma;
                rem = rem[1..];
            }

            if (res.len >= MAX_PROPERTY_SYNTAX_COMPONENTS) {
                return err.TooManySyntaxComponents;
            }
            res.components[res.len] = SyntaxComponent{
                kind: kind,
                literal: lit,
                multiplier: mult,
            };
            res.len += 1;

            rem = trimLeftSpaces(rem);
            if (rem.len == 0) {
                break;
            }
            if (rem[0] == '|') {
                rem = rem[1..];
                continue;
            }
            return err.InvalidSyntax;
        }

        return res;
    }

    fn isUniversal(): bool {
        return _.is_universal;
    }

    fn items(): []const SyntaxComponent {
        return _.components[0.._.len];
    }

    fn eql(other: PropertySyntax): bool {
        if (_.is_universal != other.is_universal || _.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.components[i].eql(other.components[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 校验候选 `initial-value` 字符串是否满足该 `<syntax>` 定义
    fn matchesValue(raw_value: string): bool {
        const v = trimSpaces(raw_value);
        if (_.is_universal) {
            return true;
        }
        if (v.len == 0) {
            return false;
        }
        if (isCssWideKeyword(v) || containsVarOrEnvRef(v)) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            const comp = _.components[i];
            const ok = switch (comp.multiplier) {
                .none => matchesSingleComponent(comp, v),
                .space => matchesSpaceSeparated(comp, v),
                .comma => matchesCommaSeparated(comp, v),
            };
            if (ok) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 序列化（不含外层双引号）
    fn toCss(buf: []u8): string! {
        if (_.is_universal) {
            if (buf.len < 1) {
                return err.BufferTooSmall;
            }
            buf[0] = '*';
            return buf[0..1];
        }

        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                if (pos + 3 > buf.len) {
                    return err.BufferTooSmall;
                }
                buf[pos] = ' ';
                buf[pos + 1] = '|';
                buf[pos + 2] = ' ';
                pos += 3;
            }
            const base = if (_.components[i].kind == .literal) _.components[i].literal else _.components[i].kind.asTypeStr();
            const mult = _.components[i].multiplier.asStr();
            const total = base.len + mult.len;
            if (pos + total > buf.len) {
                return err.BufferTooSmall;
            }
            @memcpy(buf[pos .. pos + base.len], base);
            pos += base.len;
            if (mult.len > 0) {
                @memcpy(buf[pos .. pos + mult.len], mult);
                pos += mult.len;
            }
            i += 1;
        }
        return buf[0..pos];
    }

    /// 序列化（带双引号，用于 `syntax: "..."` 描述符）
    fn toCssQuoted(buf: []u8): string! {
        if (buf.len < 2) {
            return err.BufferTooSmall;
        }
        buf[0] = '"';
        const inner = _.toCss(buf[1 .. buf.len - 1])?;
        buf[1 + inner.len] = '"';
        return buf[0 .. inner.len + 2];
    }
}

/// 辅助写入切片到输出缓冲
fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

/// 解析引号字符串（`"..."` 或 `'...'`），返回去除外层引号后的内部切片
fn parseQuotedSyntaxString(raw_val: string): string! {
    const v = trimSpaces(raw_val);
    if (v.len < 2) {
        return err.InvalidSyntax;
    }
    const q = v[0];
    if ((q != '"' && q != '\'') || v[v.len - 1] != q) {
        return err.InvalidSyntax;
    }
    // 检查中间无未转义同名引号
    let i: usize = 1;
    while (i + 1 < v.len) {
        if (v[i] == '\\') {
            if (i + 2 >= v.len) {
                return err.InvalidSyntax;
            }
            i += 2;
            continue;
        }
        if (v[i] == q) {
            return err.InvalidSyntax;
        }
        i += 1;
    }
    return v[1 .. v.len - 1];
}

/// W3C CSS Properties and Values API Level 1 `@property` 自定义属性注册规则
pub struct CssPropertyRule {
    /// 自定义属性名称（`<dashed-ident>`，如 `"--my-color"`）
    name: string,
    /// 已解析并校验的 `<syntax>` 定义
    syntax: PropertySyntax,
    /// 是否继承（`inherits: true | false`）
    inherits: bool,
    /// 初始值切片（当 `syntax` 为 `"*"` 时可为 `null`）
    initial_value: string? = null,

    /// 解析完整 `@property --name { ... }` 规则字符串
    static fn parse(input: string): CssPropertyRule! {
        const trimmed = trimSpaces(input);
        if (trimmed.len == 0) {
            return err.EmptyInput;
        }

        let pos = skipWsAndComments(trimmed, 0);
        if (pos >= trimmed.len) {
            return err.EmptyInput;
        }

        // 1. 校验 `@property` 前缀（ASCII 大小写无关）
        const at_kw = "@property";
        if (!startsWithIgnoreCaseAscii(trimmed[pos..], at_kw)) {
            return err.ExpectedAtProperty;
        }
        pos += at_kw.len;
        if (pos >= trimmed.len || (!isSpace(trimmed[pos]) && !(trimmed[pos] == '/' && pos + 1 < trimmed.len && trimmed[pos + 1] == '*'))) {
            return err.ExpectedAtProperty;
        }

        // 2. 解析 `<dashed-ident>` 属性名
        pos = skipWsAndComments(trimmed, pos);
        const name_start = pos;
        while (pos < trimmed.len && !isSpace(trimmed[pos]) && trimmed[pos] != '{' && trimmed[pos] != '/') {
            pos += 1;
        }
        const name = trimmed[name_start..pos];
        if (!isValidDashedIdent(name)) {
            return err.InvalidPropertyName;
        }

        // 3. 定位 `{ ... }` 块体
        pos = skipWsAndComments(trimmed, pos);
        if (pos >= trimmed.len || trimmed[pos] != '{') {
            return err.MissingBlock;
        }
        const body_start = pos + 1;

        // 扫描匹配的 `}`（尊重引号、括号与注释）
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
        const after_close = skipWsAndComments(trimmed, close_pos + 1);
        if (after_close != trimmed.len) {
            return err.MissingBlock;
        }

        // 4. 解析声明块内部的 `syntax` / `inherits` / `initial-value`
        const body = trimmed[body_start..close_pos];
        let parsed_syntax: PropertySyntax? = null;
        let parsed_inherits: bool? = null;
        let parsed_initial: string? = null;

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

            // 读取声明结束位置（`;` 或块尾，忽略引号/括号/注释内的 `;`）
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

            // 查找首个 `:`
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

            if (eqlIgnoreCaseAscii(desc_name, "syntax")) {
                const inner = parseQuotedSyntaxString(desc_val)?;
                parsed_syntax = PropertySyntax.parse(inner)?;
            } else if (eqlIgnoreCaseAscii(desc_name, "inherits")) {
                if (eqlIgnoreCaseAscii(desc_val, "true")) {
                    parsed_inherits = true;
                } else if (eqlIgnoreCaseAscii(desc_val, "false")) {
                    parsed_inherits = false;
                } else {
                    return err.InvalidInherits;
                }
            } else if (eqlIgnoreCaseAscii(desc_name, "initial-value")) {
                parsed_initial = desc_val;
            } else {
                return err.InvalidDeclaration;
            }
        }

        // 5. 规范完备性校验：`syntax` 与 `inherits` 必需；非通用语法时 `initial-value` 必需且必须匹配 syntax
        const syntax = parsed_syntax ?? return err.MissingSyntax;
        const inherits = parsed_inherits ?? return err.MissingInherits;

        if (!syntax.isUniversal()) {
            const init_val = parsed_initial ?? return err.MissingInitialValue;
            if (!syntax.matchesValue(init_val)) {
                return err.InvalidInitialValue;
            }
        }

        return CssPropertyRule{
            name: name,
            syntax: syntax,
            inherits: inherits,
            initial_value: parsed_initial,
        };
    }

    /// 校验候选值是否可赋值给当前已注册的 `@property`
    fn acceptsValue(value: string): bool {
        return _.syntax.matchesValue(value);
    }

    fn eql(other: CssPropertyRule): bool {
        if (!std.mem.eql(u8, _.name, other.name)) {
            return false;
        }
        if (_.inherits != other.inherits) {
            return false;
        }
        if (!_.syntax.eql(other.syntax)) {
            return false;
        }
        if (a := _.initial_value) {
            if (b := other.initial_value) {
                return std.mem.eql(u8, a, b);
            }
            return false;
        }
        return other.initial_value == null;
    }

    /// 规范格式化输出（带缩进与换行，等价于 Bun `PropertyRule.toCss` 非压缩模式）
    fn toCss(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "@property ")?;
        pos = appendBytes(buf, pos, _.name)?;
        pos = appendBytes(buf, pos, " {\n  syntax: ")?;
        const syn_written = _.syntax.toCssQuoted(buf[pos..])?;
        pos += syn_written.len;
        pos = appendBytes(buf, pos, ";\n  inherits: ")?;
        pos = appendBytes(buf, pos, if (_.inherits) "true" else "false")?;
        if (init_val := _.initial_value) {
            pos = appendBytes(buf, pos, ";\n  initial-value: ")?;
            pos = appendBytes(buf, pos, init_val)?;
            pos = appendBytes(buf, pos, ";\n}")?;
        } else {
            pos = appendBytes(buf, pos, ";\n}")?;
        }
        return buf[0..pos];
    }

    /// 压缩单行序列化输出（等价于 Bun `PropertyRule.toCss` `dest.minify = true` 模式）
    fn toCssMinified(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "@property ")?;
        pos = appendBytes(buf, pos, _.name)?;
        pos = appendBytes(buf, pos, "{syntax:")?;
        const syn_written = _.syntax.toCssQuoted(buf[pos..])?;
        pos += syn_written.len;
        pos = appendBytes(buf, pos, ";inherits:")?;
        pos = appendBytes(buf, pos, if (_.inherits) "true" else "false")?;
        if (init_val := _.initial_value) {
            pos = appendBytes(buf, pos, ";initial-value:")?;
            pos = appendBytes(buf, pos, init_val)?;
        }
        pos = appendBytes(buf, pos, "}")?;
        return buf[0..pos];
    }
}
