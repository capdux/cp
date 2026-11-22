// Source: Absorbed from /data/dev/bun/src/css/properties/text.cp | Synced: 2026-11-22T10:15:23Z

/// CSS `text-transform` 大小写转换枚举 (CSS Text Level 3)
pub enum TextTransformCase: u8 {
    none = 0,
    uppercase = 1,
    lowercase = 2,
    capitalize = 3,

    static fn fromStr(s: string): TextTransformCase? {
        if (std.mem.eql(u8, s, "none")) { return .none; }
        if (std.mem.eql(u8, s, "uppercase")) { return .uppercase; }
        if (std.mem.eql(u8, s, "lowercase")) { return .lowercase; }
        if (std.mem.eql(u8, s, "capitalize")) { return .capitalize; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .none => "none",
            .uppercase => "uppercase",
            .lowercase => "lowercase",
            .capitalize => "capitalize",
        };
    }

    fn isActive(): bool {
        return _ != .none;
    }
}

/// CSS `text-transform` 东亚全角/假名附加标志
pub struct TextTransformOther {
    full_width: bool = false,
    full_size_kana: bool = false,

    fn isEmpty(): bool {
        return !_.full_width && !_.full_size_kana;
    }

    fn toU8(): u8 {
        let v: u8 = 0;
        if (_.full_width) { v |= 1; }
        if (_.full_size_kana) { v |= 2; }
        return v;
    }

    static fn fromU8(v: u8): TextTransformOther {
        return TextTransformOther{
            full_width: (v & 1) != 0,
            full_size_kana: (v & 2) != 0,
        };
    }

    fn eql(other: TextTransformOther): bool {
        return _.full_width == other.full_width && _.full_size_kana == other.full_size_kana;
    }
}

/// CSS `white-space` 空白与换行控制枚举
pub enum WhiteSpace: u8 {
    normal = 0,
    pre = 1,
    nowrap = 2,
    pre_wrap = 3,
    break_spaces = 4,
    pre_line = 5,

    static fn fromStr(s: string): WhiteSpace? {
        if (std.mem.eql(u8, s, "normal")) { return .normal; }
        if (std.mem.eql(u8, s, "pre")) { return .pre; }
        if (std.mem.eql(u8, s, "nowrap")) { return .nowrap; }
        if (std.mem.eql(u8, s, "pre-wrap")) { return .pre_wrap; }
        if (std.mem.eql(u8, s, "break-spaces")) { return .break_spaces; }
        if (std.mem.eql(u8, s, "pre-line")) { return .pre_line; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .normal => "normal",
            .pre => "pre",
            .nowrap => "nowrap",
            .pre_wrap => "pre-wrap",
            .break_spaces => "break-spaces",
            .pre_line => "pre-line",
        };
    }

    /// 是否保留连续空格
    fn preservesSpaces(): bool {
        return _ == .pre || _ == .pre_wrap || _ == .break_spaces;
    }

    /// 是否保留源码中的硬换行符
    fn preservesNewlines(): bool {
        return _ == .pre || _ == .pre_wrap || _ == .break_spaces || _ == .pre_line;
    }

    /// 是否允许行尾自动软折行
    fn allowsWrap(): bool {
        return _ == .normal || _ == .pre_wrap || _ == .break_spaces || _ == .pre_line;
    }
}

/// CSS `word-break` 断词策略枚举
pub enum WordBreak: u8 {
    normal = 0,
    keep_all = 1,
    break_all = 2,
    break_word = 3,

    static fn fromStr(s: string): WordBreak? {
        if (std.mem.eql(u8, s, "normal")) { return .normal; }
        if (std.mem.eql(u8, s, "keep-all")) { return .keep_all; }
        if (std.mem.eql(u8, s, "break-all")) { return .break_all; }
        if (std.mem.eql(u8, s, "break-word")) { return .break_word; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .normal => "normal",
            .keep_all => "keep-all",
            .break_all => "break-all",
            .break_word => "break-word",
        };
    }

    fn breaksAnywhere(): bool {
        return _ == .break_all || _ == .break_word;
    }
}

/// CSS `overflow-wrap` 溢出断行策略枚举
pub enum OverflowWrap: u8 {
    normal = 0,
    break_word = 1,
    anywhere = 2,

    static fn fromStr(s: string): OverflowWrap? {
        if (std.mem.eql(u8, s, "normal")) { return .normal; }
        if (std.mem.eql(u8, s, "break-word")) { return .break_word; }
        if (std.mem.eql(u8, s, "anywhere")) { return .anywhere; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .normal => "normal",
            .break_word => "break-word",
            .anywhere => "anywhere",
        };
    }

    fn allowsEmergencyBreak(): bool {
        return _ == .break_word || _ == .anywhere;
    }
}

/// CSS `text-align` 文本对齐枚举
pub enum TextAlign: u8 {
    start = 0,
    end = 1,
    left = 2,
    right = 3,
    center = 4,
    justify = 5,
    match_parent = 6,
    justify_all = 7,

    static fn fromStr(s: string): TextAlign? {
        if (std.mem.eql(u8, s, "start")) { return .start; }
        if (std.mem.eql(u8, s, "end")) { return .end; }
        if (std.mem.eql(u8, s, "left")) { return .left; }
        if (std.mem.eql(u8, s, "right")) { return .right; }
        if (std.mem.eql(u8, s, "center")) { return .center; }
        if (std.mem.eql(u8, s, "justify")) { return .justify; }
        if (std.mem.eql(u8, s, "match-parent")) { return .match_parent; }
        if (std.mem.eql(u8, s, "justify-all")) { return .justify_all; }
        return null;
    }

    fn asStr(): string {
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

    /// 是否为依赖书写方向的逻辑对齐方式
    fn isLogical(): bool {
        return _ == .start || _ == .end || _ == .match_parent;
    }

    /// 是否为两端对齐
    fn isJustified(): bool {
        return _ == .justify || _ == .justify_all;
    }
}

/// CSS `text-decoration-line` 装饰线位标志结构体
pub struct TextDecorationLine {
    underline: bool = false,
    overline: bool = false,
    line_through: bool = false,
    blink: bool = false,
    spelling_error: bool = false,
    grammar_error: bool = false,

    fn isNone(): bool {
        return _.toU8() == 0;
    }

    fn hasLine(): bool {
        return _.underline || _.overline || _.line_through;
    }

    fn toU8(): u8 {
        let v: u8 = 0;
        if (_.underline) { v |= 1; }
        if (_.overline) { v |= 2; }
        if (_.line_through) { v |= 4; }
        if (_.blink) { v |= 8; }
        if (_.spelling_error) { v |= 16; }
        if (_.grammar_error) { v |= 32; }
        return v;
    }

    static fn fromU8(v: u8): TextDecorationLine {
        return TextDecorationLine{
            underline: (v & 1) != 0,
            overline: (v & 2) != 0,
            line_through: (v & 4) != 0,
            blink: (v & 8) != 0,
            spelling_error: (v & 16) != 0,
            grammar_error: (v & 32) != 0,
        };
    }

    fn eql(other: TextDecorationLine): bool {
        return _.toU8() == other.toU8();
    }
}

/// CSS `direction` 书写方向枚举
pub enum Direction: u8 {
    ltr = 0,
    rtl = 1,

    static fn fromStr(s: string): Direction? {
        if (std.mem.eql(u8, s, "ltr")) { return .ltr; }
        if (std.mem.eql(u8, s, "rtl")) { return .rtl; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .ltr => "ltr",
            .rtl => "rtl",
        };
    }

    fn isRtl(): bool {
        return _ == .rtl;
    }
}

/// CSS `list-style-type` 预定义列表标记类型枚举
pub enum ListStyleType: u8 {
    none = 0,
    disc = 1,
    circle = 2,
    square = 3,
    decimal = 4,
    decimal_leading_zero = 5,
    lower_roman = 6,
    upper_roman = 7,
    lower_alpha = 8,
    upper_alpha = 9,
    lower_latin = 10,
    upper_latin = 11,
    lower_greek = 12,
    cjk_decimal = 13,

    static fn fromStr(s: string): ListStyleType? {
        if (std.mem.eql(u8, s, "none")) { return .none; }
        if (std.mem.eql(u8, s, "disc")) { return .disc; }
        if (std.mem.eql(u8, s, "circle")) { return .circle; }
        if (std.mem.eql(u8, s, "square")) { return .square; }
        if (std.mem.eql(u8, s, "decimal")) { return .decimal; }
        if (std.mem.eql(u8, s, "decimal-leading-zero")) { return .decimal_leading_zero; }
        if (std.mem.eql(u8, s, "lower-roman")) { return .lower_roman; }
        if (std.mem.eql(u8, s, "upper-roman")) { return .upper_roman; }
        if (std.mem.eql(u8, s, "lower-alpha")) { return .lower_alpha; }
        if (std.mem.eql(u8, s, "upper-alpha")) { return .upper_alpha; }
        if (std.mem.eql(u8, s, "lower-latin")) { return .lower_latin; }
        if (std.mem.eql(u8, s, "upper-latin")) { return .upper_latin; }
        if (std.mem.eql(u8, s, "lower-greek")) { return .lower_greek; }
        if (std.mem.eql(u8, s, "cjk-decimal")) { return .cjk_decimal; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .none => "none",
            .disc => "disc",
            .circle => "circle",
            .square => "square",
            .decimal => "decimal",
            .decimal_leading_zero => "decimal-leading-zero",
            .lower_roman => "lower-roman",
            .upper_roman => "upper-roman",
            .lower_alpha => "lower-alpha",
            .upper_alpha => "upper-alpha",
            .lower_latin => "lower-latin",
            .upper_latin => "upper-latin",
            .lower_greek => "lower-greek",
            .cjk_decimal => "cjk-decimal",
        };
    }

    fn isSymbolic(): bool {
        return _ == .disc || _ == .circle || _ == .square;
    }

    fn isNumeric(): bool {
        return _ == .decimal || _ == .decimal_leading_zero || _ == .cjk_decimal;
    }

    fn isAlphabetic(): bool {
        return _ == .lower_alpha || _ == .upper_alpha || _ == .lower_latin || _ == .upper_latin || _ == .lower_greek;
    }

    fn isRoman(): bool {
        return _ == .lower_roman || _ == .upper_roman;
    }
}

/// CSS `list-style-position` 列表标记位置枚举
pub enum ListStylePosition: u8 {
    inside = 0,
    outside = 1,

    static fn fromStr(s: string): ListStylePosition? {
        if (std.mem.eql(u8, s, "inside")) { return .inside; }
        if (std.mem.eql(u8, s, "outside")) { return .outside; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .inside => "inside",
            .outside => "outside",
        };
    }

    fn isInside(): bool {
        return _ == .inside;
    }
}

/// CSS Filter 函数类型枚举 (Filter Effects Level 1)
pub enum FilterKind: u8 {
    blur = 0,
    brightness = 1,
    contrast = 2,
    grayscale = 3,
    hue_rotate = 4,
    invert = 5,
    opacity = 6,
    saturate = 7,
    sepia = 8,
    drop_shadow = 9,
    url = 10,

    static fn fromStr(s: string): FilterKind? {
        if (std.mem.eql(u8, s, "blur")) { return .blur; }
        if (std.mem.eql(u8, s, "brightness")) { return .brightness; }
        if (std.mem.eql(u8, s, "contrast")) { return .contrast; }
        if (std.mem.eql(u8, s, "grayscale")) { return .grayscale; }
        if (std.mem.eql(u8, s, "hue-rotate")) { return .hue_rotate; }
        if (std.mem.eql(u8, s, "invert")) { return .invert; }
        if (std.mem.eql(u8, s, "opacity")) { return .opacity; }
        if (std.mem.eql(u8, s, "saturate")) { return .saturate; }
        if (std.mem.eql(u8, s, "sepia")) { return .sepia; }
        if (std.mem.eql(u8, s, "drop-shadow")) { return .drop_shadow; }
        if (std.mem.eql(u8, s, "url")) { return .url; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .blur => "blur",
            .brightness => "brightness",
            .contrast => "contrast",
            .grayscale => "grayscale",
            .hue_rotate => "hue-rotate",
            .invert => "invert",
            .opacity => "opacity",
            .saturate => "saturate",
            .sepia => "sepia",
            .drop_shadow => "drop-shadow",
            .url => "url",
        };
    }

    /// 是否为纯颜色矩阵类滤镜（非几何模糊/阴影/外部 SVG 引用）
    fn isColorMatrix(): bool {
        return _ != .blur && _ != .drop_shadow && _ != .url;
    }
}

/// CSS Compositing & Blending 混合模式枚举
pub enum BlendMode: u8 {
    normal = 0,
    multiply = 1,
    screen = 2,
    overlay = 3,
    darken = 4,
    lighten = 5,
    color_dodge = 6,
    color_burn = 7,
    hard_light = 8,
    soft_light = 9,
    difference = 10,
    exclusion = 11,
    hue = 12,
    saturation = 13,
    color = 14,
    luminosity = 15,

    static fn fromStr(s: string): BlendMode? {
        if (std.mem.eql(u8, s, "normal")) { return .normal; }
        if (std.mem.eql(u8, s, "multiply")) { return .multiply; }
        if (std.mem.eql(u8, s, "screen")) { return .screen; }
        if (std.mem.eql(u8, s, "overlay")) { return .overlay; }
        if (std.mem.eql(u8, s, "darken")) { return .darken; }
        if (std.mem.eql(u8, s, "lighten")) { return .lighten; }
        if (std.mem.eql(u8, s, "color-dodge")) { return .color_dodge; }
        if (std.mem.eql(u8, s, "color-burn")) { return .color_burn; }
        if (std.mem.eql(u8, s, "hard-light")) { return .hard_light; }
        if (std.mem.eql(u8, s, "soft-light")) { return .soft_light; }
        if (std.mem.eql(u8, s, "difference")) { return .difference; }
        if (std.mem.eql(u8, s, "exclusion")) { return .exclusion; }
        if (std.mem.eql(u8, s, "hue")) { return .hue; }
        if (std.mem.eql(u8, s, "saturation")) { return .saturation; }
        if (std.mem.eql(u8, s, "color")) { return .color; }
        if (std.mem.eql(u8, s, "luminosity")) { return .luminosity; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .normal => "normal",
            .multiply => "multiply",
            .screen => "screen",
            .overlay => "overlay",
            .darken => "darken",
            .lighten => "lighten",
            .color_dodge => "color-dodge",
            .color_burn => "color-burn",
            .hard_light => "hard-light",
            .soft_light => "soft-light",
            .difference => "difference",
            .exclusion => "exclusion",
            .hue => "hue",
            .saturation => "saturation",
            .color => "color",
            .luminosity => "luminosity",
        };
    }

    /// 是否为可分离 RGB 通道混合模式（hue/saturation/color/luminosity 为不可分离 HSL 模式）
    fn isSeparable(): bool {
        return _ != .hue && _ != .saturation && _ != .color && _ != .luminosity;
    }
}

/// CSS 盒阴影/文本阴影渲染特征位标志结构体
pub struct BoxShadowFlags {
    inset: bool = false,
    has_blur: bool = false,
    has_spread: bool = false,
    is_text_shadow: bool = false,

    fn toU8(): u8 {
        let v: u8 = 0;
        if (_.inset) { v |= 1; }
        if (_.has_blur) { v |= 2; }
        if (_.has_spread) { v |= 4; }
        if (_.is_text_shadow) { v |= 8; }
        return v;
    }

    static fn fromU8(v: u8): BoxShadowFlags {
        return BoxShadowFlags{
            inset: (v & 1) != 0,
            has_blur: (v & 2) != 0,
            has_spread: (v & 4) != 0,
            is_text_shadow: (v & 8) != 0,
        };
    }

    /// 是否为简单外投影（非内阴影且无扩散半径）
    fn isSimpleDrop(): bool {
        return !_.inset && !_.has_spread;
    }

    fn eql(other: BoxShadowFlags): bool {
        return _.toU8() == other.toU8();
    }
}
