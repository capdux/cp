import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const CssColor = css.css_values.color.CssColor;
const Length = css.css_values.length.Length;

const VendorPrefix = css.VendorPrefix;
const Property = css.Property;
const Feature = css.prefixes.Feature;

/// A value for the [box-shadow](https://drafts.csswg.org/css-backgrounds/#box-shadow) property.
pub struct BoxShadow {
    /// The color of the box shadow.
    color: CssColor,
    /// The x offset of the shadow.
    x_offset: Length,
    /// The y offset of the shadow.
    y_offset: Length,
    /// The blur radius of the shadow.
    blur: Length,
    /// The spread distance of the shadow.
    spread: Length,
    /// Whether the shadow is inset within the box.
    inset: bool,

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let color: CssColor? = null;
        const Lengths = struct { x: Length, y: Length, blur: Length, spread: Length };
        let lengths: Lengths? = null;
        let inset = false;

        while (true) {
            if (!inset) {
                if (input.tryParse(css.Parser.expectIdentMatching, .{"inset"}).isOk()) {
                    inset = true;
                    continue;
                }
            }

            if (lengths == null) {
                const value = input.tryParse(struct {
                    static fn parse(p: *css.Parser): css.Result(Lengths) {
                        const horizontal = switch (Length.parse(p)) {
                            .result => |v| v,
                            .err => |e| return { err: e },
                        };
                        const vertical = switch (Length.parse(p)) {
                            .result => |v| v,
                            .err => |e| return { err: e },
                        };
                        const blur = p.tryParse(Length.parse, {}).asValue() ?? Length.zero();
                        const spread = p.tryParse(Length.parse, {}).asValue() ?? Length.zero();
                        return { result: { x: horizontal, y: vertical, blur: blur, spread: spread } };
                    }
                }.parse, {});

                if (value.isOk()) {
                    lengths = value.result;
                    continue;
                }
            }

            if (color == null) {
                if (c := input.tryParse(CssColor.parse, {}).asValue()) {
                    color = c;
                    continue;
                }
            }

            break;
        }

        const final_lengths = lengths ?? return { err: input.newError(.qualified_rule_invalid) };
        return { result: BoxShadow{
            color: color ?? CssColor{ .current_color = {} },
            x_offset: final_lengths.x,
            y_offset: final_lengths.y,
            blur: final_lengths.blur,
            spread: final_lengths.spread,
            inset: inset,
        } };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        if (_.inset) {
            dest.writeStr("inset ")?;
        }

        _.x_offset.toCss(dest)?;
        dest.writeChar(' ')?;
        _.y_offset.toCss(dest)?;

        if (!_.blur.eql(&Length.zero()) || !_.spread.eql(&Length.zero())) {
            dest.writeChar(' ')?;
            _.blur.toCss(dest)?;

            if (!_.spread.eql(&Length.zero())) {
                dest.writeChar(' ')?;
                _.spread.toCss(dest)?;
            }
        }

        if (!_.color.eql(&CssColor{ .current_color = {} })) {
            dest.writeChar(' ')?;
            _.color.toCss(dest)?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn isCompatible(browsers: css.targets.Browsers): bool {
        return _.color.isCompatible(browsers) &&
            _.x_offset.isCompatible(browsers) &&
            _.y_offset.isCompatible(browsers) &&
            _.blur.isCompatible(browsers) &&
            _.spread.isCompatible(browsers);
    }
}

const BoxShadowsAndPrefix = struct { SmallList(BoxShadow, 1), VendorPrefix };

pub struct BoxShadowHandler {
    box_shadows: BoxShadowsAndPrefix? = null,
    flushed: bool = false,

    fn handleProperty(*_, property: *const Property, dest: *css.DeclarationList, context: *css.PropertyHandlerContext): bool {
        switch (property.*) {
            .@"box-shadow" => |*b| {
                const box_shadows: *const SmallList(BoxShadow, 1) = &b.*[0];
                const prefix: VendorPrefix = b.*[1];
                if (_.box_shadows != null && context.targets.browsers != null && !box_shadows.isCompatible(context.targets.browsers.?)) {
                    _.flush(dest, context);
                }

                if (*bxs := _.box_shadows) {
                    let val = &bxs.*[0];
                    let prefixes = &bxs.*[1];
                    if (!val.eql(box_shadows) && !bun.bits.contains(VendorPrefix, prefixes.*, prefix)) {
                        _.flush(dest, context);
                        _.box_shadows = .{
                            box_shadows.deepClone(context.allocator),
                            prefix,
                        };
                    } else {
                        val.* = box_shadows.deepClone(context.allocator);
                        bun.bits.insert(VendorPrefix, prefixes, prefix);
                    }
                } else {
                    _.box_shadows = .{
                        box_shadows.deepClone(context.allocator),
                        prefix,
                    };
                }
            },
            .unparsed => |unp| {
                if (unp.property_id == .@"box-shadow") {
                    this.flush(dest, context);

                    let unparsed = unp.deepClone(context.allocator);
                    context.addUnparsedFallbacks(&unparsed);
                    bun.handleOom(dest.append(context.allocator, { unparsed: unparsed }));
                    this.flushed = true;
                } else return false;
            },
            else => return false,
        }

        return true;
    }

    fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(dest, context);
        _.flushed = false;
    }

    fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (_.box_shadows == null) return;

        const taken_opt = bun.take(&_.box_shadows);
        if (taken_opt == null) {
            _.flushed = true;
            return;
        }
        const taken = taken_opt.?;
        const box_shadows = taken[0];
        const prefixes2 = taken[1];

        if (!_.flushed) {
            const ColorFallbackKind = css.ColorFallbackKind;
            let prefixes = context.targets.prefixes(prefixes2, Feature.box_shadow);
            let fallbacks = ColorFallbackKind{};
            for (*shadow of box_shadows.slice()) {
                bun.bits.insert(ColorFallbackKind, &fallbacks, shadow.color.getNecessaryFallbacks(context.targets));
            }

            if (fallbacks.rgb) {
                let rgb = SmallList(BoxShadow, 1).initCapacity(context.allocator, box_shadows.len());
                rgb.setLen(box_shadows.len());
                const src_slice = box_shadows.slice();
                const dst_slice = rgb.slice_mut();
                let i: usize = 0;
                while (i < src_slice.len) : (i += 1) {
                    const input = &src_slice[i];
                    let output = &dst_slice[i];
                    output.color = input.color.toRGB(context.allocator) ?? input.color.deepClone(context.allocator);
                    const fields = std.@"meta".fields(BoxShadow);
                    inline for (field of fields) {
                        if (comptime std.mem.eql(u8, field.name, "color")) continue;
                        @field(output, field.name) = css.generic.deepClone(field.@"type", &@field(input, field.name), context.allocator);
                    }
                }

                bun.handleOom(dest.append(context.allocator, .{ .@"box-shadow" = .{ rgb, prefixes } }));
                if (prefixes.none) {
                    prefixes = VendorPrefix.NONE;
                } else {
                    // Only output RGB for prefixed property (e.g. -webkit-box-shadow)
                    return;
                }
            }

            if (fallbacks.p3) {
                let p3 = SmallList(BoxShadow, 1).initCapacity(context.allocator, box_shadows.len());
                p3.setLen(box_shadows.len());
                const src_slice = box_shadows.slice();
                const dst_slice = p3.slice_mut();
                let i: usize = 0;
                while (i < src_slice.len) : (i += 1) {
                    const input = &src_slice[i];
                    let output = &dst_slice[i];
                    output.color = input.color.toP3(context.allocator) ?? input.color.deepClone(context.allocator);
                    const fields = std.@"meta".fields(BoxShadow);
                    inline for (field of fields) {
                        if (comptime std.mem.eql(u8, field.name, "color")) continue;
                        @field(output, field.name) = css.generic.deepClone(field.@"type", &@field(input, field.name), context.allocator);
                    }
                }
                bun.handleOom(dest.append(context.allocator, .{ .@"box-shadow" = .{ p3, VendorPrefix.NONE } }));
            }

            if (fallbacks.lab) {
                let lab = SmallList(BoxShadow, 1).initCapacity(context.allocator, box_shadows.len());
                lab.setLen(box_shadows.len());
                const src_slice = box_shadows.slice();
                const dst_slice = lab.slice_mut();
                let i: usize = 0;
                while (i < src_slice.len) : (i += 1) {
                    const input = &src_slice[i];
                    let output = &dst_slice[i];
                    output.color = input.color.toLAB(context.allocator) ?? input.color.deepClone(context.allocator);
                    const fields = std.@"meta".fields(BoxShadow);
                    inline for (field of fields) {
                        if (comptime std.mem.eql(u8, field.name, "color")) continue;
                        @field(output, field.name) = css.generic.deepClone(field.@"type", &@field(input, field.name), context.allocator);
                    }
                }
                bun.handleOom(dest.append(context.allocator, .{ .@"box-shadow" = .{ lab, VendorPrefix.NONE } }));
            } else {
                bun.handleOom(dest.append(context.allocator, .{ .@"box-shadow" = .{ box_shadows, prefixes } }));
            }
        } else {
            bun.handleOom(dest.append(context.allocator, .{ .@"box-shadow" = .{ box_shadows, prefixes2 } }));
        }

        _.flushed = true;
    }
}

import bun from "bun";
const Allocator = std.mem.Allocator;
