// Source: Absorbed from /data/dev/bun/src/css/properties/font.cp

/// CSS Fonts Level 4 属性族解析错误集
pub err CssFontError {
    InvalidWeight,
    InvalidStyle,
    InvalidStretch,
    InvalidSize,
    InvalidLineHeight,
    InvalidVariantCaps,
    InvalidFamily,
    InvalidFontShorthand,
    TooManyFamilies,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= 0.000001;
}

/// `font-weight` 种类
pub enum FontWeightKind: u8 {
    number = 0,
    bolder = 1,
    lighter = 2,
}

/// `font-weight` 模型（`normal`=400 / `bold`=700 / `bolder` / `lighter` / `1..1000`）
pub struct FontWeight {
    kind: FontWeightKind = FontWeightKind.number,
    value: f64 = 400.0,

    static fn normal(): FontWeight { return FontWeight{ kind: FontWeightKind.number, value: 400.0 }; }
    static fn bold(): FontWeight { return FontWeight{ kind: FontWeightKind.number, value: 700.0 }; }
    static fn bolder(): FontWeight { return FontWeight{ kind: FontWeightKind.bolder, value: 0.0 }; }
    static fn lighter(): FontWeight { return FontWeight{ kind: FontWeightKind.lighter, value: 0.0 }; }

    fn isDefault(): bool {
        return _.kind == FontWeightKind.number && approxEq(_.value, 400.0);
    }

    fn isBold(): bool {
        return _.kind == FontWeightKind.number && _.value >= 700.0;
    }

    static fn parse(input: string): FontWeight ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidWeight; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return FontWeight.normal(); }
        if (std.ascii.eqlIgnoreCase(s, "bold")) { return FontWeight.bold(); }
        if (std.ascii.eqlIgnoreCase(s, "bolder")) { return FontWeight.bolder(); }
        if (std.ascii.eqlIgnoreCase(s, "lighter")) { return FontWeight.lighter(); }
        const v = std.fmt.parseFloat(f64, s) catch return err.InvalidWeight;
        if (std.math.isNan(v) || std.math.isInf(v) || v < 1.0 || v > 1000.0) {
            return err.InvalidWeight;
        }
        return FontWeight{ kind: FontWeightKind.number, value: v };
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        const res = switch (_.kind) {
            .bolder => std.fmt.bufPrint(buf, "bolder", .{}),
            .lighter => std.fmt.bufPrint(buf, "lighter", .{}),
            .number => if (approxEq(_.value, 400.0))
                std.fmt.bufPrint(buf, "normal", .{})
            else if (approxEq(_.value, 700.0))
                std.fmt.bufPrint(buf, "bold", .{})
            else
                std.fmt.bufPrint(buf, "{d}", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

/// `font-style` 种类
pub enum FontStyleKind: u8 {
    normal = 0,
    italic = 1,
    oblique = 2,
}

/// `font-style` 模型（`normal` / `italic` / `oblique <angle>`，角度范围 `-90deg..90deg`）
pub struct FontStyle {
    kind: FontStyleKind = FontStyleKind.normal,
    angle_deg: f64 = 14.0,

    static fn normal(): FontStyle { return FontStyle{ kind: FontStyleKind.normal, angle_deg: 0.0 }; }
    static fn italic(): FontStyle { return FontStyle{ kind: FontStyleKind.italic, angle_deg: 0.0 }; }
    static fn oblique(deg: f64): FontStyle ! CssFontError {
        if (std.math.isNan(deg) || std.math.isInf(deg) || deg < -90.0 || deg > 90.0) {
            return err.InvalidStyle;
        }
        return FontStyle{ kind: FontStyleKind.oblique, angle_deg: deg };
    }

    fn isDefault(): bool {
        return _.kind == FontStyleKind.normal;
    }

    static fn parse(input: string): FontStyle ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidStyle; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return FontStyle.normal(); }
        if (std.ascii.eqlIgnoreCase(s, "italic")) { return FontStyle.italic(); }
        if (std.ascii.eqlIgnoreCase(s, "oblique")) { return FontStyle.oblique(14.0); }
        if (s.len > 8 && std.ascii.eqlIgnoreCase(s[0..7], "oblique") && (s[7] == ' ' || s[7] == '\t')) {
            const ang_str = trimAscii(s[8..]);
            if (ang_str.len <= 3 || !std.ascii.eqlIgnoreCase(ang_str[ang_str.len - 3 ..], "deg")) {
                return err.InvalidStyle;
            }
            const deg = std.fmt.parseFloat(f64, ang_str[0 .. ang_str.len - 3]) catch return err.InvalidStyle;
            return FontStyle.oblique(deg);
        }
        return err.InvalidStyle;
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        const res = switch (_.kind) {
            .normal => std.fmt.bufPrint(buf, "normal", .{}),
            .italic => std.fmt.bufPrint(buf, "italic", .{}),
            .oblique => if (approxEq(_.angle_deg, 14.0))
                std.fmt.bufPrint(buf, "oblique", .{})
            else
                std.fmt.bufPrint(buf, "oblique {d}deg", .{_.angle_deg}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

/// `font-stretch` 模型（`ultra-condensed`..`ultra-expanded` 或百分比）
pub struct FontStretch {
    percent: f64 = 100.0,

    static fn default(): FontStretch { return FontStretch{ percent: 100.0 }; }

    fn isDefault(): bool {
        return approxEq(_.percent, 100.0);
    }

    static fn parse(input: string): FontStretch ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidStretch; }
        if (std.ascii.eqlIgnoreCase(s, "ultra-condensed")) { return FontStretch{ percent: 50.0 }; }
        if (std.ascii.eqlIgnoreCase(s, "extra-condensed")) { return FontStretch{ percent: 62.5 }; }
        if (std.ascii.eqlIgnoreCase(s, "condensed")) { return FontStretch{ percent: 75.0 }; }
        if (std.ascii.eqlIgnoreCase(s, "semi-condensed")) { return FontStretch{ percent: 87.5 }; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return FontStretch{ percent: 100.0 }; }
        if (std.ascii.eqlIgnoreCase(s, "semi-expanded")) { return FontStretch{ percent: 112.5 }; }
        if (std.ascii.eqlIgnoreCase(s, "expanded")) { return FontStretch{ percent: 125.0 }; }
        if (std.ascii.eqlIgnoreCase(s, "extra-expanded")) { return FontStretch{ percent: 150.0 }; }
        if (std.ascii.eqlIgnoreCase(s, "ultra-expanded")) { return FontStretch{ percent: 200.0 }; }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return err.InvalidStretch;
            if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) { return err.InvalidStretch; }
            return FontStretch{ percent: v };
        }
        return err.InvalidStretch;
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        const res = if (approxEq(_.percent, 50.0)) std.fmt.bufPrint(buf, "ultra-condensed", .{})
        else if (approxEq(_.percent, 62.5)) std.fmt.bufPrint(buf, "extra-condensed", .{})
        else if (approxEq(_.percent, 75.0)) std.fmt.bufPrint(buf, "condensed", .{})
        else if (approxEq(_.percent, 87.5)) std.fmt.bufPrint(buf, "semi-condensed", .{})
        else if (approxEq(_.percent, 100.0)) std.fmt.bufPrint(buf, "normal", .{})
        else if (approxEq(_.percent, 112.5)) std.fmt.bufPrint(buf, "semi-expanded", .{})
        else if (approxEq(_.percent, 125.0)) std.fmt.bufPrint(buf, "expanded", .{})
        else if (approxEq(_.percent, 150.0)) std.fmt.bufPrint(buf, "extra-expanded", .{})
        else if (approxEq(_.percent, 200.0)) std.fmt.bufPrint(buf, "ultra-expanded", .{})
        else std.fmt.bufPrint(buf, "{d}%", .{_.percent});
        const slice = res catch return err.BufferTooSmall;
        return slice.len;
    }
}

/// 绝对字体大小关键字
pub enum AbsoluteFontSize: u8 {
    xx_small = 0,
    x_small = 1,
    small = 2,
    medium = 3,
    large = 4,
    x_large = 5,
    xx_large = 6,
    xxx_large = 7,

    static fn parse(input: string): AbsoluteFontSize? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "xx-small")) { return .xx_small; }
        if (std.ascii.eqlIgnoreCase(s, "x-small")) { return .x_small; }
        if (std.ascii.eqlIgnoreCase(s, "small")) { return .small; }
        if (std.ascii.eqlIgnoreCase(s, "medium")) { return .medium; }
        if (std.ascii.eqlIgnoreCase(s, "large")) { return .large; }
        if (std.ascii.eqlIgnoreCase(s, "x-large")) { return .x_large; }
        if (std.ascii.eqlIgnoreCase(s, "xx-large")) { return .xx_large; }
        if (std.ascii.eqlIgnoreCase(s, "xxx-large")) { return .xxx_large; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .xx_small => "xx-small",
            .x_small => "x-small",
            .small => "small",
            .medium => "medium",
            .large => "large",
            .x_large => "x-large",
            .xx_large => "xx-large",
            .xxx_large => "xxx-large",
        };
    }
}

/// 相对字体大小关键字
pub enum RelativeFontSize: u8 {
    smaller = 0,
    larger = 1,

    static fn parse(input: string): RelativeFontSize? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "smaller")) { return .smaller; }
        if (std.ascii.eqlIgnoreCase(s, "larger")) { return .larger; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .smaller => "smaller",
            .larger => "larger",
        };
    }
}

/// `font-size` 种类
pub enum FontSizeKind: u8 {
    absolute = 0,
    relative = 1,
    px = 2,
    rem = 3,
    em = 4,
    percent = 5,
}

/// `font-size` 模型
pub struct FontSize {
    kind: FontSizeKind = FontSizeKind.absolute,
    abs: AbsoluteFontSize = AbsoluteFontSize.medium,
    rel: RelativeFontSize = RelativeFontSize.larger,
    value: f64 = 0.0,

    static fn default(): FontSize { return FontSize{}; }

    static fn parse(input: string): FontSize ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidSize; }
        if (a := AbsoluteFontSize.parse(s)) {
            return FontSize{ kind: FontSizeKind.absolute, abs: a };
        }
        if (r := RelativeFontSize.parse(s)) {
            return FontSize{ kind: FontSizeKind.relative, rel: r };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return err.InvalidSize;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidSize; }
            return FontSize{ kind: FontSizeKind.px, value: v };
        }
        if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rem")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 3]) catch return err.InvalidSize;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidSize; }
            return FontSize{ kind: FontSizeKind.rem, value: v };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "em")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return err.InvalidSize;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidSize; }
            return FontSize{ kind: FontSizeKind.em, value: v };
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return err.InvalidSize;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidSize; }
            return FontSize{ kind: FontSizeKind.percent, value: v };
        }
        return err.InvalidSize;
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        const res = switch (_.kind) {
            .absolute => std.fmt.bufPrint(buf, "{s}", .{_.abs.toCss()}),
            .relative => std.fmt.bufPrint(buf, "{s}", .{_.rel.toCss()}),
            .px => std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .rem => std.fmt.bufPrint(buf, "{d}rem", .{_.value}),
            .em => std.fmt.bufPrint(buf, "{d}em", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

/// `line-height` 种类
pub enum LineHeightKind: u8 {
    normal = 0,
    number = 1,
    px = 2,
    rem = 3,
    em = 4,
    percent = 5,
}

/// `line-height` 模型（`normal` / 无单位倍率 / 长度或百分比）
pub struct LineHeight {
    kind: LineHeightKind = LineHeightKind.normal,
    value: f64 = 0.0,

    static fn default(): LineHeight { return LineHeight{}; }

    fn isDefault(): bool {
        return _.kind == LineHeightKind.normal;
    }

    static fn parse(input: string): LineHeight ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidLineHeight; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) {
            return LineHeight.default();
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return err.InvalidLineHeight;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidLineHeight; }
            return LineHeight{ kind: LineHeightKind.px, value: v };
        }
        if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rem")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 3]) catch return err.InvalidLineHeight;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidLineHeight; }
            return LineHeight{ kind: LineHeightKind.rem, value: v };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "em")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return err.InvalidLineHeight;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidLineHeight; }
            return LineHeight{ kind: LineHeightKind.em, value: v };
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return err.InvalidLineHeight;
            if (std.math.isNan(v) || v < 0.0) { return err.InvalidLineHeight; }
            return LineHeight{ kind: LineHeightKind.percent, value: v };
        }
        const v = std.fmt.parseFloat(f64, s) catch return err.InvalidLineHeight;
        if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) { return err.InvalidLineHeight; }
        return LineHeight{ kind: LineHeightKind.number, value: v };
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        const res = switch (_.kind) {
            .normal => std.fmt.bufPrint(buf, "normal", .{}),
            .number => std.fmt.bufPrint(buf, "{d}", .{_.value}),
            .px => std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .rem => std.fmt.bufPrint(buf, "{d}rem", .{_.value}),
            .em => std.fmt.bufPrint(buf, "{d}em", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

/// `font-variant-caps`（7 变体）
pub enum FontVariantCaps: u8 {
    normal = 0,
    small_caps = 1,
    all_small_caps = 2,
    petite_caps = 3,
    all_petite_caps = 4,
    unicase = 5,
    titling_caps = 6,

    static fn default(): FontVariantCaps { return .normal; }

    static fn parse(input: string): FontVariantCaps? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "small-caps")) { return .small_caps; }
        if (std.ascii.eqlIgnoreCase(s, "all-small-caps")) { return .all_small_caps; }
        if (std.ascii.eqlIgnoreCase(s, "petite-caps")) { return .petite_caps; }
        if (std.ascii.eqlIgnoreCase(s, "all-petite-caps")) { return .all_petite_caps; }
        if (std.ascii.eqlIgnoreCase(s, "unicase")) { return .unicase; }
        if (std.ascii.eqlIgnoreCase(s, "titling-caps")) { return .titling_caps; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .small_caps => "small-caps",
            .all_small_caps => "all-small-caps",
            .petite_caps => "petite-caps",
            .all_petite_caps => "all-petite-caps",
            .unicase => "unicase",
            .titling_caps => "titling-caps",
        };
    }
}

/// CSS Fonts Level 4 全部 13 种通用字体族
pub enum GenericFontFamily: u8 {
    serif = 0,
    sans_serif = 1,
    monospace = 2,
    cursive = 3,
    fantasy = 4,
    system_ui = 5,
    ui_serif = 6,
    ui_sans_serif = 7,
    ui_monospace = 8,
    ui_rounded = 9,
    emoji = 10,
    math = 11,
    fangsong = 12,

    static fn parse(input: string): GenericFontFamily? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "serif")) { return .serif; }
        if (std.ascii.eqlIgnoreCase(s, "sans-serif")) { return .sans_serif; }
        if (std.ascii.eqlIgnoreCase(s, "monospace")) { return .monospace; }
        if (std.ascii.eqlIgnoreCase(s, "cursive")) { return .cursive; }
        if (std.ascii.eqlIgnoreCase(s, "fantasy")) { return .fantasy; }
        if (std.ascii.eqlIgnoreCase(s, "system-ui")) { return .system_ui; }
        if (std.ascii.eqlIgnoreCase(s, "ui-serif")) { return .ui_serif; }
        if (std.ascii.eqlIgnoreCase(s, "ui-sans-serif")) { return .ui_sans_serif; }
        if (std.ascii.eqlIgnoreCase(s, "ui-monospace")) { return .ui_monospace; }
        if (std.ascii.eqlIgnoreCase(s, "ui-rounded")) { return .ui_rounded; }
        if (std.ascii.eqlIgnoreCase(s, "emoji")) { return .emoji; }
        if (std.ascii.eqlIgnoreCase(s, "math")) { return .math; }
        if (std.ascii.eqlIgnoreCase(s, "fangsong")) { return .fangsong; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .serif => "serif",
            .sans_serif => "sans-serif",
            .monospace => "monospace",
            .cursive => "cursive",
            .fantasy => "fantasy",
            .system_ui => "system-ui",
            .ui_serif => "ui-serif",
            .ui_sans_serif => "ui-sans-serif",
            .ui_monospace => "ui-monospace",
            .ui_rounded => "ui-rounded",
            .emoji => "emoji",
            .math => "math",
            .fangsong => "fangsong",
        };
    }
}

/// 单个字体族项（通用族或自定义名称）
pub struct FontFamilyItem {
    name: string = "",
    generic: GenericFontFamily? = null,
    quoted: bool = false,

    static fn parse(input: string): FontFamilyItem ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidFamily; }
        if ((s[0] == '"' || s[0] == '\'')) {
            if (s.len < 3 || s[s.len - 1] != s[0]) { return err.InvalidFamily; }
            const inner = s[1 .. s.len - 1];
            if (inner.len == 0) { return err.InvalidFamily; }
            return FontFamilyItem{ name: inner, generic: null, quoted: true };
        }
        if (g := GenericFontFamily.parse(s)) {
            return FontFamilyItem{ name: g.toCss(), generic: g, quoted: false };
        }
        return FontFamilyItem{ name: s, generic: null, quoted: false };
    }

    fn toCss(buf: []u8): usize ! CssFontError {
        if (_.quoted || std.mem.indexOfScalar(u8, _.name, ' ') != null) {
            const s = std.fmt.bufPrint(buf, "\"{s}\"", .{_.name}) catch return err.BufferTooSmall;
            return s.len;
        }
        const s = std.fmt.bufPrint(buf, "{s}", .{_.name}) catch return err.BufferTooSmall;
        return s.len;
    }
}

/// 完整 `font` 简写模型（`[style || variant-caps || weight || stretch]? <font-size> [/<line-height>]? <font-family>#`）
pub struct Font {
    style: FontStyle = FontStyle.normal(),
    variant_caps: FontVariantCaps = FontVariantCaps.normal,
    weight: FontWeight = FontWeight.normal(),
    stretch: FontStretch = FontStretch.default(),
    size: FontSize = FontSize.default(),
    line_height: LineHeight = LineHeight.default(),
    families: [8]FontFamilyItem = [
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
        FontFamilyItem{},
    ],
    family_count: usize = 0,

    static fn parse(input: string): Font ! CssFontError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidFontShorthand; }

        let f = Font{};
        let has_style = false;
        let has_variant = false;
        let has_weight = false;
        let has_stretch = false;
        let has_size = false;

        let pos: usize = 0;
        while (pos < s.len && !has_size) {
            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) { pos += 1; }
            if (pos >= s.len) { break; }
            const tok_start = pos;
            while (pos < s.len && s[pos] != ' ' && s[pos] != '\t') { pos += 1; }
            const tok = s[tok_start..pos];

            // 检查是否含 `size/line-height` 紧连斜杠形式（如 `16px/1.5`）
            if (slash := std.mem.indexOfScalar(u8, tok, '/')) {
                const sz_tok = tok[0..slash];
                const lh_tok = tok[slash + 1 ..];
                f.size = FontSize.parse(sz_tok) catch return err.InvalidFontShorthand;
                if (lh_tok.len > 0) {
                    f.line_height = LineHeight.parse(lh_tok)?;
                } else {
                    while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) { pos += 1; }
                    const lh_start = pos;
                    while (pos < s.len && s[pos] != ' ' && s[pos] != '\t') { pos += 1; }
                    f.line_height = LineHeight.parse(s[lh_start..pos])?;
                }
                has_size = true;
                break;
            }

            // 尝试解析为 <font-size>
            if (sz := FontSize.parse(tok) catch null) {
                f.size = sz;
                has_size = true;
                // 检查后续是否有独立 ` / <line-height>`
                let peek = pos;
                while (peek < s.len && (s[peek] == ' ' || s[peek] == '\t')) { peek += 1; }
                if (peek < s.len && s[peek] == '/') {
                    peek += 1;
                    while (peek < s.len && (s[peek] == ' ' || s[peek] == '\t')) { peek += 1; }
                    const lh_start = peek;
                    while (peek < s.len && s[peek] != ' ' && s[peek] != '\t') { peek += 1; }
                    f.line_height = LineHeight.parse(s[lh_start..peek])?;
                    pos = peek;
                }
                break;
            }

            // 否则解析可选前缀属性（style / variant-caps / weight / stretch）
            if (std.ascii.eqlIgnoreCase(tok, "normal")) {
                continue;
            }
            if (!has_style && (std.ascii.eqlIgnoreCase(tok, "italic") || std.ascii.eqlIgnoreCase(tok, "oblique"))) {
                f.style = FontStyle.parse(tok)?;
                has_style = true;
                continue;
            }
            if (!has_variant && std.ascii.eqlIgnoreCase(tok, "small-caps")) {
                f.variant_caps = FontVariantCaps.small_caps;
                has_variant = true;
                continue;
            }
            if (!has_weight) {
                if (w := FontWeight.parse(tok) catch null) {
                    f.weight = w;
                    has_weight = true;
                    continue;
                }
            }
            if (!has_stretch) {
                if (st := FontStretch.parse(tok) catch null) {
                    f.stretch = st;
                    has_stretch = true;
                    continue;
                }
            }
            return err.InvalidFontShorthand;
        }

        if (!has_size) {
            return err.InvalidFontShorthand;
        }

        // 解析必填的逗号分隔 <font-family> 列表
        const fam_rest = trimAscii(s[pos..]);
        if (fam_rest.len == 0) {
            return err.InvalidFontShorthand;
        }

        let start: usize = 0;
        let idx: usize = 0;
        let quote: u8 = 0;
        while (idx <= fam_rest.len) {
            const at_end = idx == fam_rest.len;
            if (!at_end) {
                const c = fam_rest[idx];
                if (quote == 0 && (c == '"' || c == '\'')) {
                    quote = c;
                } else if (quote != 0 && c == quote) {
                    quote = 0;
                }
            }
            if (at_end || (quote == 0 && fam_rest[idx] == ',')) {
                const item_str = trimAscii(fam_rest[start..idx]);
                if (f.family_count >= 8) { return err.TooManyFamilies; }
                f.families[f.family_count] = FontFamilyItem.parse(item_str)?;
                f.family_count += 1;
                start = idx + 1;
            }
            idx += 1;
        }
        if (quote != 0 || f.family_count == 0) {
            return err.InvalidFamily;
        }
        return f;
    }

    /// 规范化序列化为 CSS `font` 简写（省略默认 style/variant/weight/stretch 与 normal 行高）
    fn toCss(buf: []u8): usize ! CssFontError {
        let pos: usize = 0;
        if (!_.style.isDefault()) {
            pos += _.style.toCss(buf[pos..])?;
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
        }
        if (_.variant_caps != FontVariantCaps.normal) {
            const vstr = _.variant_caps.toCss();
            if (pos + vstr.len + 1 > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + vstr.len], vstr);
            pos += vstr.len;
            buf[pos] = ' ';
            pos += 1;
        }
        if (!_.weight.isDefault()) {
            pos += _.weight.toCss(buf[pos..])?;
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
        }
        if (!_.stretch.isDefault()) {
            pos += _.stretch.toCss(buf[pos..])?;
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
        }

        pos += _.size.toCss(buf[pos..])?;
        if (!_.line_height.isDefault()) {
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = '/';
            pos += 1;
            pos += _.line_height.toCss(buf[pos..])?;
        }

        if (pos >= buf.len) { return err.BufferTooSmall; }
        buf[pos] = ' ';
        pos += 1;

        let i: usize = 0;
        while (i < _.family_count) {
            if (i > 0) {
                const sep = ", ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            pos += _.families[i].toCss(buf[pos..])?;
            i += 1;
        }
        return pos;
    }
}
