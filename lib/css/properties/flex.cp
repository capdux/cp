import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;

const Property = css.Property;
const PropertyId = css.PropertyId;

const CSSNumberFns = css.css_values.number.CSSNumberFns;
const LengthPercentage = css.css_values.length.LengthPercentage;
const CSSNumber = css.css_values.number.CSSNumber;
const LengthPercentageOrAuto = css.css_values.length.LengthPercentageOrAuto;
const Length = css.css_values.length.LengthValue;
const CSSInteger = css.css_values.number.CSSInteger;

const isFlex2009 = css.prefixes.Feature.isFlex2009;

const VendorPrefix = css.VendorPrefix;

/// A value for the [flex-direction](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#propdef-flex-direction) property.
/// A value for the [flex-direction](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#propdef-flex-direction) property.
pub enum FlexDirection {
    /// Flex items are laid out in a row.
    row,
    /// Flex items are laid out in a row, and reversed.
    @"row-reverse",
    /// Flex items are laid out in a column.
    column,
    /// Flex items are laid out in a column, and reversed.
    @"column-reverse",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): FlexDirection {
        return .row;
    }

    fn to2009(): OrientAndDirection {
        return switch (_.*) {
            .row => .{ .horizontal, .normal },
            .column => .{ .vertical, .normal },
            .@"row-reverse" => .{ .horizontal, .reverse },
            .@"column-reverse" => .{ .vertical, .reverse },
        };
    }
}

/// A value for the [flex-wrap](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#flex-wrap-property) property.
/// A value for the [flex-wrap](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#flex-wrap-property) property.
pub enum FlexWrap {
    /// The flex items do not wrap.
    nowrap,
    /// The flex items wrap.
    wrap,
    /// The flex items wrap, in reverse.
    @"wrap-reverse",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): FlexWrap {
        return .nowrap;
    }

    fn fromStandard(): FlexWrap? {
        return _;
    }
}

/// A value for the [flex-flow](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#flex-flow-property) shorthand property.
pub struct FlexFlow {
    /// The direction that flex items flow.
    direction: FlexDirection,
    /// How the flex items wrap.
    wrap: FlexWrap,

    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"flex-flow", PropertyFieldMap);

    pub const PropertyFieldMap = {
        direction: css.PropertyIdTag.@"flex-direction",
        wrap: css.PropertyIdTag.@"flex-wrap",
    };

    pub const VendorPrefixMap = {
        direction: true,
        wrap: true,
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let direction: FlexDirection? = null;
        let wrap: FlexWrap? = null;

        while (true) {
            if (direction == null) {
                if (value := input.tryParse(FlexDirection.parse, {}).asValue()) {
                    direction = value;
                    continue;
                }
            }
            if (wrap == null) {
                if (value := input.tryParse(FlexWrap.parse, {}).asValue()) {
                    wrap = value;
                    continue;
                }
            }
            break;
        }

        return {
            result: FlexFlow{
                direction: direction ?? FlexDirection.row,
                wrap: wrap ?? FlexWrap.nowrap,
            },
        };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        let needs_space = false;
        if (!_.direction.eql(&FlexDirection.default()) || _.wrap.eql(&FlexWrap.default())) {
            _.direction.toCss(dest)?;
            needs_space = true;
        }

        if (!_.wrap.eql(&FlexWrap.default())) {
            if (needs_space) {
                dest.writeStr(" ")?;
            }
            _.wrap.toCss(dest)?;
        }

        return;
    }

    static fn deepClone(val: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), val, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [flex](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#flex-property) shorthand property.
/// A value for the [flex](https://www.w3.org/TR/2018/CR-css-flexbox-1-20181119/#flex-property) shorthand property.
pub struct Flex {
    /// The flex grow factor.
    grow: CSSNumber,
    /// The flex shrink factor.
    shrink: CSSNumber,
    /// The flex basis.
    basis: LengthPercentageOrAuto,

    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.flex, PropertyFieldMap);

    pub const PropertyFieldMap = {
        grow: css.PropertyIdTag.@"flex-grow",
        shrink: css.PropertyIdTag.@"flex-shrink",
        basis: css.PropertyIdTag.@"flex-basis",
    };

    pub const VendorPrefixMap = {
        grow: true,
        shrink: true,
        basis: true,
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        if (input.tryParse(css.Parser.expectIdentMatching, .{"none"}).isOk()) {
            return {
                result: {
                    grow: 0.0,
                    shrink: 0.0,
                    basis: LengthPercentageOrAuto.auto,
                },
            };
        }

        let grow: CSSNumber? = null;
        let shrink: CSSNumber? = null;
        let basis: LengthPercentageOrAuto? = null;

        while (true) {
            if (grow == null) {
                if (value := input.tryParse(CSSNumberFns.parse, {}).asValue()) {
                    grow = value;
                    shrink = input.tryParse(CSSNumberFns.parse, {}).asValue();
                    continue;
                }
            }

            if (basis == null) {
                if (value := input.tryParse(LengthPercentageOrAuto.parse, {}).asValue()) {
                    basis = value;
                    continue;
                }
            }

            break;
        }

        return {
            result: {
                grow: grow ?? 1.0,
                shrink: shrink ?? 1.0,
                basis: basis ?? LengthPercentageOrAuto{ .length = LengthPercentage{ .percentage = { v: 0.0 } } },
            },
        };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        if (_.grow == 0.0 && _.shrink == 0.0 && _.basis == .auto) {
            dest.writeStr("none")?;
            return;
        }

        const ZeroKind = enum {
            NonZero,
            Length,
            Percentage,
        };

        // If the basis is unitless 0, we must write all three components to disambiguate.
        // If the basis is 0%, we can omit the basis.
        const basis_kind = switch (_.basis) {
            .length => |lp| brk: {
                if (lp == .dimension && lp.dimension.isZero()) break :brk ZeroKind.Length;
                if (lp == .percentage && lp.percentage.isZero()) break :brk ZeroKind.Percentage;
                break :brk ZeroKind.NonZero;
            },
            else => ZeroKind.NonZero,
        };

        if (_.grow != 1.0 || _.shrink != 1.0 || basis_kind != .NonZero) {
            CSSNumberFns.toCss(&_.grow, dest)?;
            if (_.shrink != 1.0 || basis_kind == .Length) {
                dest.writeStr(" ")?;
                CSSNumberFns.toCss(&_.shrink, dest)?;
            }
        }

        if (basis_kind != .Percentage) {
            if (_.grow != 1.0 || _.shrink != 1.0 || basis_kind == .Length) {
                dest.writeStr(" ")?;
            }
            _.basis.toCss(dest)?;
        }
    }

    static fn deepClone(val: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), val, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the legacy (prefixed) [box-orient](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#orientation) property.
/// Partially equivalent to `flex-direction` in the standard syntax.
/// A value for the legacy (prefixed) [box-orient](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#orientation) property.
/// Partially equivalent to `flex-direction` in the standard syntax.
pub enum BoxOrient {
    /// Items are laid out horizontally.
    horizontal,
    /// Items are laid out vertically.
    vertical,
    /// Items are laid out along the inline axis, according to the writing direction.
    @"inline-axis",
    /// Items are laid out along the block axis, according to the writing direction.
    @"block-axis",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the legacy (prefixed) [box-direction](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#displayorder) property.
/// Partially equivalent to the `flex-direction` property in the standard syntax.
pub enum BoxDirection {
    /// Items flow in the natural direction.
    normal,
    /// Items flow in the reverse direction.
    reverse,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

pub const FlexAlign = BoxAlign;

/// A value for the legacy (prefixed) [box-align](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#alignment) property.
/// Equivalent to the `align-items` property in the standard syntax.
/// A value for the legacy (prefixed) [box-align](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#alignment) property.
/// Equivalent to the `align-items` property in the standard syntax.
pub enum BoxAlign {
    /// Items are aligned to the start.
    start,
    /// Items are aligned to the end.
    end,
    /// Items are centered.
    center,
    /// Items are aligned to the baseline.
    baseline,
    /// Items are stretched.
    stretch,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(align_val: *const css.css_properties.@"align".AlignItems): BoxAlign? {
        return switch (align_val.*) {
            .self_position => |sp| if (sp.overflow == null) switch (sp.value) {
                .start, .@"flex-start" => .start,
                .end, .@"flex-end" => .end,
                .center => .center,
                else => null,
            } else null,
            .stretch => .stretch,
            else => null,
        };
    }
}

/// A value for the legacy (prefixed) [box-pack](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#packing) property.
/// Equivalent to the `justify-content` property in the standard syntax.
/// A value for the legacy (prefixed) [box-pack](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#packing) property.
/// Equivalent to the `justify-content` property in the standard syntax.
pub enum BoxPack {
    /// Items are justified to the start.
    start,
    /// Items are justified to the end.
    end,
    /// Items are centered.
    center,
    /// Items are justified to the start and end.
    justify,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(justify: *const css.css_properties.@"align".JustifyContent): BoxPack? {
        return switch (justify.*) {
            .content_distribution => |cd| switch (cd) {
                .@"space-between" => .justify,
                else => null,
            },
            .content_position => |cp| if (cp.overflow == null) switch (cp.value) {
                .start, .@"flex-start" => .start,
                .end, .@"flex-end" => .end,
                .center => .center,
            } else null,
            else => null,
        };
    }
}

/// A value for the legacy (prefixed) [box-lines](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#multiple) property.
/// Equivalent to the `flex-wrap` property in the standard syntax.
/// A value for the legacy (prefixed) [box-lines](https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#multiple) property.
/// Equivalent to the `flex-wrap` property in the standard syntax.
pub enum BoxLines {
    /// Items are laid out in a single line.
    single,
    /// Items may wrap into multiple lines.
    multiple,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(wrap: *const FlexWrap): BoxLines? {
        return switch (wrap.*) {
            .nowrap => .single,
            .wrap => .multiple,
            else => null,
        };
    }
}

// Old flex (2012): https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/
/// A value for the legacy (prefixed) [flex-pack](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-pack) property.
/// Equivalent to the `justify-content` property in the standard syntax.
/// A value for the legacy (prefixed) [flex-pack](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-pack) property.
/// Equivalent to the `justify-content` property in the standard syntax.
pub enum FlexPack {
    /// Items are justified to the start.
    start,
    /// Items are justified to the end.
    end,
    /// Items are centered.
    center,
    /// Items are justified to the start and end.
    justify,
    /// Items are distributed evenly, with half size spaces on either end.
    distribute,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(justify: *const css.css_properties.@"align".JustifyContent): FlexPack? {
        return switch (justify.*) {
            .content_distribution => |cd| switch (cd) {
                .@"space-between" => .justify,
                .@"space-around" => .distribute,
                else => null,
            },
            .content_position => |cp| if (cp.overflow == null) switch (cp.value) {
                .start, .@"flex-start" => .start,
                .end, .@"flex-end" => .end,
                .center => .center,
            } else null,
            else => null,
        };
    }
}

/// A value for the legacy (prefixed) [flex-item-align](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-align) property.
/// Equivalent to the `align-self` property in the standard syntax.
/// A value for the legacy (prefixed) [flex-item-align](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-align) property.
/// Equivalent to the `align-self` property in the standard syntax.
pub enum FlexItemAlign {
    /// Equivalent to the value of `flex-align`.
    auto,
    /// The item is aligned to the start.
    start,
    /// The item is aligned to the end.
    end,
    /// The item is centered.
    center,
    /// The item is aligned to the baseline.
    baseline,
    /// The item is stretched.
    stretch,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(justify: *const css.css_properties.@"align".AlignSelf): FlexItemAlign? {
        return switch (justify.*) {
            .auto => .auto,
            .stretch => .stretch,
            .self_position => |sp| if (sp.overflow == null) switch (sp.value) {
                .start, .@"flex-start" => .start,
                .end, .@"flex-end" => .end,
                .center => .center,
                else => null,
            } else null,
            else => null,
        };
    }
}

/// A value for the legacy (prefixed) [flex-line-pack](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-line-pack) property.
/// Equivalent to the `align-content` property in the standard syntax.
/// A value for the legacy (prefixed) [flex-line-pack](https://www.w3.org/TR/2012/WD-css3-flexbox-20120322/#flex-line-pack) property.
/// Equivalent to the `align-content` property in the standard syntax.
pub enum FlexLinePack {
    /// Content is aligned to the start.
    start,
    /// Content is aligned to the end.
    end,
    /// Content is centered.
    center,
    /// Content is justified.
    justify,
    /// Content is distributed evenly, with half size spaces on either end.
    distribute,
    /// Content is stretched.
    stretch,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn fromStandard(justify: *const css.css_properties.@"align".AlignContent): FlexLinePack? {
        return switch (justify.*) {
            .content_distribution => |cd| switch (cd) {
                .@"space-between" => .justify,
                .@"space-around" => .distribute,
                .stretch => .stretch,
                else => null,
            },
            .content_position => |cp| if (cp.overflow == null) switch (cp.value) {
                .start, .@"flex-start" => .start,
                .end, .@"flex-end" => .end,
                .center => .center,
            } else null,
            else => null,
        };
    }
}

pub const BoxOrdinalGroup = CSSInteger;

const FlexDirectionAndPrefix = struct { FlexDirection, VendorPrefix };
const BoxOrientAndPrefix = struct { BoxOrient, VendorPrefix };
const BoxDirectionAndPrefix = struct { BoxDirection, VendorPrefix };
const FlexWrapAndPrefix = struct { FlexWrap, VendorPrefix };
const BoxLinesAndPrefix = struct { BoxLines, VendorPrefix };

type StrAlias = []const u8;
const Prop2009 = struct { type, StrAlias };
const OrientAndDirection = struct { BoxOrient, BoxDirection };
const CSSNumberAndPrefix = struct { CSSNumber, VendorPrefix };
const LengthPercentageOrAutoAndPrefix = struct { LengthPercentageOrAuto, VendorPrefix };
const CSSIntegerAndPrefix = struct { CSSInteger, VendorPrefix };
const BoxOrdinalGroupAndPrefix = struct { BoxOrdinalGroup, VendorPrefix };

// A handler for flex-related properties that manages both standard and legacy vendor prefixed values.
pub struct FlexHandler {
    /// The flex-direction property value and vendor prefix
    direction: FlexDirectionAndPrefix? = null,
    /// The box-orient property value and vendor prefix (legacy)
    box_orient: BoxOrientAndPrefix? = null,
    /// The box-direction property value and vendor prefix (legacy)
    box_direction: BoxDirectionAndPrefix? = null,
    /// The flex-wrap property value and vendor prefix
    wrap: FlexWrapAndPrefix? = null,
    /// The box-lines property value and vendor prefix (legacy)
    box_lines: BoxLinesAndPrefix? = null,
    /// The flex-grow property value and vendor prefix
    grow: CSSNumberAndPrefix? = null,
    /// The box-flex property value and vendor prefix (legacy)
    box_flex: CSSNumberAndPrefix? = null,
    /// The flex-positive property value and vendor prefix (legacy)
    flex_positive: CSSNumberAndPrefix? = null,
    /// The flex-shrink property value and vendor prefix
    shrink: CSSNumberAndPrefix? = null,
    /// The flex-negative property value and vendor prefix (legacy)
    flex_negative: CSSNumberAndPrefix? = null,
    /// The flex-basis property value and vendor prefix
    basis: LengthPercentageOrAutoAndPrefix? = null,
    /// The preferred-size property value and vendor prefix (legacy)
    preferred_size: LengthPercentageOrAutoAndPrefix? = null,
    /// The order property value and vendor prefix
    order: CSSIntegerAndPrefix? = null,
    /// The box-ordinal-group property value and vendor prefix (legacy)
    box_ordinal_group: BoxOrdinalGroupAndPrefix? = null,
    /// The flex-order property value and vendor prefix (legacy)
    flex_order: CSSIntegerAndPrefix? = null,
    /// Whether any flex-related properties have been set
    has_any: bool = false,

    fn handleProperty(
        *_,
        property: *const Property,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ): bool {
        const maybeFlush = struct {
            fn maybeFlush(
                handler: *FlexHandler,
                d: *css.DeclarationList,
                ctx: *css.PropertyHandlerContext,
                comptime prop: []const u8,
                val: auto,
                vp: *const VendorPrefix,
            ) {
                // If two vendor prefixes for the same property have different
                // values, we need to flush what we have immediately to preserve order.
                if (*field := @field(handler, prop)) {
                    if (!std.@"meta".eql(field[0], val.*) && !bun.bits.contains(css.VendorPrefix, field[1], vp.*)) {
                        handler.flush(d, ctx);
                    }
                }
            }
        }.maybeFlush;

        const propertyHelper = struct {
            fn propertyHelper(
                handler: *FlexHandler,
                ctx: *css.PropertyHandlerContext,
                d: *css.DeclarationList,
                comptime prop: []const u8,
                val: auto,
                vp: *const VendorPrefix,
            ) {
                maybeFlush(handler, d, ctx, prop, val, vp);

                // Otherwise, update the value and add the prefix
                if (*field := @field(handler, prop)) {
                    field[0] = css.generic.deepClone(@TypeOf(val.*), val, ctx.allocator);
                    bun.bits.insert(css.VendorPrefix, &field[1], vp.*);
                } else {
                    @field(handler, prop) = .{
                        css.generic.deepClone(@TypeOf(val.*), val, ctx.allocator),
                        vp.*,
                    };
                    handler.has_any = true;
                }
            }
        }.propertyHelper;

        switch (property.*) {
            .@"flex-direction" => |*val| {
                if (context.targets.browsers != null) {
                    _.box_direction = null;
                    _.box_orient = null;
                }
                propertyHelper(_, context, dest, "direction", &val[0], &val[1]);
            },
            .@"box-orient" => |*val| propertyHelper(_, context, dest, "box_orient", &val[0], &val[1]),
            .@"box-direction" => |*val| propertyHelper(_, context, dest, "box_direction", &val[0], &val[1]),
            .@"flex-wrap" => |*val| {
                if (context.targets.browsers != null) {
                    _.box_lines = null;
                }
                propertyHelper(_, context, dest, "wrap", &val[0], &val[1]);
            },
            .@"box-lines" => |*val| propertyHelper(_, context, dest, "box_lines", &val[0], &val[1]),
            .@"flex-flow" => |*val| {
                if (context.targets.browsers != null) {
                    _.box_direction = null;
                    _.box_orient = null;
                }
                propertyHelper(_, context, dest, "direction", &val[0].direction, &val[1]);
                propertyHelper(_, context, dest, "wrap", &val[0].wrap, &val[1]);
            },
            .@"flex-grow" => |*val| {
                if (context.targets.browsers != null) {
                    _.box_flex = null;
                    _.flex_positive = null;
                }
                propertyHelper(_, context, dest, "grow", &val[0], &val[1]);
            },
            .@"box-flex" => |*val| propertyHelper(_, context, dest, "box_flex", &val[0], &val[1]),
            .@"flex-positive" => |*val| propertyHelper(_, context, dest, "flex_positive", &val[0], &val[1]),
            .@"flex-shrink" => |*val| {
                if (context.targets.browsers != null) {
                    _.flex_negative = null;
                }
                propertyHelper(_, context, dest, "shrink", &val[0], &val[1]);
            },
            .@"flex-negative" => |*val| propertyHelper(_, context, dest, "flex_negative", &val[0], &val[1]),
            .@"flex-basis" => |*val| {
                if (context.targets.browsers != null) {
                    _.preferred_size = null;
                }
                propertyHelper(_, context, dest, "basis", &val[0], &val[1]);
            },
            .@"flex-preferred-size" => |*val| propertyHelper(_, context, dest, "preferred_size", &val[0], &val[1]),
            .flex => |*val| {
                if (context.targets.browsers != null) {
                    _.box_flex = null;
                    _.flex_positive = null;
                    _.flex_negative = null;
                    _.preferred_size = null;
                }
                maybeFlush(_, dest, context, "grow", &val[0].grow, &val[1]);
                maybeFlush(_, dest, context, "shrink", &val[0].shrink, &val[1]);
                maybeFlush(_, dest, context, "basis", &val[0].basis, &val[1]);
                propertyHelper(_, context, dest, "grow", &val[0].grow, &val[1]);
                propertyHelper(_, context, dest, "shrink", &val[0].shrink, &val[1]);
                propertyHelper(_, context, dest, "basis", &val[0].basis, &val[1]);
            },
            .order => |*val| {
                if (context.targets.browsers != null) {
                    _.box_ordinal_group = null;
                    _.flex_order = null;
                }
                propertyHelper(_, context, dest, "order", &val[0], &val[1]);
            },
            .@"box-ordinal-group" => |*val| propertyHelper(_, context, dest, "box_ordinal_group", &val[0], &val[1]),
            .@"flex-order" => |*val| propertyHelper(_, context, dest, "flex_order", &val[0], &val[1]),
            .unparsed => |*val| {
                if (isFlexProperty(&val.property_id)) {
                    _.flush(dest, context);
                    dest.append(context.allocator, property.deepClone(context.allocator)) catch unreachable;
                } else {
                    return false;
                }
            },
            else => return false,
        }

        return true;
    }

    pub fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(dest, context);
    }

    pub fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) {
            return;
        }

        _.has_any = false;

        let direction = bun.take(&_.direction);
        let wrap = bun.take(&_.wrap);
        let grow = bun.take(&_.grow);
        let shrink = bun.take(&_.shrink);
        let basis = bun.take(&_.basis);
        let box_orient = bun.take(&_.box_orient);
        let box_direction = bun.take(&_.box_direction);
        let box_flex = bun.take(&_.box_flex);
        let box_ordinal_group = bun.take(&_.box_ordinal_group);
        let box_lines = bun.take(&_.box_lines);
        let flex_positive = bun.take(&_.flex_positive);
        let flex_negative = bun.take(&_.flex_negative);
        let preferred_size = bun.take(&_.preferred_size);
        let order = bun.take(&_.order);
        let flex_order = bun.take(&_.flex_order);

        // Legacy properties. These are only set if the final standard properties were unset.
        legacyProperty("box-orient", bun.take(&box_orient), dest, context);
        legacyProperty("box-direction", bun.take(&box_direction), dest, context);
        legacyProperty("box-ordinal-group", bun.take(&box_ordinal_group), dest, context);
        legacyProperty("box-flex", bun.take(&box_flex), dest, context);
        legacyProperty("box-lines", bun.take(&box_lines), dest, context);
        legacyProperty("flex-positive", bun.take(&flex_positive), dest, context);
        legacyProperty("flex-negative", bun.take(&flex_negative), dest, context);
        legacyProperty("flex-preferred-size", bun.take(&preferred_size), dest, context);
        legacyProperty("flex-order", bun.take(&flex_order), dest, context);

        if (val := direction) {
            const dir = val[0];
            if (targets := context.targets.browsers) {
                const prefixes = context.targets.prefixes(css.VendorPrefix.NONE, css.prefixes.Feature.flex_direction);
                let prefixes_2009 = css.VendorPrefix{};
                if (isFlex2009(targets)) {
                    prefixes_2009.webkit = true;
                }
                if (prefixes.moz) {
                    prefixes_2009.moz = true;
                }
                if (!prefixes_2009.isEmpty()) {
                    const orient_dir = dir.to2009();
                    const orient = orient_dir[0];
                    const newdir = orient_dir[1];
                    bun.handleOom(dest.append(context.allocator, Property{ .@"box-orient" = .{ orient, prefixes_2009 } }));
                    bun.handleOom(dest.append(context.allocator, Property{ .@"box-direction" = .{ newdir, prefixes_2009 } }));
                }
            }
        }

        if (direction != null && wrap != null) {
            const dir = &direction.?[0];
            const dir_prefix = &direction.?[1];
            const wrapinner = &wrap.?[0];
            const wrap_prefix = &wrap.?[1];

            const intersection = dir_prefix.bitwiseAnd(wrap_prefix.*);
            if (!intersection.isEmpty()) {
                let prefix = context.targets.prefixes(intersection, css.prefixes.Feature.flex_flow);
                // Firefox only implemented the 2009 spec prefixed.
                prefix.moz = false;
                dest.append(context.allocator, Property{ .@"flex-flow" = .{
                    FlexFlow{
                        direction: dir.*,
                        wrap: wrapinner.*,
                    },
                    prefix,
                } }) catch |e| bun.handleOom(e);
                bun.bits.remove(css.VendorPrefix, dir_prefix, intersection);
                bun.bits.remove(css.VendorPrefix, wrap_prefix, intersection);
            }
        }

        singleProperty("flex-direction", bun.take(&direction), null, null, dest, context, "flex_direction");
        singleProperty("flex-wrap", bun.take(&wrap), null, .{ BoxLines, "box-lines" }, dest, context, "flex_wrap");

        if (targets := context.targets.browsers) {
            if (val := grow) {
                const g = val[0];
                const prefixes = context.targets.prefixes(css.VendorPrefix.NONE, css.prefixes.Feature.flex_grow);
                let prefixes_2009 = css.VendorPrefix{};
                if (isFlex2009(targets)) {
                    prefixes_2009.webkit = true;
                }
                if (prefixes.moz) {
                    prefixes_2009.moz = true;
                }
                if (!prefixes_2009.isEmpty()) {
                    bun.handleOom(dest.append(context.allocator, Property{ .@"box-flex" = .{ g, prefixes_2009 } }));
                }
            }
        }

        if (grow != null && shrink != null && basis != null) {
            const g = grow.?[0];
            const g_prefix = &grow.?[1];
            const s = shrink.?[0];
            const s_prefix = &shrink.?[1];
            const b = basis.?[0];
            const b_prefix = &basis.?[1];

            const intersection = g_prefix.bitwiseAnd(s_prefix.bitwiseAnd(b_prefix.*));
            if (!intersection.isEmpty()) {
                let prefix = context.targets.prefixes(intersection, css.prefixes.Feature.flex);
                // Firefox only implemented the 2009 spec prefixed.
                prefix.moz = false;
                dest.append(context.allocator, Property{ .flex = .{
                    Flex{
                        grow: g,
                        shrink: s,
                        basis: b,
                    },
                    prefix,
                } }) catch |e| bun.handleOom(e);
                bun.bits.remove(css.VendorPrefix, g_prefix, intersection);
                bun.bits.remove(css.VendorPrefix, s_prefix, intersection);
                bun.bits.remove(css.VendorPrefix, b_prefix, intersection);
            }
        }

        singleProperty("flex-grow", bun.take(&grow), "flex-positive", null, dest, context, "flex_grow");
        singleProperty("flex-shrink", bun.take(&shrink), "flex-negative", null, dest, context, "flex_shrink");
        singleProperty("flex-basis", bun.take(&basis), "flex-preferred-size", null, dest, context, "flex_basis");
        singleProperty("order", bun.take(&order), "flex-order", .{ BoxOrdinalGroup, "box-ordinal-group" }, dest, context, "order");
    }

    static fn singleProperty(
        comptime prop: []const u8,
        key: auto,
        comptime prop_2012: StrAlias?,
        comptime prop_2009: Prop2009?,
        dest: *css.DeclarationList,
        ctx: *css.PropertyHandlerContext,
        comptime feature_name: []const u8,
    ) {
        if (value := key) {
            const val = value[0];
            let prefix = value[1];
            if (!prefix.isEmpty()) {
                prefix = ctx.targets.prefixes(prefix, @field(css.prefixes.Feature, feature_name));
                if (p2009 := comptime prop_2009) {
                    if (prefix.none) {
                        // 2009 spec, implemented by webkit and firefox
                        if (targets := ctx.targets.browsers) {
                            let prefixes_2009 = css.VendorPrefix{};
                            if (isFlex2009(targets)) {
                                prefixes_2009.webkit = true;
                            }
                            if (prefix.moz) {
                                prefixes_2009.moz = true;
                            }
                            if (!prefixes_2009.isEmpty()) {
                                const s = brk: {
                                    const T = comptime p2009[0];
                                    if (comptime T == BoxOrdinalGroup) break :brk @as(i32?, val);
                                    break :brk p2009[0].fromStandard(&val);
                                };
                                if (v := s) {
                                    dest.append(ctx.allocator, @unionInit(Property, p2009[1], .{
                                        v,
                                        prefixes_2009,
                                    })) catch |e| bun.handleOom(e);
                                }
                            }
                        }
                    }
                }

                if (p2012 := comptime prop_2012) {
                    let ms = true;
                    if (prefix.ms) {
                        dest.append(ctx.allocator, @unionInit(Property, p2012, .{
                            val,
                            css.VendorPrefix.MS,
                        })) catch |e| bun.handleOom(e);
                        ms = false;
                    }

                    if (!ms) {
                        prefix.ms = false;
                    }
                }

                // Firefox only implemented the 2009 spec prefixed.
                prefix.moz = false;
                dest.append(ctx.allocator, @unionInit(Property, prop, .{
                    val,
                    prefix,
                })) catch |e| bun.handleOom(e);
            }
        }
    }

    static fn legacyProperty(comptime field_name: []const u8, key: auto, dest: *css.DeclarationList, ctx: *css.PropertyHandlerContext) {
        if (value := key) {
            const val = value[0];
            const prefix = value[1];
            if (!prefix.isEmpty()) {
                dest.append(ctx.allocator, @unionInit(Property, field_name, .{
                    val,
                    prefix,
                })) catch |e| bun.handleOom(e);
            } else {
                // css.generic.eql(comptime T: type, lhs: *const T, rhs: *const T)
                // css.generic.deinit(@TypeOf(val), &val, ctx.allocator);
            }
        }
    }

    static fn isFlexProperty(property_id: *const PropertyId): bool {
        return switch (property_id.*) {
            .@"flex-direction",
            .@"box-orient",
            .@"box-direction",
            .@"flex-wrap",
            .@"box-lines",
            .@"flex-flow",
            .@"flex-grow",
            .@"box-flex",
            .@"flex-positive",
            .@"flex-shrink",
            .@"flex-negative",
            .@"flex-basis",
            .@"flex-preferred-size",
            .flex,
            .order,
            .@"box-ordinal-group",
            .@"flex-order" => true,
            else => false,
        };
    }
}

import bun from "bun";
const Allocator = std.mem.Allocator;
