// Source: Absorbed from /data/dev/bun/src/css/properties/text.cp | Synced: 2026-11-21T17:10:30Z

/// CSS Text 3/4 与 Text Decoration 解析错误集
pub err CssTextError {
    InvalidTextTransform,
    InvalidDecorationLine,
    InvalidDecorationThickness,
    InvalidTextDecoration,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= 0.000001;
}

fn splitTokens(input: string, out: *[8]string): usize {
    const s = trimAscii(input);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) { pos += 1; }
        if (pos >= s.len) { break; }
        const start = pos;
        let depth: usize = 0;
        while (pos < s.len) {
            const c = s[pos];
            if (c == '(') { depth += 1; }
            else if (c == ')' && depth > 0) { depth -= 1; }
            else if (depth == 0 && (c == ' ' || c == '\t')) { break; }
            pos += 1;
        }
        if (count < 8) {
            out.*[count] = s[start..pos];
        }
        count += 1;
    }
    return count;
}

/// `text-transform` 大小写转换类别
pub enum TextTransformCase: u8 {
    none = 0,
    uppercase = 1,
    lowercase = 2,
    capitalize = 3,

    static fn parse(input: string): TextTransformCase? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "none")) { return .none; }
        if (std.ascii.eqlIgnoreCase(s, "uppercase")) { return .uppercase; }
        if (std.ascii.eqlIgnoreCase(s, "lowercase")) { return .lowercase; }
        if (std.ascii.eqlIgnoreCase(s, "capitalize")) { return .capitalize; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .none => "none",
            .uppercase => "uppercase",
            .lowercase => "lowercase",
            .capitalize => "capitalize",
        };
    }
}

/// `text-transform` 附加特性位标志
pub const TEXT_TRANSFORM_FULL_WIDTH: u8 = 1 << 0;
pub const TEXT_TRANSFORM_FULL_SIZE_KANA: u8 = 1 << 1;

/// `text-transform` 完整模型（支持大小写转换 + `full-width` / `full-size-kana` 位标志）
pub struct TextTransform {
    case_mode: TextTransformCase = TextTransformCase.none,
    other_bits: u8 = 0,

    static fn default(): TextTransform { return TextTransform{}; }

    fn isDefault(): bool {
        return _.case_mode == TextTransformCase.none && _.other_bits == 0;
    }

    fn hasFullWidth(): bool { return (_.other_bits & TEXT_TRANSFORM_FULL_WIDTH) != 0; }
    fn hasFullSizeKana(): bool { return (_.other_bits & TEXT_TRANSFORM_FULL_SIZE_KANA) != 0; }

    static fn parse(input: string): TextTransform ! CssTextError {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = splitTokens(input, &toks);
        if (n == 0 || n > 3) { return err.InvalidTextTransform; }

        if (n == 1 && std.ascii.eqlIgnoreCase(toks[0], "none")) {
            return TextTransform.default();
        }

        let res = TextTransform{};
        let has_case = false;
        let i: usize = 0;
        while (i < n) {
            const tok = toks[i];
            if (std.ascii.eqlIgnoreCase(tok, "none")) {
                return err.InvalidTextTransform;
            }
            if (std.ascii.eqlIgnoreCase(tok, "full-width")) {
                if (res.hasFullWidth()) { return err.InvalidTextTransform; }
                res.other_bits |= TEXT_TRANSFORM_FULL_WIDTH;
            } else if (std.ascii.eqlIgnoreCase(tok, "full-size-kana")) {
                if (res.hasFullSizeKana()) { return err.InvalidTextTransform; }
                res.other_bits |= TEXT_TRANSFORM_FULL_SIZE_KANA;
            } else if (c := TextTransformCase.parse(tok)) {
                if (has_case) { return err.InvalidTextTransform; }
                res.case_mode = c;
                has_case = true;
            } else {
                return err.InvalidTextTransform;
            }
            i += 1;
        }
        return res;
    }

    fn toCss(buf: []u8): usize ! CssTextError {
        if (_.isDefault()) {
            const s = std.fmt.bufPrint(buf, "none", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        let pos: usize = 0;
        if (_.case_mode != TextTransformCase.none) {
            const s = std.fmt.bufPrint(buf[pos..], "{s}", .{_.case_mode.toCss()}) catch return err.BufferTooSmall;
            pos += s.len;
        }
        if (_.hasFullWidth()) {
            if (pos > 0) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            const s = std.fmt.bufPrint(buf[pos..], "full-width", .{}) catch return err.BufferTooSmall;
            pos += s.len;
        }
        if (_.hasFullSizeKana()) {
            if (pos > 0) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            const s = std.fmt.bufPrint(buf[pos..], "full-size-kana", .{}) catch return err.BufferTooSmall;
            pos += s.len;
        }
        return pos;
    }
}

/// CSS `white-space`（6 变体）
pub enum WhiteSpace: u8 {
    normal = 0,
    pre = 1,
    nowrap = 2,
    pre_wrap = 3,
    break_spaces = 4,
    pre_line = 5,

    static fn default(): WhiteSpace { return .normal; }

    static fn parse(input: string): WhiteSpace? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "pre")) { return .pre; }
        if (std.ascii.eqlIgnoreCase(s, "nowrap")) { return .nowrap; }
        if (std.ascii.eqlIgnoreCase(s, "pre-wrap")) { return .pre_wrap; }
        if (std.ascii.eqlIgnoreCase(s, "break-spaces")) { return .break_spaces; }
        if (std.ascii.eqlIgnoreCase(s, "pre-line")) { return .pre_line; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .pre => "pre",
            .nowrap => "nowrap",
            .pre_wrap => "pre-wrap",
            .break_spaces => "break-spaces",
            .pre_line => "pre-line",
        };
    }

    fn preservesNewlines(): bool {
        return _ == .pre || _ == .pre_wrap || _ == .break_spaces || _ == .pre_line;
    }

    fn allowsWrapping(): bool {
        return _ != .pre && _ != .nowrap;
    }
}

/// CSS `word-break`
pub enum WordBreak: u8 {
    normal = 0,
    keep_all = 1,
    break_all = 2,
    break_word = 3,

    static fn default(): WordBreak { return .normal; }

    static fn parse(input: string): WordBreak? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "keep-all")) { return .keep_all; }
        if (std.ascii.eqlIgnoreCase(s, "break-all")) { return .break_all; }
        if (std.ascii.eqlIgnoreCase(s, "break-word")) { return .break_word; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .keep_all => "keep-all",
            .break_all => "break-all",
            .break_word => "break-word",
        };
    }
}

/// CSS `line-break`
pub enum LineBreak: u8 {
    auto = 0,
    loose = 1,
    normal = 2,
    strict = 3,
    anywhere = 4,

    static fn default(): LineBreak { return .auto; }

    static fn parse(input: string): LineBreak? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return .auto; }
        if (std.ascii.eqlIgnoreCase(s, "loose")) { return .loose; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "strict")) { return .strict; }
        if (std.ascii.eqlIgnoreCase(s, "anywhere")) { return .anywhere; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .auto => "auto",
            .loose => "loose",
            .normal => "normal",
            .strict => "strict",
            .anywhere => "anywhere",
        };
    }
}

/// CSS `hyphens`
pub enum Hyphens: u8 {
    none = 0,
    manual = 1,
    auto = 2,

    static fn default(): Hyphens { return .manual; }

    static fn parse(input: string): Hyphens? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "none")) { return .none; }
        if (std.ascii.eqlIgnoreCase(s, "manual")) { return .manual; }
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return .auto; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .none => "none",
            .manual => "manual",
            .auto => "auto",
        };
    }
}

/// CSS `overflow-wrap`
pub enum OverflowWrap: u8 {
    normal = 0,
    break_word = 1,
    anywhere = 2,

    static fn default(): OverflowWrap { return .normal; }

    static fn parse(input: string): OverflowWrap? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "break-word")) { return .break_word; }
        if (std.ascii.eqlIgnoreCase(s, "anywhere")) { return .anywhere; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .break_word => "break-word",
            .anywhere => "anywhere",
        };
    }
}

/// CSS `text-align`（8 变体）
pub enum TextAlign: u8 {
    start = 0,
    end = 1,
    left = 2,
    right = 3,
    center = 4,
    justify = 5,
    match_parent = 6,
    justify_all = 7,

    static fn default(): TextAlign { return .start; }

    static fn parse(input: string): TextAlign? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "start")) { return .start; }
        if (std.ascii.eqlIgnoreCase(s, "end")) { return .end; }
        if (std.ascii.eqlIgnoreCase(s, "left")) { return .left; }
        if (std.ascii.eqlIgnoreCase(s, "right")) { return .right; }
        if (std.ascii.eqlIgnoreCase(s, "center")) { return .center; }
        if (std.ascii.eqlIgnoreCase(s, "justify")) { return .justify; }
        if (std.ascii.eqlIgnoreCase(s, "match-parent")) { return .match_parent; }
        if (std.ascii.eqlIgnoreCase(s, "justify-all")) { return .justify_all; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .start => "start",
            .end => "end",
            .left => "left",
            .right => "right",
            .center => "center",
            .justify => "justify",
            .match_parent => "match-parent",
            .justify_all => "justify-all",
        };
    }
}

/// `text-decoration-line` 位标志常量
pub const DECO_LINE_UNDERLINE: u8 = 1 << 0;
pub const DECO_LINE_OVERLINE: u8 = 1 << 1;
pub const DECO_LINE_THROUGH: u8 = 1 << 2;
pub const DECO_LINE_BLINK: u8 = 1 << 3;

/// `text-decoration-line` 位标志模型（`none` 或 `underline || overline || line-through || blink`）
pub struct TextDecorationLine {
    bits: u8 = 0,

    static fn none(): TextDecorationLine { return TextDecorationLine{ bits: 0 }; }

    fn isNone(): bool { return _.bits == 0; }
    fn hasUnderline(): bool { return (_.bits & DECO_LINE_UNDERLINE) != 0; }
    fn hasOverline(): bool { return (_.bits & DECO_LINE_OVERLINE) != 0; }
    fn hasLineThrough(): bool { return (_.bits & DECO_LINE_THROUGH) != 0; }
    fn hasBlink(): bool { return (_.bits & DECO_LINE_BLINK) != 0; }

    static fn parseSingleBit(tok: string): u8? {
        if (std.ascii.eqlIgnoreCase(tok, "underline")) { return DECO_LINE_UNDERLINE; }
        if (std.ascii.eqlIgnoreCase(tok, "overline")) { return DECO_LINE_OVERLINE; }
        if (std.ascii.eqlIgnoreCase(tok, "line-through")) { return DECO_LINE_THROUGH; }
        if (std.ascii.eqlIgnoreCase(tok, "blink")) { return DECO_LINE_BLINK; }
        return null;
    }

    static fn parse(input: string): TextDecorationLine ! CssTextError {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = splitTokens(input, &toks);
        if (n == 0 || n > 4) { return err.InvalidDecorationLine; }
        if (n == 1 && std.ascii.eqlIgnoreCase(toks[0], "none")) {
            return TextDecorationLine.none();
        }
        let bits: u8 = 0;
        let i: usize = 0;
        while (i < n) {
            const bit = TextDecorationLine.parseSingleBit(toks[i]) ?? return err.InvalidDecorationLine;
            if ((bits & bit) != 0) { return err.InvalidDecorationLine; }
            bits |= bit;
            i += 1;
        }
        return TextDecorationLine{ bits: bits };
    }

    fn toCss(buf: []u8): usize ! CssTextError {
        if (_.isNone()) {
            const s = std.fmt.bufPrint(buf, "none", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        let pos: usize = 0;
        const labels: [4]string = ["underline", "overline", "line-through", "blink"];
        const masks: [4]u8 = [DECO_LINE_UNDERLINE, DECO_LINE_OVERLINE, DECO_LINE_THROUGH, DECO_LINE_BLINK];
        let i: usize = 0;
        while (i < 4) {
            if ((_.bits & masks[i]) != 0) {
                if (pos > 0) {
                    if (pos >= buf.len) { return err.BufferTooSmall; }
                    buf[pos] = ' ';
                    pos += 1;
                }
                const s = std.fmt.bufPrint(buf[pos..], "{s}", .{labels[i]}) catch return err.BufferTooSmall;
                pos += s.len;
            }
            i += 1;
        }
        return pos;
    }
}

/// `text-decoration-style`
pub enum TextDecorationStyle: u8 {
    solid = 0,
    double = 1,
    dotted = 2,
    dashed = 3,
    wavy = 4,

    static fn default(): TextDecorationStyle { return .solid; }

    static fn parse(input: string): TextDecorationStyle? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "solid")) { return .solid; }
        if (std.ascii.eqlIgnoreCase(s, "double")) { return .double; }
        if (std.ascii.eqlIgnoreCase(s, "dotted")) { return .dotted; }
        if (std.ascii.eqlIgnoreCase(s, "dashed")) { return .dashed; }
        if (std.ascii.eqlIgnoreCase(s, "wavy")) { return .wavy; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .solid => "solid",
            .double => "double",
            .dotted => "dotted",
            .dashed => "dashed",
            .wavy => "wavy",
        };
    }
}

/// `text-decoration-thickness` 类型
pub enum TextDecorationThicknessKind: u8 {
    auto = 0,
    from_font = 1,
    px = 2,
    em = 3,
    percent = 4,
}

/// `text-decoration-thickness` 模型
pub struct TextDecorationThickness {
    kind: TextDecorationThicknessKind = TextDecorationThicknessKind.auto,
    value: f64 = 0.0,

    static fn default(): TextDecorationThickness { return TextDecorationThickness{}; }
    fn isDefault(): bool { return _.kind == TextDecorationThicknessKind.auto; }

    static fn parse(input: string): TextDecorationThickness? {
        const s = trimAscii(input);
        if (s.len == 0) { return null; }
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return TextDecorationThickness{ kind: TextDecorationThicknessKind.auto }; }
        if (std.ascii.eqlIgnoreCase(s, "from-font")) { return TextDecorationThickness{ kind: TextDecorationThicknessKind.from_font }; }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return null;
            if (std.math.isNan(v) || v < 0.0) { return null; }
            return TextDecorationThickness{ kind: TextDecorationThicknessKind.px, value: v };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "em")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return null;
            if (std.math.isNan(v) || v < 0.0) { return null; }
            return TextDecorationThickness{ kind: TextDecorationThicknessKind.em, value: v };
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            if (std.math.isNan(v) || v < 0.0) { return null; }
            return TextDecorationThickness{ kind: TextDecorationThicknessKind.percent, value: v };
        }
        return null;
    }

    fn toCss(buf: []u8): usize ! CssTextError {
        const res = switch (_.kind) {
            .auto => std.fmt.bufPrint(buf, "auto", .{}),
            .from_font => std.fmt.bufPrint(buf, "from-font", .{}),
            .px => std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .em => std.fmt.bufPrint(buf, "{d}em", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

fn isColorToken(tok: string): bool {
    if (tok.len >= 4 && tok[0] == '#') {
        return tok.len == 4 || tok.len == 5 || tok.len == 7 || tok.len == 9;
    }
    if (tok.len > 4 && (std.ascii.eqlIgnoreCase(tok[0..4], "rgb(") || std.ascii.eqlIgnoreCase(tok[0..4], "hsl(")) && tok[tok.len - 1] == ')') {
        return true;
    }
    if (tok.len > 5 && (std.ascii.eqlIgnoreCase(tok[0..5], "rgba(") || std.ascii.eqlIgnoreCase(tok[0..5], "hsla(")) && tok[tok.len - 1] == ')') {
        return true;
    }
    return std.ascii.eqlIgnoreCase(tok, "currentcolor")
        || std.ascii.eqlIgnoreCase(tok, "transparent")
        || std.ascii.eqlIgnoreCase(tok, "red")
        || std.ascii.eqlIgnoreCase(tok, "blue")
        || std.ascii.eqlIgnoreCase(tok, "green")
        || std.ascii.eqlIgnoreCase(tok, "black")
        || std.ascii.eqlIgnoreCase(tok, "white");
}

/// CSS `text-decoration` 简写模型（任意顺序解析 `line || style || color || thickness`）
pub struct TextDecoration {
    line: TextDecorationLine = TextDecorationLine.none(),
    style: TextDecorationStyle = TextDecorationStyle.solid,
    color: string? = null,
    thickness: TextDecorationThickness = TextDecorationThickness.default(),

    static fn default(): TextDecoration { return TextDecoration{}; }

    static fn parse(input: string): TextDecoration ! CssTextError {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = splitTokens(input, &toks);
        if (n == 0 || n > 7) { return err.InvalidTextDecoration; }

        let d = TextDecoration{};
        let has_none_line = false;
        let has_style = false;
        let has_thick = false;

        let i: usize = 0;
        while (i < n) {
            const tok = toks[i];
            if (std.ascii.eqlIgnoreCase(tok, "none")) {
                if (has_none_line || !d.line.isNone()) { return err.InvalidTextDecoration; }
                has_none_line = true;
                i += 1;
                continue;
            }
            if (bit := TextDecorationLine.parseSingleBit(tok)) {
                if (has_none_line || (d.line.bits & bit) != 0) { return err.InvalidTextDecoration; }
                d.line.bits |= bit;
                i += 1;
                continue;
            }
            if (!has_style) {
                if (st := TextDecorationStyle.parse(tok)) {
                    d.style = st;
                    has_style = true;
                    i += 1;
                    continue;
                }
            }
            if (!has_thick) {
                if (th := TextDecorationThickness.parse(tok)) {
                    d.thickness = th;
                    has_thick = true;
                    i += 1;
                    continue;
                }
            }
            if (d.color == null && isColorToken(tok)) {
                d.color = tok;
                i += 1;
                continue;
            }
            return err.InvalidTextDecoration;
        }
        return d;
    }

    /// 规范化序列化（省略默认 `solid`、`auto` thickness 与 `currentcolor`）
    fn toCss(buf: []u8): usize ! CssTextError {
        let pos: usize = 0;
        pos += _.line.toCss(buf[pos..])?;

        if (_.style != TextDecorationStyle.solid) {
            const s = std.fmt.bufPrint(buf[pos..], " {s}", .{_.style.toCss()}) catch return err.BufferTooSmall;
            pos += s.len;
        }
        if (c := _.color) {
            if (!std.ascii.eqlIgnoreCase(c, "currentcolor")) {
                const s = std.fmt.bufPrint(buf[pos..], " {s}", .{c}) catch return err.BufferTooSmall;
                pos += s.len;
            }
        }
        if (!_.thickness.isDefault()) {
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
            pos += _.thickness.toCss(buf[pos..])?;
        }
        return pos;
    }
}
