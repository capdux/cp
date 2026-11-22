// Source: Absorbed from /data/dev/bun/src/css/logical.cp | Synced: 2026-11-22T08:50:00Z
// Ported from Bun: /data/dev/bun/src/css/logical.cp & /data/dev/bun/docs/bundler/css.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation CSS Logical Properties & Values Level 1 classifier (12 logical groups),
// LTR/RTL physical property mapper, shorthand axis splitter, and `:dir(ltr)`/`:dir(rtl)` fallback generator.

/// CSS 逻辑属性分类与降级错误集
pub err CssLogicalError {
    EmptyProperty,
    NotLogicalProperty,
    EmptySelector,
    EmptyValue,
    BufferTooSmall,
}

/// 属性类别：物理属性或逻辑属性（对齐 Bun `PropertyCategory`）
pub enum LogicalPropertyCategory: u8 {
    physical = 0,
    logical = 1,

    static fn default(): LogicalPropertyCategory {
        return LogicalPropertyCategory.physical;
    }
}

/// 逻辑属性分组（完整对齐 Bun `LogicalGroup` 12 组）
pub enum LogicalPropertyGroup: u8 {
    border_color = 0,
    border_style = 1,
    border_width = 2,
    border_radius = 3,
    margin = 4,
    scroll_margin = 5,
    padding = 6,
    scroll_padding = 7,
    inset = 8,
    size = 9,
    min_size = 10,
    max_size = 11,
}

/// 水平书写方向
pub enum WritingDirection: u8 {
    ltr = 0,
    rtl = 1,
}

/// 逻辑属性到物理属性的 LTR/RTL 映射描述
pub struct LogicalPhysicalPair {
    group: LogicalPropertyGroup = LogicalPropertyGroup.margin,
    ltr_prop: []const u8 = "",
    rtl_prop: []const u8 = "",
    is_shorthand_pair: bool = false,
    ltr_second_prop: []const u8 = "",
    rtl_second_prop: []const u8 = "",

    /// 是否依赖书写方向（`inline-start`/`inline-end`、四角逻辑圆角等需区分 LTR/RTL）
    fn isDirectionDependent(): bool {
        return !bytesEql(_.ltr_prop, _.rtl_prop) || !bytesEql(_.ltr_second_prop, _.rtl_second_prop);
    }

    /// 返回指定方向下的主物理属性名
    fn physicalFor(dir: WritingDirection): []const u8 {
        return switch (dir) {
            .ltr => _.ltr_prop,
            .rtl => _.rtl_prop,
        };
    }

    /// 返回指定方向下的第二物理属性名（仅轴简写时非空）
    fn secondPhysicalFor(dir: WritingDirection): []const u8 {
        return switch (dir) {
            .ltr => _.ltr_second_prop,
            .rtl => _.rtl_second_prop,
        };
    }
}

/// 判定属性名属于逻辑属性还是物理属性（大小写不敏感）
pub fn classifyLogicalProperty(prop_name: []const u8): LogicalPropertyCategory {
    if (logicalGroupOf(prop_name) != null) {
        return LogicalPropertyCategory.logical;
    }
    return LogicalPropertyCategory.physical;
}

/// 查询逻辑属性所属的 12 大分组（非逻辑属性返回 `null`）
pub fn logicalGroupOf(prop_name: []const u8): LogicalPropertyGroup? {
    if (prop_name.len == 0) {
        return null;
    }
    if (mapped := mapLogicalToPhysical(prop_name) catch null) {
        return mapped.group;
    }
    return null;
}

/// 将 CSS 逻辑属性映射为 `horizontal-tb` 下的 LTR/RTL 物理属性描述
pub fn mapLogicalToPhysical(prop_name: []const u8): LogicalPhysicalPair! {
    const p = trimWs(prop_name);
    if (p.len == 0) {
        return CssLogicalError.EmptyProperty;
    }

    // 1. Size / Min-Size / Max-Size
    if (eqCi(p, "inline-size")) {
        return single(LogicalPropertyGroup.size, "width", "width");
    }
    if (eqCi(p, "block-size")) {
        return single(LogicalPropertyGroup.size, "height", "height");
    }
    if (eqCi(p, "min-inline-size")) {
        return single(LogicalPropertyGroup.min_size, "min-width", "min-width");
    }
    if (eqCi(p, "min-block-size")) {
        return single(LogicalPropertyGroup.min_size, "min-height", "min-height");
    }
    if (eqCi(p, "max-inline-size")) {
        return single(LogicalPropertyGroup.max_size, "max-width", "max-width");
    }
    if (eqCi(p, "max-block-size")) {
        return single(LogicalPropertyGroup.max_size, "max-height", "max-height");
    }

    // 2. Margin
    if (eqCi(p, "margin-inline-start")) {
        return single(LogicalPropertyGroup.margin, "margin-left", "margin-right");
    }
    if (eqCi(p, "margin-inline-end")) {
        return single(LogicalPropertyGroup.margin, "margin-right", "margin-left");
    }
    if (eqCi(p, "margin-block-start")) {
        return single(LogicalPropertyGroup.margin, "margin-top", "margin-top");
    }
    if (eqCi(p, "margin-block-end")) {
        return single(LogicalPropertyGroup.margin, "margin-bottom", "margin-bottom");
    }
    if (eqCi(p, "margin-inline")) {
        return pair(LogicalPropertyGroup.margin, "margin-left", "margin-right", "margin-right", "margin-left");
    }
    if (eqCi(p, "margin-block")) {
        return pair(LogicalPropertyGroup.margin, "margin-top", "margin-bottom", "margin-top", "margin-bottom");
    }

    // 3. Padding
    if (eqCi(p, "padding-inline-start")) {
        return single(LogicalPropertyGroup.padding, "padding-left", "padding-right");
    }
    if (eqCi(p, "padding-inline-end")) {
        return single(LogicalPropertyGroup.padding, "padding-right", "padding-left");
    }
    if (eqCi(p, "padding-block-start")) {
        return single(LogicalPropertyGroup.padding, "padding-top", "padding-top");
    }
    if (eqCi(p, "padding-block-end")) {
        return single(LogicalPropertyGroup.padding, "padding-bottom", "padding-bottom");
    }
    if (eqCi(p, "padding-inline")) {
        return pair(LogicalPropertyGroup.padding, "padding-left", "padding-right", "padding-right", "padding-left");
    }
    if (eqCi(p, "padding-block")) {
        return pair(LogicalPropertyGroup.padding, "padding-top", "padding-bottom", "padding-top", "padding-bottom");
    }

    // 4. Scroll-Margin
    if (eqCi(p, "scroll-margin-inline-start")) {
        return single(LogicalPropertyGroup.scroll_margin, "scroll-margin-left", "scroll-margin-right");
    }
    if (eqCi(p, "scroll-margin-inline-end")) {
        return single(LogicalPropertyGroup.scroll_margin, "scroll-margin-right", "scroll-margin-left");
    }
    if (eqCi(p, "scroll-margin-block-start")) {
        return single(LogicalPropertyGroup.scroll_margin, "scroll-margin-top", "scroll-margin-top");
    }
    if (eqCi(p, "scroll-margin-block-end")) {
        return single(LogicalPropertyGroup.scroll_margin, "scroll-margin-bottom", "scroll-margin-bottom");
    }
    if (eqCi(p, "scroll-margin-inline")) {
        return pair(LogicalPropertyGroup.scroll_margin, "scroll-margin-left", "scroll-margin-right", "scroll-margin-right", "scroll-margin-left");
    }
    if (eqCi(p, "scroll-margin-block")) {
        return pair(LogicalPropertyGroup.scroll_margin, "scroll-margin-top", "scroll-margin-bottom", "scroll-margin-top", "scroll-margin-bottom");
    }

    // 5. Scroll-Padding
    if (eqCi(p, "scroll-padding-inline-start")) {
        return single(LogicalPropertyGroup.scroll_padding, "scroll-padding-left", "scroll-padding-right");
    }
    if (eqCi(p, "scroll-padding-inline-end")) {
        return single(LogicalPropertyGroup.scroll_padding, "scroll-padding-right", "scroll-padding-left");
    }
    if (eqCi(p, "scroll-padding-block-start")) {
        return single(LogicalPropertyGroup.scroll_padding, "scroll-padding-top", "scroll-padding-top");
    }
    if (eqCi(p, "scroll-padding-block-end")) {
        return single(LogicalPropertyGroup.scroll_padding, "scroll-padding-bottom", "scroll-padding-bottom");
    }
    if (eqCi(p, "scroll-padding-inline")) {
        return pair(LogicalPropertyGroup.scroll_padding, "scroll-padding-left", "scroll-padding-right", "scroll-padding-right", "scroll-padding-left");
    }
    if (eqCi(p, "scroll-padding-block")) {
        return pair(LogicalPropertyGroup.scroll_padding, "scroll-padding-top", "scroll-padding-bottom", "scroll-padding-top", "scroll-padding-bottom");
    }

    // 6. Inset
    if (eqCi(p, "inset-inline-start")) {
        return single(LogicalPropertyGroup.inset, "left", "right");
    }
    if (eqCi(p, "inset-inline-end")) {
        return single(LogicalPropertyGroup.inset, "right", "left");
    }
    if (eqCi(p, "inset-block-start")) {
        return single(LogicalPropertyGroup.inset, "top", "top");
    }
    if (eqCi(p, "inset-block-end")) {
        return single(LogicalPropertyGroup.inset, "bottom", "bottom");
    }
    if (eqCi(p, "inset-inline")) {
        return pair(LogicalPropertyGroup.inset, "left", "right", "right", "left");
    }
    if (eqCi(p, "inset-block")) {
        return pair(LogicalPropertyGroup.inset, "top", "bottom", "top", "bottom");
    }

    // 7. Border-Radius (logical corners)
    if (eqCi(p, "border-start-start-radius")) {
        return single(LogicalPropertyGroup.border_radius, "border-top-left-radius", "border-top-right-radius");
    }
    if (eqCi(p, "border-start-end-radius")) {
        return single(LogicalPropertyGroup.border_radius, "border-top-right-radius", "border-top-left-radius");
    }
    if (eqCi(p, "border-end-start-radius")) {
        return single(LogicalPropertyGroup.border_radius, "border-bottom-left-radius", "border-bottom-right-radius");
    }
    if (eqCi(p, "border-end-end-radius")) {
        return single(LogicalPropertyGroup.border_radius, "border-bottom-right-radius", "border-bottom-left-radius");
    }

    // 8. Border-Color
    if (eqCi(p, "border-inline-start-color")) {
        return single(LogicalPropertyGroup.border_color, "border-left-color", "border-right-color");
    }
    if (eqCi(p, "border-inline-end-color")) {
        return single(LogicalPropertyGroup.border_color, "border-right-color", "border-left-color");
    }
    if (eqCi(p, "border-block-start-color")) {
        return single(LogicalPropertyGroup.border_color, "border-top-color", "border-top-color");
    }
    if (eqCi(p, "border-block-end-color")) {
        return single(LogicalPropertyGroup.border_color, "border-bottom-color", "border-bottom-color");
    }
    if (eqCi(p, "border-inline-color")) {
        return pair(LogicalPropertyGroup.border_color, "border-left-color", "border-right-color", "border-right-color", "border-left-color");
    }
    if (eqCi(p, "border-block-color")) {
        return pair(LogicalPropertyGroup.border_color, "border-top-color", "border-bottom-color", "border-top-color", "border-bottom-color");
    }

    // 9. Border-Style
    if (eqCi(p, "border-inline-start-style")) {
        return single(LogicalPropertyGroup.border_style, "border-left-style", "border-right-style");
    }
    if (eqCi(p, "border-inline-end-style")) {
        return single(LogicalPropertyGroup.border_style, "border-right-style", "border-left-style");
    }
    if (eqCi(p, "border-block-start-style")) {
        return single(LogicalPropertyGroup.border_style, "border-top-style", "border-top-style");
    }
    if (eqCi(p, "border-block-end-style")) {
        return single(LogicalPropertyGroup.border_style, "border-bottom-style", "border-bottom-style");
    }
    if (eqCi(p, "border-inline-style")) {
        return pair(LogicalPropertyGroup.border_style, "border-left-style", "border-right-style", "border-right-style", "border-left-style");
    }
    if (eqCi(p, "border-block-style")) {
        return pair(LogicalPropertyGroup.border_style, "border-top-style", "border-bottom-style", "border-top-style", "border-bottom-style");
    }

    // 10. Border-Width
    if (eqCi(p, "border-inline-start-width")) {
        return single(LogicalPropertyGroup.border_width, "border-left-width", "border-right-width");
    }
    if (eqCi(p, "border-inline-end-width")) {
        return single(LogicalPropertyGroup.border_width, "border-right-width", "border-left-width");
    }
    if (eqCi(p, "border-block-start-width")) {
        return single(LogicalPropertyGroup.border_width, "border-top-width", "border-top-width");
    }
    if (eqCi(p, "border-block-end-width")) {
        return single(LogicalPropertyGroup.border_width, "border-bottom-width", "border-bottom-width");
    }
    if (eqCi(p, "border-inline-width")) {
        return pair(LogicalPropertyGroup.border_width, "border-left-width", "border-right-width", "border-right-width", "border-left-width");
    }
    if (eqCi(p, "border-block-width")) {
        return pair(LogicalPropertyGroup.border_width, "border-top-width", "border-bottom-width", "border-top-width", "border-bottom-width");
    }

    return CssLogicalError.NotLogicalProperty;
}

/// 将单条 CSS 逻辑声明降级为物理属性规则（支持轴简写单值/双值拆分与 `:dir(ltr)`/`:dir(rtl)` 双分支输出）
pub fn formatLogicalFallbackRule(
    selector: []const u8,
    prop_name: []const u8,
    value: []const u8,
    out: []u8,
): usize! {
    const sel = trimWs(selector);
    if (sel.len == 0) {
        return CssLogicalError.EmptySelector;
    }
    const val = trimWs(value);
    if (val.len == 0) {
        return CssLogicalError.EmptyValue;
    }

    const mapped = mapLogicalToPhysical(prop_name)?;
    let pos: usize = 0;

    if (!mapped.is_shorthand_pair) {
        if (!mapped.isDirectionDependent()) {
            pos = writeRuleOneDecl(out, pos, sel, mapped.ltr_prop, val)?;
            return pos;
        }
        pos = writeDirRuleOneDecl(out, pos, ":dir(ltr) ", sel, mapped.ltr_prop, val)?;
        pos = appendByte(out, pos, ' ')?;
        pos = writeDirRuleOneDecl(out, pos, ":dir(rtl) ", sel, mapped.rtl_prop, val)?;
        return pos;
    }

    // 轴简写拆分（支持单值如 `16px` 或双值如 `8px 16px`）
    const split = splitTwoTopTokens(val);
    const v1 = split.first;
    const v2 = split.second;

    if (!mapped.isDirectionDependent() || bytesEql(v1, v2)) {
        // 方向无关（block 轴）或单值/对称双值（如 `margin-inline: 16px`），直接单块输出
        pos = writeRuleTwoDecls(out, pos, sel, mapped.ltr_prop, v1, mapped.ltr_second_prop, v2)?;
        return pos;
    }

    // 非对称 inline 轴双值（如 `margin-inline: 8px 16px`）：LTR 为 left=8px right=16px，RTL 为 right=8px left=16px
    pos = writeDirRuleTwoDecls(out, pos, ":dir(ltr) ", sel, mapped.ltr_prop, v1, mapped.ltr_second_prop, v2)?;
    pos = appendByte(out, pos, ' ')?;
    pos = writeDirRuleTwoDecls(out, pos, ":dir(rtl) ", sel, mapped.rtl_prop, v1, mapped.rtl_second_prop, v2)?;
    return pos;
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

fn single(group: LogicalPropertyGroup, ltr: []const u8, rtl: []const u8): LogicalPhysicalPair {
    return LogicalPhysicalPair{
        group: group,
        ltr_prop: ltr,
        rtl_prop: rtl,
        is_shorthand_pair: false,
    };
}

fn pair(
    group: LogicalPropertyGroup,
    ltr1: []const u8,
    ltr2: []const u8,
    rtl1: []const u8,
    rtl2: []const u8,
): LogicalPhysicalPair {
    return LogicalPhysicalPair{
        group: group,
        ltr_prop: ltr1,
        rtl_prop: rtl1,
        is_shorthand_pair: true,
        ltr_second_prop: ltr2,
        rtl_second_prop: rtl2,
    };
}

struct TwoTokens {
    first: []const u8 = "",
    second: []const u8 = "",
}

fn splitTwoTopTokens(val: []const u8): TwoTokens {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c == '(') {
            depth += 1;
        } else if (c == ')' && depth > 0) {
            depth -= 1;
        } else if (depth == 0 && isWs(c)) {
            const first = val[0..i];
            let j = i;
            while (j < val.len && isWs(val[j])) {
                j += 1;
            }
            if (j < val.len) {
                return TwoTokens{ first: first, second: trimWs(val[j..]) };
            }
        }
        i += 1;
    }
    return TwoTokens{ first: val, second: val };
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqCi(a: []const u8, b: []const u8): bool {
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

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return CssLogicalError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return CssLogicalError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeRuleOneDecl(out: []u8, pos_: usize, sel: []const u8, prop: []const u8, val: []const u8): usize! {
    let pos = pos_;
    pos = appendBytes(out, pos, sel)?;
    pos = appendBytes(out, pos, " { ")?;
    pos = appendBytes(out, pos, prop)?;
    pos = appendBytes(out, pos, ": ")?;
    pos = appendBytes(out, pos, val)?;
    pos = appendBytes(out, pos, "; }")?;
    return pos;
}

fn writeDirRuleOneDecl(out: []u8, pos_: usize, dir_prefix: []const u8, sel: []const u8, prop: []const u8, val: []const u8): usize! {
    let pos = pos_;
    pos = appendBytes(out, pos, dir_prefix)?;
    return writeRuleOneDecl(out, pos, sel, prop, val);
}

fn writeRuleTwoDecls(
    out: []u8,
    pos_: usize,
    sel: []const u8,
    p1: []const u8,
    v1: []const u8,
    p2: []const u8,
    v2: []const u8,
): usize! {
    let pos = pos_;
    pos = appendBytes(out, pos, sel)?;
    pos = appendBytes(out, pos, " { ")?;
    pos = appendBytes(out, pos, p1)?;
    pos = appendBytes(out, pos, ": ")?;
    pos = appendBytes(out, pos, v1)?;
    pos = appendBytes(out, pos, "; ")?;
    pos = appendBytes(out, pos, p2)?;
    pos = appendBytes(out, pos, ": ")?;
    pos = appendBytes(out, pos, v2)?;
    pos = appendBytes(out, pos, "; }")?;
    return pos;
}

fn writeDirRuleTwoDecls(
    out: []u8,
    pos_: usize,
    dir_prefix: []const u8,
    sel: []const u8,
    p1: []const u8,
    v1: []const u8,
    p2: []const u8,
    v2: []const u8,
): usize! {
    let pos = pos_;
    pos = appendBytes(out, pos, dir_prefix)?;
    return writeRuleTwoDecls(out, pos, sel, p1, v1, p2, v2);
}
