// Source: Absorbed from /data/dev/bun/src/css/properties/margin_padding.cp | Synced: 2026-11-22T14:37:12Z
// Pure-CP zero-allocation CSS Box Spacing (`margin` / `padding` / `inset` / `scroll-margin` / `scroll-padding`)
// physical & logical shorthand parser, serializer, and browser-target lowering handler
// (aligned with LightningCSS & Bun `NewSizeHandler` + `docs/bundler/css.mdx`).

/// 单个输出声明表支持的最大普通物理/逻辑声明条数
pub const CSS_MARGIN_PAD_MAX_DECLS: usize = 32;
/// 单个输出声明表支持的最大方向性（LTR / RTL）降级声明条数
pub const CSS_MARGIN_PAD_MAX_DIR_DECLS: usize = 16;
/// 单条声明值内联缓冲区字节上限（零堆分配）
pub const CSS_MARGIN_PAD_MAX_VAL_BYTES: usize = 160;

/// CSS 盒模型间距属性解析与降级错误集
pub err CssMarginPadError {
    EmptyValue,
    InvalidProperty,
    InvalidValue,
    TooManyValues,
    UnclosedDelimiter,
    OutputFull,
    BufferTooSmall,
}

/// 5 大 CSS 盒模型物理/逻辑间距属性家族（对齐 Bun `MarginHandler`..`InsetHandler`）
pub enum CssBoxSpacingFamily: u8 {
    margin = 0,
    padding = 1,
    inset = 2,
    scroll_margin = 3,
    scroll_padding = 4,

    /// 四边简写属性名
    fn shorthandName(): []const u8 {
        return switch (_) {
            .margin => "margin",
            .padding => "padding",
            .inset => "inset",
            .scroll_margin => "scroll-margin",
            .scroll_padding => "scroll-padding",
        };
    }

    fn topName(): []const u8 {
        return switch (_) {
            .margin => "margin-top",
            .padding => "padding-top",
            .inset => "top",
            .scroll_margin => "scroll-margin-top",
            .scroll_padding => "scroll-padding-top",
        };
    }

    fn rightName(): []const u8 {
        return switch (_) {
            .margin => "margin-right",
            .padding => "padding-right",
            .inset => "right",
            .scroll_margin => "scroll-margin-right",
            .scroll_padding => "scroll-padding-right",
        };
    }

    fn bottomName(): []const u8 {
        return switch (_) {
            .margin => "margin-bottom",
            .padding => "padding-bottom",
            .inset => "bottom",
            .scroll_margin => "scroll-margin-bottom",
            .scroll_padding => "scroll-padding-bottom",
        };
    }

    fn leftName(): []const u8 {
        return switch (_) {
            .margin => "margin-left",
            .padding => "padding-left",
            .inset => "left",
            .scroll_margin => "scroll-margin-left",
            .scroll_padding => "scroll-padding-left",
        };
    }

    fn blockShorthandName(): []const u8 {
        return switch (_) {
            .margin => "margin-block",
            .padding => "padding-block",
            .inset => "inset-block",
            .scroll_margin => "scroll-margin-block",
            .scroll_padding => "scroll-padding-block",
        };
    }

    fn blockStartName(): []const u8 {
        return switch (_) {
            .margin => "margin-block-start",
            .padding => "padding-block-start",
            .inset => "inset-block-start",
            .scroll_margin => "scroll-margin-block-start",
            .scroll_padding => "scroll-padding-block-start",
        };
    }

    fn blockEndName(): []const u8 {
        return switch (_) {
            .margin => "margin-block-end",
            .padding => "padding-block-end",
            .inset => "inset-block-end",
            .scroll_margin => "scroll-margin-block-end",
            .scroll_padding => "scroll-padding-block-end",
        };
    }

    fn inlineShorthandName(): []const u8 {
        return switch (_) {
            .margin => "margin-inline",
            .padding => "padding-inline",
            .inset => "inset-inline",
            .scroll_margin => "scroll-margin-inline",
            .scroll_padding => "scroll-padding-inline",
        };
    }

    fn inlineStartName(): []const u8 {
        return switch (_) {
            .margin => "margin-inline-start",
            .padding => "padding-inline-start",
            .inset => "inset-inline-start",
            .scroll_margin => "scroll-margin-inline-start",
            .scroll_padding => "scroll-padding-inline-start",
        };
    }

    fn inlineEndName(): []const u8 {
        return switch (_) {
            .margin => "margin-inline-end",
            .padding => "padding-inline-end",
            .inset => "inset-inline-end",
            .scroll_margin => "scroll-margin-inline-end",
            .scroll_padding => "scroll-padding-inline-end",
        };
    }

    /// 判定该属性族的 4 边简写是否依赖 `logical_inset` 浏览器特性（仅 `inset` 简写）
    fn requiresInsetShorthandFeature(): bool {
        return _ == .inset;
    }

    /// 判定该属性族是否拥有逻辑属性特性门控（`margin` / `padding` / `inset`）
    fn hasLogicalCompatFeature(): bool {
        return _ == .margin || _ == .padding || _ == .inset;
    }
}

/// 单个间距家族内部的 11 种属性槽位
pub enum CssSpacingSide: u8 {
    shorthand = 0,
    top = 1,
    right = 2,
    bottom = 3,
    left = 4,
    block_shorthand = 5,
    block_start = 6,
    block_end = 7,
    inline_shorthand = 8,
    inline_start = 9,
    inline_end = 10,

    fn isLogical(): bool {
        return switch (_) {
            .block_shorthand, .block_start, .block_end, .inline_shorthand, .inline_start, .inline_end => true,
            else => false,
        };
    }

    fn isShorthand(): bool {
        return _ == .shorthand || _ == .block_shorthand || _ == .inline_shorthand;
    }
}

/// 物理/逻辑属性类别（用于级联保序 Flush 判定）
pub enum CssSpacingCategory: u8 {
    physical = 0,
    logical = 1,
}

/// 已识别的盒模型间距属性引用（家族 + 边槽位）
pub struct CssSpacingPropRef {
    family: CssBoxSpacingFamily = .margin,
    side: CssSpacingSide = .shorthand,

    /// 按 ASCII 大小写不敏感解析全部 55 种标准盒模型物理/逻辑间距属性名
    static fn parse(raw_name: []const u8): CssSpacingPropRef? {
        const name = trimWs(raw_name);
        if (name.len == 0) return null;

        // inset 物理四边无前缀（top/right/bottom/left）
        if (eqlAsciiIgnoreCase(name, "top")) return CssSpacingPropRef{ family: .inset, side: .top };
        if (eqlAsciiIgnoreCase(name, "right")) return CssSpacingPropRef{ family: .inset, side: .right };
        if (eqlAsciiIgnoreCase(name, "bottom")) return CssSpacingPropRef{ family: .inset, side: .bottom };
        if (eqlAsciiIgnoreCase(name, "left")) return CssSpacingPropRef{ family: .inset, side: .left };

        // 按前缀分派 5 大属性族
        if (startsWithAsciiIgnoreCase(name, "scroll-margin")) {
            return parseFamilySuffix(.scroll_margin, name[13..]);
        }
        if (startsWithAsciiIgnoreCase(name, "scroll-padding")) {
            return parseFamilySuffix(.scroll_padding, name[14..]);
        }
        if (startsWithAsciiIgnoreCase(name, "margin")) {
            return parseFamilySuffix(.margin, name[6..]);
        }
        if (startsWithAsciiIgnoreCase(name, "padding")) {
            return parseFamilySuffix(.padding, name[7..]);
        }
        if (startsWithAsciiIgnoreCase(name, "inset")) {
            return parseInsetSuffix(name[5..]);
        }
        return null;
    }

    /// 返回规范小写 CSS 属性名
    fn propertyName(): []const u8 {
        return switch (_.side) {
            .shorthand => _.family.shorthandName(),
            .top => _.family.topName(),
            .right => _.family.rightName(),
            .bottom => _.family.bottomName(),
            .left => _.family.leftName(),
            .block_shorthand => _.family.blockShorthandName(),
            .block_start => _.family.blockStartName(),
            .block_end => _.family.blockEndName(),
            .inline_shorthand => _.family.inlineShorthandName(),
            .inline_start => _.family.inlineStartName(),
            .inline_end => _.family.inlineEndName(),
        };
    }

    fn category(): CssSpacingCategory {
        if (_.side.isLogical()) {
            return .logical;
        }
        return .physical;
    }
}

/// 单边间距值切片视图（零堆分配，自动标记 `var()` / `env()` 等 unparsed 运行时表达式）
pub struct CssSpacingValue {
    raw: []const u8 = "",
    is_unparsed: bool = false,

    static fn fromSlice(raw: []const u8): CssSpacingValue! {
        const trimmed = trimWs(raw);
        if (trimmed.len == 0) {
            return err.EmptyValue;
        }
        return CssSpacingValue{
            raw: trimmed,
            is_unparsed: hasUnparsedFunction(trimmed),
        };
    }

    fn eql(other: CssSpacingValue): bool {
        if (_.is_unparsed != other.is_unparsed) {
            return false;
        }
        return bytesEql(_.raw, other.raw);
    }
}

/// CSS 四边矩形简写（`margin` / `padding` / `inset` / `scroll-margin` / `scroll-padding`）
pub struct CssSpacingRect {
    top: CssSpacingValue = CssSpacingValue{},
    right: CssSpacingValue = CssSpacingValue{},
    bottom: CssSpacingValue = CssSpacingValue{},
    left: CssSpacingValue = CssSpacingValue{},

    /// 解析 1~4 个空白分隔的顶层 CSS 值（保护 `calc(...)` / `var(...)` 内空白）
    static fn parse(raw: []const u8): CssSpacingRect! {
        let parts: [4][]const u8 = [""; 4];
        const n = splitTopLevelValues(raw, parts[0..], 4)?;
        if (n == 0) {
            return err.EmptyValue;
        }
        const v0 = CssSpacingValue.fromSlice(parts[0])?;
        if (n == 1) {
            return CssSpacingRect{ top: v0, right: v0, bottom: v0, left: v0 };
        }
        const v1 = CssSpacingValue.fromSlice(parts[1])?;
        if (n == 2) {
            return CssSpacingRect{ top: v0, right: v1, bottom: v0, left: v1 };
        }
        const v2 = CssSpacingValue.fromSlice(parts[2])?;
        if (n == 3) {
            return CssSpacingRect{ top: v0, right: v1, bottom: v2, left: v1 };
        }
        const v3 = CssSpacingValue.fromSlice(parts[3])?;
        return CssSpacingRect{ top: v0, right: v1, bottom: v2, left: v3 };
    }

    /// 按 CSS 最小化规则序列化为 1/2/3/4 边简写字符串
    fn formatCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, _.top.raw)?;

        const lr_eq = _.left.eql(_.right);
        const tb_eq = _.top.eql(_.bottom);

        if (lr_eq && tb_eq && _.top.eql(_.right)) {
            return pos;
        }

        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.right.raw)?;

        if (lr_eq && tb_eq) {
            return pos;
        }

        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.bottom.raw)?;

        if (lr_eq) {
            return pos;
        }

        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.left.raw)?;
        return pos;
    }
}

/// CSS 逻辑双边轴简写（`*-block` / `*-inline`）
pub struct CssSpacingAxis {
    start: CssSpacingValue = CssSpacingValue{},
    end: CssSpacingValue = CssSpacingValue{},

    /// 解析 1~2 个空白分隔的顶层 CSS 值（单值同时赋给 start 与 end）
    static fn parse(raw: []const u8): CssSpacingAxis! {
        let parts: [2][]const u8 = [""; 2];
        const n = splitTopLevelValues(raw, parts[0..], 2)?;
        if (n == 0) {
            return err.EmptyValue;
        }
        const v0 = CssSpacingValue.fromSlice(parts[0])?;
        if (n == 1) {
            return CssSpacingAxis{ start: v0, end: v0 };
        }
        const v1 = CssSpacingValue.fromSlice(parts[1])?;
        return CssSpacingAxis{ start: v0, end: v1 };
    }

    /// 双值相等时折叠为单值输出，否则输出 `start end`
    fn formatCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, _.start.raw)?;
        if (!_.start.eql(_.end)) {
            pos = appendChar(out, pos, ' ')?;
            pos = appendBytes(out, pos, _.end.raw)?;
        }
        return pos;
    }
}

/// 目标浏览器对 CSS 逻辑间距属性与简写的兼容能力配置
pub struct CssSpacingTarget {
    /// 是否原生支持 `*-block-start` / `*-inline-start` 等逻辑长写属性
    supports_logical: bool = true,
    /// 是否原生支持 `*-block` / `*-inline` 等逻辑双边简写
    supports_logical_shorthand: bool = true,
    /// 是否原生支持 `inset` 四边简写（对应 Bun `Feature.logical_inset`）
    supports_inset_shorthand: bool = true,

    /// 现代浏览器基线（全部原生保留并合并为最短简写）
    static fn modern(): CssSpacingTarget {
        return CssSpacingTarget{
            supports_logical: true,
            supports_logical_shorthand: true,
            supports_inset_shorthand: true,
        };
    }

    /// 旧版浏览器基线（逻辑属性全部编译为物理属性 + `:dir(ltr)`/`:dir(rtl)` 分支）
    static fn legacy(): CssSpacingTarget {
        return CssSpacingTarget{
            supports_logical: false,
            supports_logical_shorthand: false,
            supports_inset_shorthand: false,
        };
    }

    /// Safari 14 / 旧 Chrome 过渡基线（支持逻辑长写，不支持 `*-block`/`*-inline` 与 `inset` 简写）
    static fn safari14(): CssSpacingTarget {
        return CssSpacingTarget{
            supports_logical: true,
            supports_logical_shorthand: false,
            supports_inset_shorthand: false,
        };
    }
}

/// 零堆分配单条输出声明
pub struct CssSpacingDecl {
    prop_name: []const u8 = "",
    value: [CSS_MARGIN_PAD_MAX_VAL_BYTES]u8 = [0; CSS_MARGIN_PAD_MAX_VAL_BYTES],
    value_len: usize = 0,

    fn valueSlice(): []const u8 {
        return _.value[0.._.value_len];
    }

    fn formatCss(out: []u8, minify: bool): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, _.prop_name)?;
        pos = appendChar(out, pos, ':')?;
        if (!minify) {
            pos = appendChar(out, pos, ' ')?;
        }
        pos = appendBytes(out, pos, _.value[0.._.value_len])?;
        return pos;
    }
}

/// 零堆分配盒模型间距编译输出容器（含主声明列表及 LTR/RTL 方向降级分支）
pub struct CssSpacingOutput {
    decls: [CSS_MARGIN_PAD_MAX_DECLS]CssSpacingDecl = [CssSpacingDecl{}; CSS_MARGIN_PAD_MAX_DECLS],
    decl_len: usize = 0,
    ltr_decls: [CSS_MARGIN_PAD_MAX_DIR_DECLS]CssSpacingDecl = [CssSpacingDecl{}; CSS_MARGIN_PAD_MAX_DIR_DECLS],
    ltr_len: usize = 0,
    rtl_decls: [CSS_MARGIN_PAD_MAX_DIR_DECLS]CssSpacingDecl = [CssSpacingDecl{}; CSS_MARGIN_PAD_MAX_DIR_DECLS],
    rtl_len: usize = 0,

    static fn init(): CssSpacingOutput {
        return CssSpacingOutput{};
    }

    fn reset(*_) {
        _.decl_len = 0;
        _.ltr_len = 0;
        _.rtl_len = 0;
    }

    fn hasDirectionalRules(): bool {
        return _.ltr_len > 0 || _.rtl_len > 0;
    }

    fn pushDecl(*_, prop_name: []const u8, val: []const u8): usize! {
        if (_.decl_len >= CSS_MARGIN_PAD_MAX_DECLS) {
            return err.OutputFull;
        }
        if (val.len > CSS_MARGIN_PAD_MAX_VAL_BYTES) {
            return err.BufferTooSmall;
        }
        let d = CssSpacingDecl{ prop_name: prop_name, value_len: val.len };
        copyBytes(d.value[0..], val);
        _.decls[_.decl_len] = d;
        _.decl_len += 1;
        return _.decl_len;
    }

    fn pushLtr(*_, prop_name: []const u8, val: []const u8): usize! {
        if (_.ltr_len >= CSS_MARGIN_PAD_MAX_DIR_DECLS) {
            return err.OutputFull;
        }
        if (val.len > CSS_MARGIN_PAD_MAX_VAL_BYTES) {
            return err.BufferTooSmall;
        }
        let d = CssSpacingDecl{ prop_name: prop_name, value_len: val.len };
        copyBytes(d.value[0..], val);
        _.ltr_decls[_.ltr_len] = d;
        _.ltr_len += 1;
        return _.ltr_len;
    }

    fn pushRtl(*_, prop_name: []const u8, val: []const u8): usize! {
        if (_.rtl_len >= CSS_MARGIN_PAD_MAX_DIR_DECLS) {
            return err.OutputFull;
        }
        if (val.len > CSS_MARGIN_PAD_MAX_VAL_BYTES) {
            return err.BufferTooSmall;
        }
        let d = CssSpacingDecl{ prop_name: prop_name, value_len: val.len };
        copyBytes(d.value[0..], val);
        _.rtl_decls[_.rtl_len] = d;
        _.rtl_len += 1;
        return _.rtl_len;
    }

    fn formatDeclsCss(out: []u8, minify: bool): usize! {
        return formatDeclList(_.decls[0.._.decl_len], out, minify);
    }

    fn formatLtrCss(out: []u8, minify: bool): usize! {
        return formatDeclList(_.ltr_decls[0.._.ltr_len], out, minify);
    }

    fn formatRtlCss(out: []u8, minify: bool): usize! {
        return formatDeclList(_.rtl_decls[0.._.rtl_len], out, minify);
    }
}

/// CSS 盒模型物理/逻辑间距属性合并与降级状态机（对齐 Bun `NewSizeHandler`）
pub struct CssBoxSpacingHandler {
    family: CssBoxSpacingFamily = .margin,
    target: CssSpacingTarget = CssSpacingTarget.modern(),
    top: CssSpacingValue? = null,
    right: CssSpacingValue? = null,
    bottom: CssSpacingValue? = null,
    left: CssSpacingValue? = null,
    block_start: CssSpacingValue? = null,
    block_end: CssSpacingValue? = null,
    inline_start: CssSpacingValue? = null,
    inline_end: CssSpacingValue? = null,
    has_any: bool = false,
    category: CssSpacingCategory = .physical,

    static fn init(family: CssBoxSpacingFamily, target: CssSpacingTarget): CssBoxSpacingHandler {
        return CssBoxSpacingHandler{
            family: family,
            target: target,
        };
    }

    /// 处理单条 CSS 属性声明；若属性不属于当前 `family` 返回 `false`
    fn handleProperty(
        *_,
        prop_name: []const u8,
        raw_value: []const u8,
        out: *CssSpacingOutput,
    ): bool! {
        const parsed = CssSpacingPropRef.parse(prop_name);
        if (parsed == null) {
            return false;
        }
        const pref = parsed.?;
        if (pref.family != _.family) {
            return false;
        }

        const trimmed = trimWs(raw_value);
        if (trimmed.len == 0) {
            return err.EmptyValue;
        }

        switch (pref.side) {
            .top => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                if (v.is_unparsed) {
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.topName(), v.raw)?;
                } else {
                    * = _.setPhysicalSlot(.top, v, out)?;
                }
            },
            .right => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                if (v.is_unparsed) {
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.rightName(), v.raw)?;
                } else {
                    * = _.setPhysicalSlot(.right, v, out)?;
                }
            },
            .bottom => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                if (v.is_unparsed) {
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.bottomName(), v.raw)?;
                } else {
                    * = _.setPhysicalSlot(.bottom, v, out)?;
                }
            },
            .left => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                if (v.is_unparsed) {
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.leftName(), v.raw)?;
                } else {
                    * = _.setPhysicalSlot(.left, v, out)?;
                }
            },
            .shorthand => {
                if (hasUnparsedFunction(trimmed)) {
                    // 含 var()/env() 的 4 边简写可能在运行时展开为多个值，不能安全拆分为四边，先 flush 再原样保留
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.shorthandName(), trimmed)?;
                } else {
                    const rect = CssSpacingRect.parse(trimmed)?;
                    if (_.has_any && _.category != .physical) {
                        * = _.flush(out)?;
                    }
                    _.top = rect.top;
                    _.right = rect.right;
                    _.bottom = rect.bottom;
                    _.left = rect.left;
                    _.block_start = null;
                    _.block_end = null;
                    _.inline_start = null;
                    _.inline_end = null;
                    _.category = .physical;
                    _.has_any = true;
                }
            },
            .block_start => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                * = _.setLogicalSlot(.block_start, v, out)?;
            },
            .block_end => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                * = _.setLogicalSlot(.block_end, v, out)?;
            },
            .inline_start => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                * = _.setLogicalSlot(.inline_start, v, out)?;
            },
            .inline_end => {
                const v = CssSpacingValue.fromSlice(trimmed)?;
                * = _.setLogicalSlot(.inline_end, v, out)?;
            },
            .block_shorthand => {
                if (hasUnparsedFunction(trimmed) && countTopLevelValues(trimmed) == 1) {
                    // 单值 var(--x) 可能包含双值，不能拆分，先 flush 原样透传
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.blockShorthandName(), trimmed)?;
                } else {
                    const axis = CssSpacingAxis.parse(trimmed)?;
                    * = _.setLogicalSlot(.block_start, axis.start, out)?;
                    * = _.setLogicalSlot(.block_end, axis.end, out)?;
                }
            },
            .inline_shorthand => {
                if (hasUnparsedFunction(trimmed) && countTopLevelValues(trimmed) == 1) {
                    * = _.flush(out)?;
                    * = out.pushDecl(_.family.inlineShorthandName(), trimmed)?;
                } else {
                    const axis = CssSpacingAxis.parse(trimmed)?;
                    * = _.setLogicalSlot(.inline_start, axis.start, out)?;
                    * = _.setLogicalSlot(.inline_end, axis.end, out)?;
                }
            },
        }
        return true;
    }

    /// 刷出当前暂存的物理或逻辑属性槽位并执行简写合并与浏览器降级
    fn flush(*_, out: *CssSpacingOutput): usize! {
        if (!_.has_any) {
            return out.decl_len;
        }
        _.has_any = false;

        const top = _.top;
        const right = _.right;
        const bottom = _.bottom;
        const left = _.left;
        _.top = null;
        _.right = null;
        _.bottom = null;
        _.left = null;

        const can_shorthand = if (_.family.requiresInsetShorthandFeature())
            _.target.supports_inset_shorthand
        else
            true;

        if (can_shorthand && top != null && right != null && bottom != null && left != null) {
            const rect = CssSpacingRect{
                top: top.?,
                right: right.?,
                bottom: bottom.?,
                left: left.?,
            };
            let buf: [CSS_MARGIN_PAD_MAX_VAL_BYTES]u8 = [0; CSS_MARGIN_PAD_MAX_VAL_BYTES];
            const n = rect.formatCss(buf[0..])?;
            * = out.pushDecl(_.family.shorthandName(), buf[0..n])?;
        } else {
            if (v := top) {
                * = out.pushDecl(_.family.topName(), v.raw)?;
            }
            if (v := bottom) {
                * = out.pushDecl(_.family.bottomName(), v.raw)?;
            }
            if (v := left) {
                * = out.pushDecl(_.family.leftName(), v.raw)?;
            }
            if (v := right) {
                * = out.pushDecl(_.family.rightName(), v.raw)?;
            }
        }

        const block_start = _.block_start;
        const block_end = _.block_end;
        const inline_start = _.inline_start;
        const inline_end = _.inline_end;
        _.block_start = null;
        _.block_end = null;
        _.inline_start = null;
        _.inline_end = null;

        const logical_supported = if (_.family.hasLogicalCompatFeature())
            _.target.supports_logical
        else
            true;
        const shorthand_supported = logical_supported && _.target.supports_logical_shorthand;

        // 1. Block 轴输出或降级为 top / bottom
        if (logical_supported) {
            if (shorthand_supported && block_start != null && block_end != null &&
                !block_start.?.is_unparsed && !block_end.?.is_unparsed)
            {
                const axis = CssSpacingAxis{ start: block_start.?, end: block_end.? };
                let buf: [CSS_MARGIN_PAD_MAX_VAL_BYTES]u8 = [0; CSS_MARGIN_PAD_MAX_VAL_BYTES];
                const n = axis.formatCss(buf[0..])?;
                * = out.pushDecl(_.family.blockShorthandName(), buf[0..n])?;
            } else {
                if (v := block_start) {
                    * = out.pushDecl(_.family.blockStartName(), v.raw)?;
                }
                if (v := block_end) {
                    * = out.pushDecl(_.family.blockEndName(), v.raw)?;
                }
            }
        } else {
            if (v := block_start) {
                * = out.pushDecl(_.family.topName(), v.raw)?;
            }
            if (v := block_end) {
                * = out.pushDecl(_.family.bottomName(), v.raw)?;
            }
        }

        // 2. Inline 轴输出或降级为物理 left/right 及 LTR/RTL 方向规则
        if (logical_supported) {
            if (shorthand_supported && inline_start != null && inline_end != null &&
                !inline_start.?.is_unparsed && !inline_end.?.is_unparsed)
            {
                const axis = CssSpacingAxis{ start: inline_start.?, end: inline_end.? };
                let buf: [CSS_MARGIN_PAD_MAX_VAL_BYTES]u8 = [0; CSS_MARGIN_PAD_MAX_VAL_BYTES];
                const n = axis.formatCss(buf[0..])?;
                * = out.pushDecl(_.family.inlineShorthandName(), buf[0..n])?;
            } else {
                if (v := inline_start) {
                    * = out.pushDecl(_.family.inlineStartName(), v.raw)?;
                }
                if (v := inline_end) {
                    * = out.pushDecl(_.family.inlineEndName(), v.raw)?;
                }
            }
        } else if (inline_start != null || inline_end != null) {
            // Bun / LightningCSS 等值优化：inline-start 与 inline-end 非 unparsed 且相等时直接输出物理 left + right，免去 LTR/RTL 分支
            const symmetric = inline_start != null && inline_end != null &&
                !inline_start.?.is_unparsed && !inline_end.?.is_unparsed &&
                inline_start.?.eql(inline_end.?);
            if (symmetric) {
                * = out.pushDecl(_.family.leftName(), inline_start.?.raw)?;
                * = out.pushDecl(_.family.rightName(), inline_end.?.raw)?;
            } else {
                if (v := inline_start) {
                    * = out.pushLtr(_.family.leftName(), v.raw)?;
                    * = out.pushRtl(_.family.rightName(), v.raw)?;
                }
                if (v := inline_end) {
                    * = out.pushLtr(_.family.rightName(), v.raw)?;
                    * = out.pushRtl(_.family.leftName(), v.raw)?;
                }
            }
        }

        return out.decl_len;
    }

    /// 规则块结束时调用，刷出所有挂起声明
    fn finalize(*_, out: *CssSpacingOutput): usize! {
        return _.flush(out);
    }

    fn setPhysicalSlot(*_, side: CssSpacingSide, val: CssSpacingValue, out: *CssSpacingOutput): usize! {
        if (_.has_any && _.category != .physical) {
            * = _.flush(out)?;
        }
        switch (side) {
            .top => _.top = val,
            .right => _.right = val,
            .bottom => _.bottom = val,
            .left => _.left = val,
            else => {},
        }
        _.category = .physical;
        _.has_any = true;
        return out.decl_len;
    }

    fn setLogicalSlot(*_, side: CssSpacingSide, val: CssSpacingValue, out: *CssSpacingOutput): usize! {
        const had_existing = switch (side) {
            .block_start => _.block_start != null,
            .block_end => _.block_end != null,
            .inline_start => _.inline_start != null,
            .inline_end => _.inline_end != null,
            else => false,
        };
        if (_.has_any && (_.category != .logical || (had_existing && val.is_unparsed))) {
            * = _.flush(out)?;
        }
        switch (side) {
            .block_start => _.block_start = val,
            .block_end => _.block_end = val,
            .inline_start => _.inline_start = val,
            .inline_end => _.inline_end = val,
            else => {},
        }
        _.category = .logical;
        _.has_any = true;
        return out.decl_len;
    }
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
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

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn startsWithAsciiIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqlAsciiIgnoreCase(s[0..prefix.len], prefix);
}

fn containsAsciiIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0 || haystack.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (eqlAsciiIgnoreCase(haystack[i .. i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn hasUnparsedFunction(raw: []const u8): bool {
    return containsAsciiIgnoreCase(raw, "var(") || containsAsciiIgnoreCase(raw, "env(");
}

fn parseFamilySuffix(family: CssBoxSpacingFamily, suffix: []const u8): CssSpacingPropRef? {
    if (suffix.len == 0) return CssSpacingPropRef{ family: family, side: .shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-top")) return CssSpacingPropRef{ family: family, side: .top };
    if (eqlAsciiIgnoreCase(suffix, "-right")) return CssSpacingPropRef{ family: family, side: .right };
    if (eqlAsciiIgnoreCase(suffix, "-bottom")) return CssSpacingPropRef{ family: family, side: .bottom };
    if (eqlAsciiIgnoreCase(suffix, "-left")) return CssSpacingPropRef{ family: family, side: .left };
    if (eqlAsciiIgnoreCase(suffix, "-block")) return CssSpacingPropRef{ family: family, side: .block_shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-block-start")) return CssSpacingPropRef{ family: family, side: .block_start };
    if (eqlAsciiIgnoreCase(suffix, "-block-end")) return CssSpacingPropRef{ family: family, side: .block_end };
    if (eqlAsciiIgnoreCase(suffix, "-inline")) return CssSpacingPropRef{ family: family, side: .inline_shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-inline-start")) return CssSpacingPropRef{ family: family, side: .inline_start };
    if (eqlAsciiIgnoreCase(suffix, "-inline-end")) return CssSpacingPropRef{ family: family, side: .inline_end };
    return null;
}

fn parseInsetSuffix(suffix: []const u8): CssSpacingPropRef? {
    if (suffix.len == 0) return CssSpacingPropRef{ family: .inset, side: .shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-block")) return CssSpacingPropRef{ family: .inset, side: .block_shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-block-start")) return CssSpacingPropRef{ family: .inset, side: .block_start };
    if (eqlAsciiIgnoreCase(suffix, "-block-end")) return CssSpacingPropRef{ family: .inset, side: .block_end };
    if (eqlAsciiIgnoreCase(suffix, "-inline")) return CssSpacingPropRef{ family: .inset, side: .inline_shorthand };
    if (eqlAsciiIgnoreCase(suffix, "-inline-start")) return CssSpacingPropRef{ family: .inset, side: .inline_start };
    if (eqlAsciiIgnoreCase(suffix, "-inline-end")) return CssSpacingPropRef{ family: .inset, side: .inline_end };
    return null;
}

fn splitTopLevelValues(raw: []const u8, out_parts: [][]const u8, max_parts: usize): usize! {
    let count: usize = 0;
    let i: usize = 0;

    while (i < raw.len) {
        while (i < raw.len && isWs(raw[i])) {
            i += 1;
        }
        if (i >= raw.len) break;

        const start = i;
        let paren_depth: usize = 0;
        let bracket_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i < raw.len) {
            const c = raw[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < raw.len) {
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
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')') {
                if (paren_depth == 0) return err.UnclosedDelimiter;
                paren_depth -= 1;
            } else if (c == '[') {
                bracket_depth += 1;
            } else if (c == ']') {
                if (bracket_depth == 0) return err.UnclosedDelimiter;
                bracket_depth -= 1;
            } else if (paren_depth == 0 && bracket_depth == 0 && isWs(c)) {
                break;
            }
            i += 1;
        }

        if (paren_depth != 0 || bracket_depth != 0 || in_quote != 0) {
            return err.UnclosedDelimiter;
        }
        if (count >= max_parts) {
            return err.TooManyValues;
        }
        out_parts[count] = raw[start..i];
        count += 1;
    }

    return count;
}

fn countTopLevelValues(raw: []const u8): usize {
    let parts: [4][]const u8 = [""; 4];
    const n = splitTopLevelValues(raw, parts[0..], 4) catch {
        return 2;
    };
    return n;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn formatDeclList(decls: []const CssSpacingDecl, out: []u8, minify: bool): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < decls.len) {
        if (i > 0) {
            pos = appendChar(out, pos, ';')?;
            if (!minify) {
                pos = appendChar(out, pos, ' ')?;
            }
        }
        const n = decls[i].formatCss(out[pos..], minify)?;
        pos += n;
        i += 1;
    }
    return pos;
}
