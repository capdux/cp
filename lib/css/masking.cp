// Source: Absorbed from /data/dev/bun/src/css/properties/masking.cp | Synced: 2026-11-22T04:57:22Z

/// CSS `<geometry-box>`（CSS Masking Level 1）
pub enum GeometryBox: u8 {
    border_box = 0,
    padding_box = 1,
    content_box = 2,
    margin_box = 3,
    fill_box = 4,
    stroke_box = 5,
    view_box = 6,

    static fn default(): GeometryBox {
        return .border_box;
    }

    static fn parse(input: string): GeometryBox? {
        if (std.ascii.eqlIgnoreCase(input, "border-box")) { return .border_box; }
        if (std.ascii.eqlIgnoreCase(input, "padding-box")) { return .padding_box; }
        if (std.ascii.eqlIgnoreCase(input, "content-box")) { return .content_box; }
        if (std.ascii.eqlIgnoreCase(input, "margin-box")) { return .margin_box; }
        if (std.ascii.eqlIgnoreCase(input, "fill-box")) { return .fill_box; }
        if (std.ascii.eqlIgnoreCase(input, "stroke-box")) { return .stroke_box; }
        if (std.ascii.eqlIgnoreCase(input, "view-box")) { return .view_box; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .border_box => "border-box",
            .padding_box => "padding-box",
            .content_box => "content-box",
            .margin_box => "margin-box",
            .fill_box => "fill-box",
            .stroke_box => "stroke-box",
            .view_box => "view-box",
        };
    }

    fn isSvgBox(): bool {
        return switch (_) {
            .fill_box, .stroke_box, .view_box => true,
            else => false,
        };
    }

    fn isCssBox(): bool {
        return !_.isSvgBox();
    }

    fn intoMaskClip(): MaskClip {
        return switch (_) {
            .border_box => MaskClip.border_box,
            .padding_box => MaskClip.padding_box,
            .content_box => MaskClip.content_box,
            .margin_box => MaskClip.margin_box,
            .fill_box => MaskClip.fill_box,
            .stroke_box => MaskClip.stroke_box,
            .view_box => MaskClip.view_box,
        };
    }
}

/// CSS `mask-origin`（`<coord-box>`，不含 `margin-box`）
pub enum MaskOrigin: u8 {
    border_box = 0,
    padding_box = 1,
    content_box = 2,
    fill_box = 3,
    stroke_box = 4,
    view_box = 5,

    static fn default(): MaskOrigin {
        return .border_box;
    }

    static fn parse(input: string): MaskOrigin? {
        if (std.ascii.eqlIgnoreCase(input, "border-box")) { return .border_box; }
        if (std.ascii.eqlIgnoreCase(input, "padding-box")) { return .padding_box; }
        if (std.ascii.eqlIgnoreCase(input, "content-box")) { return .content_box; }
        if (std.ascii.eqlIgnoreCase(input, "fill-box")) { return .fill_box; }
        if (std.ascii.eqlIgnoreCase(input, "stroke-box")) { return .stroke_box; }
        if (std.ascii.eqlIgnoreCase(input, "view-box")) { return .view_box; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .border_box => "border-box",
            .padding_box => "padding-box",
            .content_box => "content-box",
            .fill_box => "fill-box",
            .stroke_box => "stroke-box",
            .view_box => "view-box",
        };
    }

    fn toGeometryBox(): GeometryBox {
        return switch (_) {
            .border_box => GeometryBox.border_box,
            .padding_box => GeometryBox.padding_box,
            .content_box => GeometryBox.content_box,
            .fill_box => GeometryBox.fill_box,
            .stroke_box => GeometryBox.stroke_box,
            .view_box => GeometryBox.view_box,
        };
    }
}

/// CSS `mask-clip`（`<geometry-box> | no-clip`）
pub enum MaskClip: u8 {
    border_box = 0,
    padding_box = 1,
    content_box = 2,
    margin_box = 3,
    fill_box = 4,
    stroke_box = 5,
    view_box = 6,
    no_clip = 7,

    static fn default(): MaskClip {
        return .border_box;
    }

    static fn parse(input: string): MaskClip? {
        if (std.ascii.eqlIgnoreCase(input, "no-clip")) {
            return .no_clip;
        }
        if (g := GeometryBox.parse(input)) {
            return g.intoMaskClip();
        }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .border_box => "border-box",
            .padding_box => "padding-box",
            .content_box => "content-box",
            .margin_box => "margin-box",
            .fill_box => "fill-box",
            .stroke_box => "stroke-box",
            .view_box => "view-box",
            .no_clip => "no-clip",
        };
    }

    fn isClipped(): bool {
        return _ != .no_clip;
    }
}

/// CSS `mask-mode`（`luminance | alpha | match-source`）
pub enum MaskMode: u8 {
    luminance = 0,
    alpha = 1,
    match_source = 2,

    static fn default(): MaskMode {
        return .match_source;
    }

    static fn parse(input: string): MaskMode? {
        if (std.ascii.eqlIgnoreCase(input, "luminance")) { return .luminance; }
        if (std.ascii.eqlIgnoreCase(input, "alpha")) { return .alpha; }
        if (std.ascii.eqlIgnoreCase(input, "match-source")) { return .match_source; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .luminance => "luminance",
            .alpha => "alpha",
            .match_source => "match-source",
        };
    }

    fn toWebKitSourceType(): WebKitMaskSourceType {
        return switch (_) {
            .luminance => WebKitMaskSourceType.luminance,
            .alpha => WebKitMaskSourceType.alpha,
            .match_source => WebKitMaskSourceType.auto,
        };
    }
}

/// CSS `mask-type`（`luminance | alpha`，初始值 `luminance`）
pub enum MaskType: u8 {
    luminance = 0,
    alpha = 1,

    static fn default(): MaskType {
        return .luminance;
    }

    static fn parse(input: string): MaskType? {
        if (std.ascii.eqlIgnoreCase(input, "luminance")) { return .luminance; }
        if (std.ascii.eqlIgnoreCase(input, "alpha")) { return .alpha; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .luminance => "luminance",
            .alpha => "alpha",
        };
    }
}

/// CSS `mask-composite`（`add | subtract | intersect | exclude`）
pub enum MaskComposite: u8 {
    add = 0,
    subtract = 1,
    intersect = 2,
    exclude = 3,

    static fn default(): MaskComposite {
        return .add;
    }

    static fn parse(input: string): MaskComposite? {
        if (std.ascii.eqlIgnoreCase(input, "add")) { return .add; }
        if (std.ascii.eqlIgnoreCase(input, "subtract")) { return .subtract; }
        if (std.ascii.eqlIgnoreCase(input, "intersect")) { return .intersect; }
        if (std.ascii.eqlIgnoreCase(input, "exclude")) { return .exclude; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .add => "add",
            .subtract => "subtract",
            .intersect => "intersect",
            .exclude => "exclude",
        };
    }

    fn toWebKit(): WebKitMaskComposite {
        return switch (_) {
            .add => WebKitMaskComposite.source_over,
            .subtract => WebKitMaskComposite.source_out,
            .intersect => WebKitMaskComposite.source_in,
            .exclude => WebKitMaskComposite.xor,
        };
    }
}

/// CSS `-webkit-mask-composite`（WebKit Porter-Duff 合成算子）
pub enum WebKitMaskComposite: u8 {
    clear = 0,
    copy = 1,
    source_over = 2,
    source_in = 3,
    source_out = 4,
    source_atop = 5,
    destination_over = 6,
    destination_in = 7,
    destination_out = 8,
    destination_atop = 9,
    xor = 10,

    static fn parse(input: string): WebKitMaskComposite? {
        if (std.ascii.eqlIgnoreCase(input, "clear")) { return .clear; }
        if (std.ascii.eqlIgnoreCase(input, "copy")) { return .copy; }
        if (std.ascii.eqlIgnoreCase(input, "source-over")) { return .source_over; }
        if (std.ascii.eqlIgnoreCase(input, "source-in")) { return .source_in; }
        if (std.ascii.eqlIgnoreCase(input, "source-out")) { return .source_out; }
        if (std.ascii.eqlIgnoreCase(input, "source-atop")) { return .source_atop; }
        if (std.ascii.eqlIgnoreCase(input, "destination-over")) { return .destination_over; }
        if (std.ascii.eqlIgnoreCase(input, "destination-in")) { return .destination_in; }
        if (std.ascii.eqlIgnoreCase(input, "destination-out")) { return .destination_out; }
        if (std.ascii.eqlIgnoreCase(input, "destination-atop")) { return .destination_atop; }
        if (std.ascii.eqlIgnoreCase(input, "xor")) { return .xor; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .clear => "clear",
            .copy => "copy",
            .source_over => "source-over",
            .source_in => "source-in",
            .source_out => "source-out",
            .source_atop => "source-atop",
            .destination_over => "destination-over",
            .destination_in => "destination-in",
            .destination_out => "destination-out",
            .destination_atop => "destination-atop",
            .xor => "xor",
        };
    }

    fn toStandard(): MaskComposite? {
        return switch (_) {
            .source_over => MaskComposite.add,
            .source_in => MaskComposite.intersect,
            .source_out => MaskComposite.subtract,
            .xor => MaskComposite.exclude,
            else => null,
        };
    }
}

/// CSS `-webkit-mask-source-type`（`auto | luminance | alpha`）
pub enum WebKitMaskSourceType: u8 {
    auto = 0,
    luminance = 1,
    alpha = 2,

    static fn parse(input: string): WebKitMaskSourceType? {
        if (std.ascii.eqlIgnoreCase(input, "auto")) { return .auto; }
        if (std.ascii.eqlIgnoreCase(input, "luminance")) { return .luminance; }
        if (std.ascii.eqlIgnoreCase(input, "alpha")) { return .alpha; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .auto => "auto",
            .luminance => "luminance",
            .alpha => "alpha",
        };
    }

    fn toStandard(): MaskMode {
        return switch (_) {
            .auto => MaskMode.match_source,
            .luminance => MaskMode.luminance,
            .alpha => MaskMode.alpha,
        };
    }
}

/// CSS `mask-border-mode`（`luminance | alpha`，初始值 `alpha`）
pub enum MaskBorderMode: u8 {
    luminance = 0,
    alpha = 1,

    static fn default(): MaskBorderMode {
        return .alpha;
    }

    static fn parse(input: string): MaskBorderMode? {
        if (std.ascii.eqlIgnoreCase(input, "luminance")) { return .luminance; }
        if (std.ascii.eqlIgnoreCase(input, "alpha")) { return .alpha; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .luminance => "luminance",
            .alpha => "alpha",
        };
    }
}

/// CSS `mask-border-repeat`（`stretch | repeat | round | space`）
pub enum MaskBorderRepeat: u8 {
    stretch = 0,
    repeat = 1,
    round = 2,
    space = 3,

    static fn default(): MaskBorderRepeat {
        return .stretch;
    }

    static fn parse(input: string): MaskBorderRepeat? {
        if (std.ascii.eqlIgnoreCase(input, "stretch")) { return .stretch; }
        if (std.ascii.eqlIgnoreCase(input, "repeat")) { return .repeat; }
        if (std.ascii.eqlIgnoreCase(input, "round")) { return .round; }
        if (std.ascii.eqlIgnoreCase(input, "space")) { return .space; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .stretch => "stretch",
            .repeat => "repeat",
            .round => "round",
            .space => "space",
        };
    }
}

/// CSS `polygon()` 填充规则（`nonzero | evenodd`）
pub enum FillRule: u8 {
    nonzero = 0,
    evenodd = 1,

    static fn default(): FillRule {
        return .nonzero;
    }

    static fn parse(input: string): FillRule? {
        if (std.ascii.eqlIgnoreCase(input, "nonzero")) { return .nonzero; }
        if (std.ascii.eqlIgnoreCase(input, "evenodd")) { return .evenodd; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .nonzero => "nonzero",
            .evenodd => "evenodd",
        };
    }
}

/// 遮罩特性位标志常量
pub const MASK_FLAG_IMAGE: u16 = 1 << 0;
pub const MASK_FLAG_MODE: u16 = 1 << 1;
pub const MASK_FLAG_ORIGIN: u16 = 1 << 2;
pub const MASK_FLAG_CLIP: u16 = 1 << 3;
pub const MASK_FLAG_COMPOSITE: u16 = 1 << 4;
pub const MASK_FLAG_BORDER: u16 = 1 << 5;
pub const MASK_FLAG_CLIP_PATH: u16 = 1 << 6;
pub const MASK_FLAG_WEBKIT_PREFIX: u16 = 1 << 7;

/// CSS 遮罩声明特性位标志集合
pub struct MaskFeatureFlags {
    bits: u16 = 0,

    static fn empty(): MaskFeatureFlags {
        return MaskFeatureFlags{ bits: 0 };
    }

    fn isEmpty(): bool {
        return _.bits == 0;
    }

    fn has(flag: u16): bool {
        return (_.bits & flag) == flag;
    }

    fn add(*_, flag: u16) {
        _.bits |= flag;
    }

    fn remove(*_, flag: u16) {
        _.bits &= ~flag;
    }

    fn needsWebKitFallback(): bool {
        return (_.bits & (MASK_FLAG_IMAGE | MASK_FLAG_MODE | MASK_FLAG_CLIP | MASK_FLAG_COMPOSITE | MASK_FLAG_BORDER | MASK_FLAG_CLIP_PATH)) != 0;
    }
}

/// 聚合的 CSS 遮罩属性配置
pub struct MaskConfig {
    mode: MaskMode = MaskMode.match_source,
    origin: MaskOrigin = MaskOrigin.border_box,
    clip: MaskClip = MaskClip.border_box,
    composite: MaskComposite = MaskComposite.add,
    mask_type: MaskType = MaskType.luminance,

    static fn default(): MaskConfig {
        return MaskConfig{};
    }

    fn isDefault(): bool {
        return _.mode == MaskMode.match_source
            && _.origin == MaskOrigin.border_box
            && _.clip == MaskClip.border_box
            && _.composite == MaskComposite.add
            && _.mask_type == MaskType.luminance;
    }
}

/// 聚合的 CSS `mask-border` 属性配置
pub struct MaskBorderConfig {
    mode: MaskBorderMode = MaskBorderMode.alpha,
    repeat_x: MaskBorderRepeat = MaskBorderRepeat.stretch,
    repeat_y: MaskBorderRepeat = MaskBorderRepeat.stretch,

    static fn default(): MaskBorderConfig {
        return MaskBorderConfig{};
    }

    fn isDefault(): bool {
        return _.mode == MaskBorderMode.alpha
            && _.repeat_x == MaskBorderRepeat.stretch
            && _.repeat_y == MaskBorderRepeat.stretch;
    }
}

/// 将标准 CSS Masking 属性名映射为 `-webkit-` 兼容前缀属性名（不支持的属性返回 `null`）。
pub fn getWebKitMaskPropertyName(standard_prop: string): string? {
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border-source")) { return "-webkit-mask-box-image-source"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border-slice")) { return "-webkit-mask-box-image-slice"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border-width")) { return "-webkit-mask-box-image-width"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border-outset")) { return "-webkit-mask-box-image-outset"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border-repeat")) { return "-webkit-mask-box-image-repeat"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-border")) { return "-webkit-mask-box-image"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-composite")) { return "-webkit-mask-composite"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-mode")) { return "-webkit-mask-source-type"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-image")) { return "-webkit-mask-image"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-clip")) { return "-webkit-mask-clip"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask-origin")) { return "-webkit-mask-origin"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "mask")) { return "-webkit-mask"; }
    if (std.ascii.eqlIgnoreCase(standard_prop, "clip-path")) { return "-webkit-clip-path"; }
    return null;
}
