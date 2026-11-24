// Source: Absorbed from /data/dev/bun/src/css/properties/prefix_handler.cp | Synced: 2026-11-23T21:35:26Z
// Pure-CP zero-allocation CSS Property Fallback Handler (LightningCSS / Bun `FallbackHandler`).
// Manages progressive-enhancement fallback emission & declaration deduplication within a rule block:
// - Emits compatible fallback declarations before modern values (e.g. `oklch`/`oklab`/`lab`/`lch`/`color-mix`/`display-p3`)
// - Preserves manual fallback chains when a subsequent value is incompatible with target browsers
// - Overwrites previous slot in-place when a subsequent value is compatible or unparsed (`var(...)`)
// - Resets per-property slot indices via `finalize()` at rule block boundaries

/// 单个规则块内支持的最大输出声明数（含自动注入的 fallback）
pub const CSS_FALLBACK_MAX_DECLS: usize = 64;
/// 单条属性支持的最大候选 fallback 数
pub const CSS_FALLBACK_MAX_PER_PROP: usize = 4;

/// CSS Fallback 处理器错误集
pub err CssFallbackError {
    EmptyValue,
    UnsupportedProperty,
    CapacityExceeded,
    BufferTooSmall,
}

/// 受 `CssFallbackHandler` 管理的颜色/视觉属性类别（对齐 Bun `FallbackHandler` 字段集）
pub enum CssFallbackProp: u8 {
    Color = 0,
    TextShadow = 1,
    BackgroundColor = 2,
    BorderColor = 3,
    Fill = 4,
    Stroke = 5,
    CaretColor = 6,

    /// 按 ASCII 大小写无关解析属性名
    static fn parse(raw: []const u8): CssFallbackProp? {
        if (eqlAsciiIgnoreCase(raw, "color")) return .Color;
        if (eqlAsciiIgnoreCase(raw, "text-shadow")) return .TextShadow;
        if (eqlAsciiIgnoreCase(raw, "background-color")) return .BackgroundColor;
        if (eqlAsciiIgnoreCase(raw, "border-color")) return .BorderColor;
        if (eqlAsciiIgnoreCase(raw, "fill")) return .Fill;
        if (eqlAsciiIgnoreCase(raw, "stroke")) return .Stroke;
        if (eqlAsciiIgnoreCase(raw, "caret-color")) return .CaretColor;
        return null;
    }

    /// 返回规范小写属性名
    fn asStr(): []const u8 {
        return switch (_) {
            .Color => "color",
            .TextShadow => "text-shadow",
            .BackgroundColor => "background-color",
            .BorderColor => "border-color",
            .Fill => "fill",
            .Stroke => "stroke",
            .CaretColor => "caret-color",
        };
    }

    fn slotIndex(): usize {
        return switch (_) {
            .Color => 0,
            .TextShadow => 1,
            .BackgroundColor => 2,
            .BorderColor => 3,
            .Fill => 4,
            .Stroke => 5,
            .CaretColor => 6,
        };
    }
}

/// 现代 CSS 颜色特性兼容位掩码（目标浏览器支持集合）
pub const CSS_COMPAT_LAB_COLORS: u16 = 1 << 0;
pub const CSS_COMPAT_OKLAB_COLORS: u16 = 1 << 1;
pub const CSS_COMPAT_P3_COLORS: u16 = 1 << 2;
pub const CSS_COMPAT_COLOR_MIX: u16 = 1 << 3;
pub const CSS_COMPAT_LIGHT_DARK: u16 = 1 << 4;
pub const CSS_COMPAT_ALL_MODERN: u16 =
    CSS_COMPAT_LAB_COLORS | CSS_COMPAT_OKLAB_COLORS | CSS_COMPAT_P3_COLORS | CSS_COMPAT_COLOR_MIX | CSS_COMPAT_LIGHT_DARK;

/// 输出列表中的单条声明
pub struct CssFallbackDecl {
    prop: CssFallbackProp = .Color,
    value: []const u8 = "",
    is_fallback: bool = false,
    is_unparsed: bool = false,
}

/// 零堆分配 CSS 声明输出列表
pub struct CssFallbackDeclList {
    items: [CSS_FALLBACK_MAX_DECLS]CssFallbackDecl = [CssFallbackDecl{}; CSS_FALLBACK_MAX_DECLS],
    count: usize = 0,

    static fn init(): CssFallbackDeclList {
        return CssFallbackDeclList{};
    }

    fn push(*_, decl: CssFallbackDecl): usize! {
        if (_.count >= CSS_FALLBACK_MAX_DECLS) return err.CapacityExceeded;
        _.items[_.count] = decl;
        _.count += 1;
        return _.count;
    }

    fn slice(): []const CssFallbackDecl {
        return _.items[0.._.count];
    }

    /// 将声明列表序列化为 CSS 声明块正文（支持压缩/美化模式）
    fn formatCss(out: []u8, minify: bool): usize! {
        let pos: usize = 0;
        for (i of 0.._.count) {
            const d = _.items[i];
            pos = copyInto(out, pos, d.prop.asStr())?;
            pos = copyInto(out, pos, if (minify) ":" else ": ")?;
            pos = copyInto(out, pos, d.value)?;
            const is_last = (i + 1 == _.count);
            if (!is_last) {
                pos = copyInto(out, pos, if (minify) ";" else "; ")?;
            } else if (!minify) {
                pos = copyInto(out, pos, ";")?;
            }
        }
        return pos;
    }
}

/// 检测 CSS 值是否使用了现代色域/函数，返回所需浏览器特性位掩码
pub fn detectRequiredColorFeatures(value: []const u8): u16 {
    let mask: u16 = 0;
    if (containsAsciiIgnoreCase(value, "oklab(") || containsAsciiIgnoreCase(value, "oklch(")) {
        mask |= CSS_COMPAT_OKLAB_COLORS;
    }
    if (containsAsciiIgnoreCase(value, "lab(") || containsAsciiIgnoreCase(value, "lch(")) {
        mask |= CSS_COMPAT_LAB_COLORS;
    }
    if (containsAsciiIgnoreCase(value, "display-p3")) {
        mask |= CSS_COMPAT_P3_COLORS;
    }
    if (containsAsciiIgnoreCase(value, "color-mix(")) {
        mask |= CSS_COMPAT_COLOR_MIX;
    }
    if (containsAsciiIgnoreCase(value, "light-dark(")) {
        mask |= CSS_COMPAT_LIGHT_DARK;
    }
    return mask;
}

/// 判断给定 CSS 值在 `supported_features` 目标浏览器下是否原生兼容
pub fn isCssValueCompatible(value: []const u8, supported_features: u16): bool {
    const req = detectRequiredColorFeatures(value);
    return (req & supported_features) == req;
}

/// 判断给定 CSS 值是否包含 `var(...)` 动态变量（需走 UnparsedProperty 路径）
pub fn containsCssVarRef(value: []const u8): bool {
    return containsAsciiIgnoreCase(value, "var(");
}

/// 零堆分配 CSS Fallback 降级处理器（对齐 Bun `css/properties/prefix_handler.cp` `FallbackHandler`）
pub struct CssFallbackHandler {
    slots: [7]usize? = [null; 7],
    supported_features: u16 = 0,
    unparsed_fallback_count: usize = 0,

    static fn init(supported_features: u16): CssFallbackHandler {
        return CssFallbackHandler{
            supported_features: supported_features,
        };
    }

    /// 处理一条已解析属性声明，按目标浏览器兼容性决定是否前置注入 `fallbacks` 或原地覆盖旧槽位
    fn handleProperty(
        *_,
        prop: CssFallbackProp,
        value: []const u8,
        fallbacks: []const ([]const u8),
        dest: *CssFallbackDeclList,
    ): bool! {
        if (value.len == 0) return err.EmptyValue;

        const slot_idx = prop.slotIndex();
        const compatible = isCssValueCompatible(value, _.supported_features);

        // 首次出现该属性且目标浏览器不兼容时，前置注入自动降级声明
        if (_.slots[slot_idx] == null && !compatible) {
            for (i of 0..fallbacks.len) {
                if (fallbacks[i].len == 0) continue;
                * = dest.push(CssFallbackDecl{
                    prop: prop,
                    value: fallbacks[i],
                    is_fallback: true,
                    is_unparsed: false,
                })?;
            }
        }

        // 若尚未有槽位，或新值不兼容目标浏览器（保留前一条 fallback 形成渐进增强链），追加新条目
        if (_.slots[slot_idx] == null || !compatible) {
            const new_pos = dest.count;
            * = dest.push(CssFallbackDecl{
                prop: prop,
                value: value,
                is_fallback: false,
                is_unparsed: false,
            })?;
            _.slots[slot_idx] = new_pos;
        } else {
            // 新值兼容且已有槽位：原地覆盖旧槽位以去重
            const existing = _.slots[slot_idx] ?? 0;
            dest.items[existing] = CssFallbackDecl{
                prop: prop,
                value: value,
                is_fallback: false,
                is_unparsed: false,
            };
        }

        return true;
    }

    /// 处理含 `var(...)` 的未解析属性声明（对齐 Bun `UnparsedProperty` 分支）
    fn handleUnparsed(
        *_,
        prop: CssFallbackProp,
        raw_value: []const u8,
        dest: *CssFallbackDeclList,
    ): bool! {
        if (raw_value.len == 0) return err.EmptyValue;
        const slot_idx = prop.slotIndex();
        _.unparsed_fallback_count += 1;

        if (existing := _.slots[slot_idx]) {
            dest.items[existing] = CssFallbackDecl{
                prop: prop,
                value: raw_value,
                is_fallback: false,
                is_unparsed: true,
            };
        } else {
            const new_pos = dest.count;
            * = dest.push(CssFallbackDecl{
                prop: prop,
                value: raw_value,
                is_fallback: false,
                is_unparsed: true,
            })?;
            _.slots[slot_idx] = new_pos;
        }

        return true;
    }

    /// 结束当前规则块并清空所有属性槽位状态（对齐 Bun `FallbackHandler.finalize()`）
    fn finalize(*_) {
        for (i of 0..7) {
            _.slots[i] = null;
        }
    }
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn containsAsciiIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let matched = true;
        for (j of 0..needle.len) {
            if (toLowerAscii(haystack[i + j]) != toLowerAscii(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
        i += 1;
    }
    return false;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}
