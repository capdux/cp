import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;

const LengthPercentage = css.css_values.length.LengthPercentage;
const Size2D = css.css_values.size.Size2D;
const Rect = css.css_values.rect.Rect;
const Property = css.Property;
const VendorPrefix = css.VendorPrefix;
const PropertyIdTag = css.PropertyIdTag;

/// A value for the [border-radius](https://www.w3.org/TR/css-backgrounds-3/#border-radius) property.
pub struct BorderRadius {
    /// The x and y radius values for the top left corner.
    top_left: Size2D(LengthPercentage),
    /// The x and y radius values for the top right corner.
    top_right: Size2D(LengthPercentage),
    /// The x and y radius values for the bottom right corner.
    bottom_right: Size2D(LengthPercentage),
    /// The x and y radius values for the bottom left corner.
    bottom_left: Size2D(LengthPercentage),

    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"border-radius", PropertyFieldMap);

    pub const PropertyFieldMap = {
        top_left: "border-top-left-radius",
        top_right: "border-top-right-radius",
        bottom_right: "border-bottom-right-radius",
        bottom_left: "border-bottom-left-radius",
    };

    pub const VendorPrefixMap = {
        top_left: true,
        top_right: true,
        bottom_right: true,
        bottom_left: true,
    };

    static fn parse(input: *css.Parser): css.Result(BorderRadius) {
        const widths = switch (Rect(LengthPercentage).parse(input)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        const heights = if (input.tryParse(css.Parser.expectDelim, .{'/'}).isOk())
            switch (Rect(LengthPercentage).parse(input)) {
                .result => |v| v,
                .err => |e| {
                    widths.deinit(input.allocator());
                    return { err: e };
                },
            }
        else
            widths.deepClone(input.allocator());

        const Size2DLP = Size2D(LengthPercentage);
        return {
            result: BorderRadius{
                top_left: Size2DLP{ a: widths.top, b: heights.top },
                top_right: Size2DLP{ a: widths.right, b: heights.right },
                bottom_right: Size2DLP{ a: widths.bottom, b: heights.bottom },
                bottom_left: Size2DLP{ a: widths.left, b: heights.left },
            },
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        const RectLP = Rect(*const LengthPercentage);
        const widths = RectLP{
            top: &_.top_left.a,
            right: &_.top_right.a,
            bottom: &_.bottom_right.a,
            left: &_.bottom_left.a,
        };

        const heights = RectLP{
            top: &_.top_left.b,
            right: &_.top_right.b,
            bottom: &_.bottom_right.b,
            left: &_.bottom_left.b,
        };

        widths.toCss(dest)?;

        if (!widths.eql(&heights)) {
            dest.delim('/', true)?;
            heights.toCss(dest)?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

const RadiusAndPrefix = struct { Size2D(LengthPercentage), css.VendorPrefix };

pub struct BorderRadiusHandler {
    top_left: RadiusAndPrefix? = null,
    top_right: RadiusAndPrefix? = null,
    bottom_right: RadiusAndPrefix? = null,
    bottom_left: RadiusAndPrefix? = null,
    start_start: css.Property? = null,
    start_end: css.Property? = null,
    end_end: css.Property? = null,
    end_start: css.Property? = null,
    category: css.PropertyCategory = .physical,
    has_any: bool = false,

    fn handleProperty(*_, property: *const css.Property, dest: *css.DeclarationList, context: *css.PropertyHandlerContext): bool {
        switch (property.*) {
            .@"border-top-left-radius" => |val| propertyHelper(_, dest, context, "top_left", &val[0], val[1]),
            .@"border-top-right-radius" => |val| propertyHelper(_, dest, context, "top_right", &val[0], val[1]),
            .@"border-bottom-right-radius" => |val| propertyHelper(_, dest, context, "bottom_right", &val[0], val[1]),
            .@"border-bottom-left-radius" => |val| propertyHelper(_, dest, context, "bottom_left", &val[0], val[1]),
            .@"border-start-start-radius" => logicalPropertyHelper(this, dest, context, "start_start", property),
            .@"border-start-end-radius" => logicalPropertyHelper(this, dest, context, "start_end", property),
            .@"border-end-end-radius" => logicalPropertyHelper(this, dest, context, "end_end", property),
            .@"border-end-start-radius" => logicalPropertyHelper(this, dest, context, "end_start", property),
            .@"border-radius" => |val| {
                this.start_start = null;
                this.start_end = null;
                this.end_end = null;
                this.end_start = null;

                maybeFlush(this, dest, context, "top_left", &val[0].top_left, val[1]);
                maybeFlush(this, dest, context, "top_right", &val[0].top_right, val[1]);
                maybeFlush(this, dest, context, "bottom_right", &val[0].bottom_right, val[1]);
                maybeFlush(this, dest, context, "bottom_left", &val[0].bottom_left, val[1]);

                propertyHelper(this, dest, context, "top_left", &val[0].top_left, val[1]);
                propertyHelper(this, dest, context, "top_right", &val[0].top_right, val[1]);
                propertyHelper(this, dest, context, "bottom_right", &val[0].bottom_right, val[1]);
                propertyHelper(this, dest, context, "bottom_left", &val[0].bottom_left, val[1]);
            },
            .unparsed => |unparsed| {
                if (isBorderRadiusProperty(unparsed.property_id)) {
                    // Even if we weren't able to parse the value (e.g. due to var() references),
                    // we can still add vendor prefixes to the property itself.
                    switch (unparsed.property_id) {
                        .@"border-start-start-radius" => logicalPropertyHelper(_, dest, context, "start_start", property),
                        .@"border-start-end-radius" => logicalPropertyHelper(_, dest, context, "start_end", property),
                        .@"border-end-end-radius" => logicalPropertyHelper(_, dest, context, "end_end", property),
                        .@"border-end-start-radius" => logicalPropertyHelper(_, dest, context, "end_start", property),
                        else => {
                            _.flush(dest, context);
                            bun.handleOom(dest.append(context.allocator, Property{ .unparsed = unparsed.getPrefixed(context.allocator, context.targets, css.prefixes.Feature.border_radius) }));
                        },
                    }
                } else return false;
            },
            else => return false,
        }

        return true;
    }

    pub fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(dest, context);
    }

    pub fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) return;

        _.has_any = false;

        let top_left = bun.take(&_.top_left);
        let top_right = bun.take(&_.top_right);
        let bottom_right = bun.take(&_.bottom_right);
        let bottom_left = bun.take(&_.bottom_left);
        const start_start = bun.take(&_.start_start);
        const start_end = bun.take(&_.start_end);
        const end_end = bun.take(&_.end_end);
        const end_start = bun.take(&_.end_start);

        if (top_left != null && top_right != null && bottom_right != null && bottom_left != null) {
            const intersection = top_left.?[1].bitwiseAnd(top_right.?[1]).bitwiseAnd(bottom_right.?[1]).bitwiseAnd(bottom_left.?[1]);
            if (!intersection.isEmpty()) {
                const prefix = context.targets.prefixes(intersection, css.prefixes.Feature.border_radius);
                dest.append(context.allocator, Property{ .@"border-radius" = .{
                    BorderRadius{
                        top_left: top_left.?[0].deepClone(context.allocator),
                        top_right: top_right.?[0].deepClone(context.allocator),
                        bottom_right: bottom_right.?[0].deepClone(context.allocator),
                        bottom_left: bottom_left.?[0].deepClone(context.allocator),
                    },
                    prefix,
                } }) catch |e| bun.handleOom(e);
                bun.bits.remove(VendorPrefix, &top_left.?[1], intersection);
                bun.bits.remove(VendorPrefix, &top_right.?[1], intersection);
                bun.bits.remove(VendorPrefix, &bottom_right.?[1], intersection);
                bun.bits.remove(VendorPrefix, &bottom_left.?[1], intersection);
            }
        }

        const logical_supported = !context.shouldCompileLogical(.logical_border_radius);

        singleProperty(dest, context, "border-top-left-radius", top_left);
        singleProperty(dest, context, "border-top-right-radius", top_right);
        singleProperty(dest, context, "border-bottom-right-radius", bottom_right);
        singleProperty(dest, context, "border-bottom-left-radius", bottom_left);

        logicalProperty(dest, context, start_start, "border-top-left-radius", "border-top-right-radius", logical_supported);
        logicalProperty(dest, context, start_end, "border-top-right-radius", "border-top-left-radius", logical_supported);
        logicalProperty(dest, context, end_end, "border-bottom-right-radius", "border-bottom-left-radius", logical_supported);
        logicalProperty(dest, context, end_start, "border-bottom-left-radius", "border-bottom-right-radius", logical_supported);
    }

    static fn singleProperty(d: *css.DeclarationList, ctx: *css.PropertyHandlerContext, comptime prop: []const u8, val: RadiusAndPrefix?) {
        if (v := val) {
            if (!v[1].isEmpty()) {
                const prefix = ctx.targets.prefixes(v[1], css.prefixes.Feature.border_radius);
                bun.handleOom(d.append(ctx.allocator, @unionInit(css.Property, prop, .{ v[0], prefix })));
            }
        }
    }

    static fn logicalProperty(d: *css.DeclarationList, ctx: *css.PropertyHandlerContext, val: css.Property?, comptime ltr: []const u8, comptime rtl: []const u8, logical_supported: bool) {
        if (v := val) {
            if (logical_supported) {
                bun.handleOom(d.append(ctx.allocator, v));
            } else {
                const prefix = ctx.targets.prefixes(css.VendorPrefix{ .none = true }, css.prefixes.Feature.border_radius);
                switch (v) {
                    .@"border-start-start-radius",
                    .@"border-start-end-radius",
                    .@"border-end-end-radius",
                    .@"border-end-start-radius" => |radius| {
                        ctx.addLogicalRule(
                            ctx.allocator,
                            @unionInit(css.Property, ltr, .{ radius, prefix }),
                            @unionInit(css.Property, rtl, .{ radius, prefix }),
                        );
                    },
                    .unparsed => |unparsed| {
                        ctx.addLogicalRule(
                            ctx.allocator,
                            Property{ .unparsed = unparsed.withPropertyId(ctx.allocator, @unionInit(css.PropertyId, ltr, prefix)) },
                            Property{ .unparsed = unparsed.withPropertyId(ctx.allocator, @unionInit(css.PropertyId, rtl, prefix)) },
                        );
                    },
                    else => {},
                }
            }
        }
    }

    fn maybeFlush(*_, d: *css.DeclarationList, ctx: *css.PropertyHandlerContext, comptime prop: []const u8, val: auto, vp: css.VendorPrefix) {
        // If two vendor prefixes for the same property have different
        // values, we need to flush what we have immediately to preserve order.
        if (*existing := @field(_, prop)) {
            if (!existing.*[0].eql(val) && !bun.bits.contains(VendorPrefix, existing.*[1], vp)) {
                _.flush(d, ctx);
            }
        }

        if (@field(_, prop) != null && ctx.targets.browsers != null && !css.generic.isCompatible(Size2D(LengthPercentage), val, ctx.targets.browsers.?)) {
            _.flush(d, ctx);
        }
    }

    fn propertyHelper(*_, d: *css.DeclarationList, ctx: *css.PropertyHandlerContext, comptime prop: []const u8, val: *const Size2D(LengthPercentage), vp: css.VendorPrefix) {
        if (_.category != .physical) {
            _.flush(d, ctx);
        }

        maybeFlush(_, d, ctx, prop, val, vp);

        // Otherwise, update the value and add the prefix.
        if (*existing := @field(_, prop)) {
            existing.* = .{ val.deepClone(ctx.allocator), vp };
        } else {
            @field(_, prop) = .{ val.deepClone(ctx.allocator), vp };
            _.has_any = true;
        }

        _.category = .physical;
    }

    fn logicalPropertyHelper(*_, d: *css.DeclarationList, ctx: *css.PropertyHandlerContext, comptime prop: []const u8, val: *const css.Property) {
        if (_.category != .logical) {
            _.flush(d, ctx);
        }

        @field(_, prop) = val.deepClone(ctx.allocator);
        _.category = .logical;
        _.has_any = true;
    }
}

pub fn isBorderRadiusProperty(property_id: PropertyIdTag): bool {
    if (isLogicalBorderRadiusProperty(property_id)) {
        return true;
    }

    return switch (property_id) {
        .@"border-top-left-radius",
        .@"border-top-right-radius",
        .@"border-bottom-right-radius",
        .@"border-bottom-left-radius",
        .@"border-radius" => true,
        else => false,
    };
}

pub fn isLogicalBorderRadiusProperty(property_id: PropertyIdTag): bool {
    return switch (property_id) {
        .@"border-start-start-radius",
        .@"border-start-end-radius",
        .@"border-end-end-radius",
        .@"border-end-start-radius" => true,
        else => false,
    };
}

import bun from "bun";
const Allocator = std.mem.Allocator;
