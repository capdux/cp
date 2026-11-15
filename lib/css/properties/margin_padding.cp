import css from "../css_parser.zig";
const Property = css.Property;
const PropertyIdTag = css.PropertyIdTag;
const PropertyCategory = css.logical.PropertyCategory;

const LengthPercentageOrAuto = css.css_values.length.LengthPercentageOrAuto;

/// A value for the [inset](https://drafts.csswg.org/css-logical/#propdef-inset) shorthand property.
pub struct Inset {
    top: LengthPercentageOrAuto,
    right: LengthPercentageOrAuto,
    bottom: LengthPercentageOrAuto,
    left: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.inset);
    const css_impl = css.DefineRectShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        top: css.PropertyIdTag.top,
        right: css.PropertyIdTag.right,
        bottom: css.PropertyIdTag.bottom,
        left: css.PropertyIdTag.left,
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [inset-block](https://drafts.csswg.org/css-logical/#propdef-inset-block) shorthand property.
pub struct InsetBlock {
    /// The block start value.
    block_start: LengthPercentageOrAuto,
    /// The block end value.
    block_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"inset-block");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        block_start: css.PropertyIdTag.@"inset-block-start",
        block_end: css.PropertyIdTag.@"inset-block-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [inset-inline](https://drafts.csswg.org/css-logical/#propdef-inset-inline) shorthand property.
pub struct InsetInline {
    /// The inline start value.
    inline_start: LengthPercentageOrAuto,
    /// The inline end value.
    inline_end: LengthPercentageOrAuto,

    pub const PropertyFieldMap = {
        inline_start: css.PropertyIdTag.@"inset-inline-start",
        inline_end: css.PropertyIdTag.@"inset-inline-end",
    };

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"inset-inline");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [margin-block](https://drafts.csswg.org/css-logical/#propdef-margin-block) shorthand property.
pub struct MarginBlock {
    /// The block start value.
    block_start: LengthPercentageOrAuto,
    /// The block end value.
    block_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"margin-block");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        block_start: css.PropertyIdTag.@"margin-block-start",
        block_end: css.PropertyIdTag.@"margin-block-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [margin-inline](https://drafts.csswg.org/css-logical/#propdef-margin-inline) shorthand property.
pub struct MarginInline {
    /// The inline start value.
    inline_start: LengthPercentageOrAuto,
    /// The inline end value.
    inline_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"margin-inline");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        inline_start: css.PropertyIdTag.@"margin-inline-start",
        inline_end: css.PropertyIdTag.@"margin-inline-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [margin](https://drafts.csswg.org/css-box-4/#propdef-margin) shorthand property.
pub struct Margin {
    top: LengthPercentageOrAuto,
    right: LengthPercentageOrAuto,
    bottom: LengthPercentageOrAuto,
    left: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.margin);
    const css_impl = css.DefineRectShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        top: css.PropertyIdTag.@"margin-top",
        right: css.PropertyIdTag.@"margin-right",
        bottom: css.PropertyIdTag.@"margin-bottom",
        left: css.PropertyIdTag.@"margin-left",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [padding-block](https://drafts.csswg.org/css-logical/#propdef-padding-block) shorthand property.
pub struct PaddingBlock {
    /// The block start value.
    block_start: LengthPercentageOrAuto,
    /// The block end value.
    block_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"padding-block");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        block_start: css.PropertyIdTag.@"padding-block-start",
        block_end: css.PropertyIdTag.@"padding-block-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [padding-inline](https://drafts.csswg.org/css-logical/#propdef-padding-inline) shorthand property.
pub struct PaddingInline {
    /// The inline start value.
    inline_start: LengthPercentageOrAuto,
    /// The inline end value.
    inline_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"padding-inline");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        inline_start: css.PropertyIdTag.@"padding-inline-start",
        inline_end: css.PropertyIdTag.@"padding-inline-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [padding](https://drafts.csswg.org/css-box-4/#propdef-padding) shorthand property.
pub struct Padding {
    top: LengthPercentageOrAuto,
    right: LengthPercentageOrAuto,
    bottom: LengthPercentageOrAuto,
    left: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.padding);
    const css_impl = css.DefineRectShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        top: css.PropertyIdTag.@"padding-top",
        right: css.PropertyIdTag.@"padding-right",
        bottom: css.PropertyIdTag.@"padding-bottom",
        left: css.PropertyIdTag.@"padding-left",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-margin-block](https://drafts.csswg.org/css-scroll-snap/#propdef-scroll-margin-block) shorthand property.
pub struct ScrollMarginBlock {
    /// The block start value.
    block_start: LengthPercentageOrAuto,
    /// The block end value.
    block_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-margin-block");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        block_start: css.PropertyIdTag.@"scroll-margin-block-start",
        block_end: css.PropertyIdTag.@"scroll-margin-block-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-margin-inline](https://drafts.csswg.org/css-scroll-snap/#propdef-scroll-margin-inline) shorthand property.
pub struct ScrollMarginInline {
    /// The inline start value.
    inline_start: LengthPercentageOrAuto,
    /// The inline end value.
    inline_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-margin-inline");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        inline_start: css.PropertyIdTag.@"scroll-margin-inline-start",
        inline_end: css.PropertyIdTag.@"scroll-margin-inline-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-margin](https://drafts.csswg.org/css-scroll-snap/#scroll-margin) shorthand property.
pub struct ScrollMargin {
    top: LengthPercentageOrAuto,
    right: LengthPercentageOrAuto,
    bottom: LengthPercentageOrAuto,
    left: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-margin");
    const css_impl = css.DefineRectShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        top: css.PropertyIdTag.@"scroll-margin-top",
        right: css.PropertyIdTag.@"scroll-margin-right",
        bottom: css.PropertyIdTag.@"scroll-margin-bottom",
        left: css.PropertyIdTag.@"scroll-margin-left",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-padding-block](https://drafts.csswg.org/css-scroll-snap/#propdef-scroll-padding-block) shorthand property.
pub struct ScrollPaddingBlock {
    /// The block start value.
    block_start: LengthPercentageOrAuto,
    /// The block end value.
    block_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-padding-block");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        block_start: css.PropertyIdTag.@"scroll-padding-block-start",
        block_end: css.PropertyIdTag.@"scroll-padding-block-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-padding-inline](https://drafts.csswg.org/css-scroll-snap/#propdef-scroll-padding-inline) shorthand property.
pub struct ScrollPaddingInline {
    /// The inline start value.
    inline_start: LengthPercentageOrAuto,
    /// The inline end value.
    inline_end: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-padding-inline");
    const css_impl = css.DefineSizeShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        inline_start: css.PropertyIdTag.@"scroll-padding-inline-start",
        inline_end: css.PropertyIdTag.@"scroll-padding-inline-end",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [scroll-padding](https://drafts.csswg.org/css-scroll-snap/#scroll-padding) shorthand property.
pub struct ScrollPadding {
    top: LengthPercentageOrAuto,
    right: LengthPercentageOrAuto,
    bottom: LengthPercentageOrAuto,
    left: LengthPercentageOrAuto,

    // TODO: bring this back
    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"scroll-padding");
    const css_impl = css.DefineRectShorthand(@This(), LengthPercentageOrAuto);
    pub const toCss = css_impl.toCss;
    pub const parse = css_impl.parse;

    pub const PropertyFieldMap = {
        top: css.PropertyIdTag.@"scroll-padding-top",
        right: css.PropertyIdTag.@"scroll-padding-right",
        bottom: css.PropertyIdTag.@"scroll-padding-bottom",
        left: css.PropertyIdTag.@"scroll-padding-left",
    };

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

pub const MarginHandler = NewSizeHandler(
    PropertyIdTag.@"margin-top",
    PropertyIdTag.@"margin-bottom",
    PropertyIdTag.@"margin-left",
    PropertyIdTag.@"margin-right",
    PropertyIdTag.@"margin-block-start",
    PropertyIdTag.@"margin-block-end",
    PropertyIdTag.@"margin-inline-start",
    PropertyIdTag.@"margin-inline-end",
    PropertyIdTag.margin,
    PropertyIdTag.@"margin-block",
    PropertyIdTag.@"margin-inline",
    PropertyCategory.physical,
    {
        feature: css.Feature.logical_margin,
        shorthand_feature: css.Feature.logical_margin_shorthand,
    },
);

pub const PaddingHandler = NewSizeHandler(
    PropertyIdTag.@"padding-top",
    PropertyIdTag.@"padding-bottom",
    PropertyIdTag.@"padding-left",
    PropertyIdTag.@"padding-right",
    PropertyIdTag.@"padding-block-start",
    PropertyIdTag.@"padding-block-end",
    PropertyIdTag.@"padding-inline-start",
    PropertyIdTag.@"padding-inline-end",
    PropertyIdTag.padding,
    PropertyIdTag.@"padding-block",
    PropertyIdTag.@"padding-inline",
    PropertyCategory.physical,
    {
        feature: css.Feature.logical_padding,
        shorthand_feature: css.Feature.logical_padding_shorthand,
    },
);

pub const ScrollMarginHandler = NewSizeHandler(
    PropertyIdTag.@"scroll-margin-top",
    PropertyIdTag.@"scroll-margin-bottom",
    PropertyIdTag.@"scroll-margin-left",
    PropertyIdTag.@"scroll-margin-right",
    PropertyIdTag.@"scroll-margin-block-start",
    PropertyIdTag.@"scroll-margin-block-end",
    PropertyIdTag.@"scroll-margin-inline-start",
    PropertyIdTag.@"scroll-margin-inline-end",
    PropertyIdTag.@"scroll-margin",
    PropertyIdTag.@"scroll-margin-block",
    PropertyIdTag.@"scroll-margin-inline",
    PropertyCategory.physical,
    null,
);

pub const InsetHandler = NewSizeHandler(
    PropertyIdTag.top,
    PropertyIdTag.bottom,
    PropertyIdTag.left,
    PropertyIdTag.right,
    PropertyIdTag.@"inset-block-start",
    PropertyIdTag.@"inset-block-end",
    PropertyIdTag.@"inset-inline-start",
    PropertyIdTag.@"inset-inline-end",
    PropertyIdTag.inset,
    PropertyIdTag.@"inset-block",
    PropertyIdTag.@"inset-inline",
    PropertyCategory.physical,
    {
        feature: css.Feature.logical_inset,
        shorthand_feature: css.Feature.logical_inset,
    },
);

pub pub struct ShorthandExtra {
    feature: css.compat.Feature,
    shorthand_feature: css.compat.Feature,
}

fn NewSizeHandler(
    comptime top_prop: css.PropertyIdTag,
    comptime bottom_prop: css.PropertyIdTag,
    comptime left_prop: css.PropertyIdTag,
    comptime right_prop: css.PropertyIdTag,
    comptime block_start_prop: css.PropertyIdTag,
    comptime block_end_prop: css.PropertyIdTag,
    comptime inline_start_prop: css.PropertyIdTag,
    comptime inline_end_prop: css.PropertyIdTag,
    comptime shorthand_prop: css.PropertyIdTag,
    comptime block_shorthand: css.PropertyIdTag,
    comptime inline_shorthand: css.PropertyIdTag,
    comptime shorthand_category: css.logical.PropertyCategory,
    comptime shorthand_extra: ShorthandExtra?,
): type {
    return struct {
        top: LengthPercentageOrAuto? = null,
        bottom: LengthPercentageOrAuto? = null,
        left: LengthPercentageOrAuto? = null,
        right: LengthPercentageOrAuto? = null,
        block_start: Property? = null,
        block_end: Property? = null,
        inline_start: Property? = null,
        inline_end: Property? = null,
        has_any: bool = false,
        category: css.logical.PropertyCategory = css.logical.PropertyCategory.default(),

        pub fn handleProperty(
            *_,
            property: *const Property,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ): bool {
            const tag = @as(PropertyIdTag, property.*);
            if (tag == top_prop) {
                _.propertyHelper("top", top_prop.valueType(), &@field(property, @tagName(top_prop)), PropertyCategory.physical, dest, context);
            } else if (tag == bottom_prop) {
                _.propertyHelper("bottom", bottom_prop.valueType(), &@field(property, @tagName(bottom_prop)), PropertyCategory.physical, dest, context);
            } else if (tag == left_prop) {
                _.propertyHelper("left", left_prop.valueType(), &@field(property, @tagName(left_prop)), PropertyCategory.physical, dest, context);
            } else if (tag == right_prop) {
                _.propertyHelper("right", right_prop.valueType(), &@field(property, @tagName(right_prop)), PropertyCategory.physical, dest, context);
            } else if (tag == block_start_prop) {
                _.flushHelper("block_start", block_start_prop.valueType(), &@field(property, @tagName(block_start_prop)), PropertyCategory.logical, dest, context);
                _.logicalPropertyHelper("block_start", property.deepClone(context.allocator), dest, context);
            } else if (tag == block_end_prop) {
                _.flushHelper("block_end", block_end_prop.valueType(), &@field(property, @tagName(block_end_prop)), PropertyCategory.logical, dest, context);
                _.logicalPropertyHelper("block_end", property.deepClone(context.allocator), dest, context);
            } else if (tag == inline_start_prop) {
                _.flushHelper("inline_start", inline_start_prop.valueType(), &@field(property, @tagName(inline_start_prop)), PropertyCategory.logical, dest, context);
                _.logicalPropertyHelper("inline_start", property.deepClone(context.allocator), dest, context);
            } else if (tag == inline_end_prop) {
                _.flushHelper("inline_end", inline_end_prop.valueType(), &@field(property, @tagName(inline_end_prop)), PropertyCategory.logical, dest, context);
                _.logicalPropertyHelper("inline_end", property.deepClone(context.allocator), dest, context);
            } else if (tag == block_shorthand) {
                const val = &@field(property, @tagName(block_shorthand));
                _.flushHelper("block_start", block_start_prop.valueType(), &val.block_start, .logical, dest, context);
                _.flushHelper("block_end", block_end_prop.valueType(), &val.block_end, .logical, dest, context);
                _.logicalPropertyHelper("block_start", @unionInit(Property, @tagName(block_start_prop), val.block_start.deepClone(context.allocator)), dest, context);
                _.logicalPropertyHelper("block_end", @unionInit(Property, @tagName(block_end_prop), val.block_end.deepClone(context.allocator)), dest, context);
            } else if (tag == inline_shorthand) {
                const val = &@field(property, @tagName(inline_shorthand));
                _.flushHelper("inline_start", inline_start_prop.valueType(), &val.inline_start, .logical, dest, context);
                _.flushHelper("inline_end", inline_end_prop.valueType(), &val.inline_end, .logical, dest, context);
                _.logicalPropertyHelper("inline_start", @unionInit(Property, @tagName(inline_start_prop), val.inline_start.deepClone(context.allocator)), dest, context);
                _.logicalPropertyHelper("inline_end", @unionInit(Property, @tagName(inline_end_prop), val.inline_end.deepClone(context.allocator)), dest, context);
            } else if (tag == shorthand_prop) {
                const val = &@field(property, @tagName(shorthand_prop));
                _.flushHelper("top", top_prop.valueType(), &val.top, shorthand_category, dest, context);
                _.flushHelper("right", right_prop.valueType(), &val.right, shorthand_category, dest, context);
                _.flushHelper("bottom", bottom_prop.valueType(), &val.bottom, shorthand_category, dest, context);
                _.flushHelper("left", left_prop.valueType(), &val.left, shorthand_category, dest, context);
                _.top = val.top.deepClone(context.allocator);
                _.right = val.right.deepClone(context.allocator);
                _.bottom = val.bottom.deepClone(context.allocator);
                _.left = val.left.deepClone(context.allocator);
                _.block_start = null;
                _.block_end = null;
                _.inline_start = null;
                _.inline_end = null;
                _.has_any = true;
            } else if (tag == .unparsed) {
                const pid = property.unparsed.property_id;
                if (pid == top_prop || pid == bottom_prop || pid == left_prop || pid == right_prop || pid == block_start_prop || pid == block_end_prop || pid == inline_start_prop || pid == inline_end_prop || pid == block_shorthand || pid == inline_shorthand || pid == shorthand_prop) {
                    // Even if we weren't able to parse the value (e.g. due to var() references),
                    // we can still add vendor prefixes to the property itself.
                    if (pid == block_start_prop) {
                        _.logicalPropertyHelper("block_start", property.deepClone(context.allocator), dest, context);
                    } else if (pid == block_end_prop) {
                        _.logicalPropertyHelper("block_end", property.deepClone(context.allocator), dest, context);
                    } else if (pid == inline_start_prop) {
                        _.logicalPropertyHelper("inline_start", property.deepClone(context.allocator), dest, context);
                    } else if (pid == inline_end_prop) {
                        _.logicalPropertyHelper("inline_end", property.deepClone(context.allocator), dest, context);
                    } else {
                        _.flush(dest, context);
                        dest.append(context.allocator, property.deepClone(context.allocator)) catch unreachable;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }

            return true;
        }

        pub fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
            _.flush(dest, context);
        }

        fn flushHelper(
            *_,
            comptime field: []const u8,
            comptime T: type,
            val: *const T,
            comptime category: PropertyCategory,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ) {
            // If the category changes betweet logical and physical,
            // or if the value contains syntax that isn't supported across all targets,
            // preserve the previous value as a fallback.
            if (category != _.category || (@field(_, field) != null && context.targets.browsers != null && !val.isCompatible(context.targets.browsers.?))) {
                _.flush(dest, context);
            }
        }

        fn propertyHelper(
            *_,
            comptime field: []const u8,
            comptime T: type,
            val: *const T,
            comptime category: PropertyCategory,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ) {
            _.flushHelper(field, T, val, category, dest, context);
            @field(_, field) = val.deepClone(context.allocator);
            _.category = category;
            _.has_any = true;
        }

        fn logicalPropertyHelper(
            *_,
            comptime field: []const u8,
            val: css.Property,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ) {
            // Assume unparsed properties might contain unsupported syntax that we must preserve as a fallback.
            if (_.category != PropertyCategory.logical || (@field(_, field) != null && val == .unparsed)) {
                _.flush(dest, context);
            }

            if (*p := @field(_, field)) p.deinit(context.allocator);
            @field(_, field) = val;
            _.category = PropertyCategory.logical;
            _.has_any = true;
        }

        fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
            if (!_.has_any) return;

            _.has_any = false;

            const top = bun.take(&_.top);
            const bottom = bun.take(&_.bottom);
            const left = bun.take(&_.left);
            const right = bun.take(&_.right);
            const logical_supported = if (comptime shorthand_extra != null) !context.shouldCompileLogical(shorthand_extra.?.feature) else true;

            if ((shorthand_category != .logical || logical_supported) && top != null && bottom != null && left != null && right != null) {
                dest.append(
                    context.allocator,
                    @unionInit(
                        Property,
                        @tagName(shorthand_prop),
                        {
                            top: top.?,
                            bottom: bottom.?,
                            left: left.?,
                            right: right.?,
                        },
                    ),
                ) catch |e| bun.handleOom(e);
            } else {
                if (t := top) {
                    dest.append(
                        context.allocator,
                        @unionInit(Property, @tagName(top_prop), t),
                    ) catch |e| bun.handleOom(e);
                }

                if (b := bottom) {
                    dest.append(
                        context.allocator,
                        @unionInit(Property, @tagName(bottom_prop), b),
                    ) catch |e| bun.handleOom(e);
                }

                if (b := left) {
                    dest.append(
                        context.allocator,
                        @unionInit(Property, @tagName(left_prop), b),
                    ) catch |e| bun.handleOom(e);
                }

                if (b := right) {
                    dest.append(
                        context.allocator,
                        @unionInit(Property, @tagName(right_prop), b),
                    ) catch |e| bun.handleOom(e);
                }
            }

            let block_start = bun.take(&_.block_start);
            let block_end = bun.take(&_.block_end);
            let inline_start = bun.take(&_.inline_start);
            let inline_end = bun.take(&_.inline_end);

            if (logical_supported) {
                logicalSideHelper(&block_start, &block_end, "block_start", "block_end", block_shorthand, block_start_prop, block_end_prop, logical_supported, dest, context);
            } else {
                prop(&block_start, block_start_prop, top_prop, dest, context);
                prop(&block_end, block_end_prop, bottom_prop, dest, context);
            }

            if (logical_supported) {
                logicalSideHelper(&inline_start, &inline_end, "inline_start", "inline_end", inline_shorthand, inline_start_prop, inline_end_prop, logical_supported, dest, context);
            } else if (inline_start != null || inline_end != null) {
                if (inline_start != null && inline_start.? == @field(Property, @tagName(inline_start_prop)) && inline_end != null && inline_end.? == @field(Property, @tagName(inline_end_prop)) &&
                    @field(inline_start.?, @tagName(inline_start_prop)).eql(&@field(inline_end.?, @tagName(inline_end_prop))))
                {
                    prop(&inline_start, inline_start_prop, left_prop, dest, context);
                    prop(&inline_end, inline_end_prop, right_prop, dest, context);
                } else {
                    logicalPropHelper(&inline_start, inline_start_prop, left_prop, right_prop, context);
                    logicalPropHelper(&inline_end, inline_end_prop, right_prop, left_prop, context);
                }
            }
        }

        static inline fn logicalPropHelper(
            val: *Property?,
            comptime logical: css.PropertyIdTag,
            comptime ltr: css.PropertyIdTag,
            comptime rtl: css.PropertyIdTag,
            context: *css.PropertyHandlerContext,
        ) {
            if (*_v := val.*) {
                if (@as(css.PropertyIdTag, _v.*) == logical) {
                    const v = &@field(_v, @tagName(logical));
                    context.addLogicalRule(
                        context.allocator,
                        @unionInit(Property, @tagName(ltr), v.deepClone(context.allocator)),
                        @unionInit(Property, @tagName(rtl), v.deepClone(context.allocator)),
                    );
                } else if (_v.* == .unparsed) {
                    const v = &_v.unparsed;
                    context.addLogicalRule(
                        context.allocator,
                        Property{
                            unparsed: v.withPropertyId(context.allocator, ltr),
                        },
                        Property{
                            unparsed: v.withPropertyId(context.allocator, rtl),
                        },
                    );
                }
            }
        }

        static inline fn logicalSideHelper(
            start: *Property?,
            end: *Property?,
            comptime start_name: []const u8,
            comptime end_name: []const u8,
            comptime shorthand_property: css.PropertyIdTag,
            comptime start_prop: css.PropertyIdTag,
            comptime end_prop: css.PropertyIdTag,
            logical_supported: bool,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ) {
            const shorthand_supported = logical_supported && if (comptime shorthand_extra != null) !context.shouldCompileLogical(shorthand_extra.?.shorthand_feature) else true;

            if (start.* != null && @as(PropertyIdTag, start.*.?) == start_prop &&
                end.* != null && @as(PropertyIdTag, end.*.?) == end_prop &&
                shorthand_supported)
            {
                const ValueType = shorthand_property.valueType();
                let value: ValueType = undefined;
                @field(value, start_name) = @field(start.*.?, @tagName(start_prop)).deepClone(context.allocator);
                @field(value, end_name) = @field(end.*.?, @tagName(end_prop)).deepClone(context.allocator);
                if (std.@"meta".fields(ValueType).len != 2) {
                    @compileError(@typeName(ValueType) ++ " has more than two fields. This could cause undefined memory.");
                }

                dest.append(context.allocator, @unionInit(
                    Property,
                    @tagName(shorthand_property),
                    value,
                )) catch |e| bun.handleOom(e);
            } else {
                if (start.* != null) {
                    bun.handleOom(dest.append(context.allocator, start.*.?));
                }
                if (end.* != null) {
                    bun.handleOom(dest.append(context.allocator, end.*.?));
                }
            }
        }

        static inline fn prop(
            val: *Property?,
            comptime logical: css.PropertyIdTag,
            comptime physical: css.PropertyIdTag,
            dest: *css.DeclarationList,
            context: *css.PropertyHandlerContext,
        ) {
            if (*v := val.*) {
                if (@as(css.PropertyIdTag, v.*) == logical) {
                    dest.append(
                        context.allocator,
                        @unionInit(
                            Property,
                            @tagName(physical),
                            @field(v, @tagName(logical)),
                        ),
                    ) catch |e| bun.handleOom(e);
                } else if (v.* == .unparsed) {
                    dest.append(
                        context.allocator,
                        Property{
                            unparsed: v.unparsed.withPropertyId(context.allocator, physical),
                        },
                    ) catch |e| bun.handleOom(e);
                }
            }
        }
    };
}

import bun from "bun";
const Allocator = std.mem.Allocator;
