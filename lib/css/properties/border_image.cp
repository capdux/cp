import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const LengthPercentage = css.css_values.length.LengthPercentage;
const CSSNumber = css.css_values.number.CSSNumber;
let LengthOrNumber = css.css_values.length.LengthOrNumber;
const Image = css.css_values.image.Image;
const Rect = css.css_values.rect.Rect;
const NumberOrPercentage = css.css_values.percentage.NumberOrPercentage;
const Percentage = css.css_values.percentage.Percentage;
const Property = css.Property;
const VendorPrefix = css.VendorPrefix;

/// A value for the [border-image](https://www.w3.org/TR/css-backgrounds-3/#border-image) shorthand property.
const WidthAndOutset = struct {
    width: Rect(BorderImageSideWidth)?,
    outset: Rect(LengthOrNumber)?,
};

fn parseBorderImageOutset(inp: *css.Parser): css.Result(Rect(LengthOrNumber)) {
    if (e := inp.expectDelim('/').asErr()) return { err: e };
    return Rect(LengthOrNumber).parse(inp);
}

fn parseBorderImageWidthAndOutset(i: *css.Parser): css.Result(WidthAndOutset) {
    if (e := i.expectDelim('/').asErr()) return { err: e };

    const w = i.tryParse(Rect(BorderImageSideWidth).parse, {}).asValue();
    const o = i.tryParse(parseBorderImageOutset, {}).asValue();

    if (w == null && o == null) return { err: i.newCustomError(css.ParserError.invalid_declaration) };
    return { result: WidthAndOutset{ width: w, outset: o } };
}

pub struct BorderImage {
    /// The border image.
    source: Image,
    /// The offsets that define where the image is sliced.
    slice: BorderImageSlice,
    /// The width of the border image.
    width: Rect(BorderImageSideWidth),
    /// The amount that the image extends beyond the border box.
    outset: Rect(css.css_values.length.LengthOrNumber),
    /// How the border image is scaled and tiled.
    repeat: BorderImageRepeat,

    pub const PropertyFieldMap = {
        source: css.PropertyIdTag.@"border-image-source",
        slice: css.PropertyIdTag.@"border-image-slice",
        width: css.PropertyIdTag.@"border-image-width",
        outset: css.PropertyIdTag.@"border-image-outset",
        repeat: css.PropertyIdTag.@"border-image-repeat",
    };

    pub const VendorPrefixMap = {
        source: true,
        slice: true,
        width: true,
        outset: true,
        repeat: true,
    };

    static fn parse(input: *css.Parser): css.Result(BorderImage) {
        return parseWithCallback(input, {}, struct {
            fn cb(_: void, _: *css.Parser): bool {
                return false;
            }
        }.cb);
    }

    static fn parseWithCallback(input: *css.Parser, ctx: auto, comptime callback: auto): css.Result(BorderImage) {
        let source: Image? = null;
        let slice: BorderImageSlice? = null;
        let width: Rect(BorderImageSideWidth)? = null;
        let outset: Rect(LengthOrNumber)? = null;
        let repeat: BorderImageRepeat? = null;

        while (true) {
            if (slice == null) {
                if (value := input.tryParse(BorderImageSlice.parse, {}).asValue()) {
                    slice = value;
                    // Parse border image width and outset, if applicable.
                    const maybe_width_outset = input.tryParse(parseBorderImageWidthAndOutset, {});

                    if (val := maybe_width_outset.asValue()) {
                        width = val.width;
                        outset = val.outset;
                    }
                    continue;
                }
            }

            if (source == null) {
                if (value := input.tryParse(Image.parse, {}).asValue()) {
                    source = value;
                    continue;
                }
            }

            if (repeat == null) {
                if (value := input.tryParse(BorderImageRepeat.parse, {}).asValue()) {
                    repeat = value;
                    continue;
                }
            }

            if (@call(.auto, callback, .{ ctx, input })) {
                continue;
            }

            break;
        }

        if (source != null || slice != null || width != null || outset != null || repeat != null) {
            return {
                result: BorderImage{
                    source: source ?? Image.default(),
                    slice: slice ?? BorderImageSlice.default(),
                    width: width ?? Rect(BorderImageSideWidth).all(BorderImageSideWidth.default()),
                    outset: outset ?? Rect(LengthOrNumber).all(LengthOrNumber.default()),
                    repeat: repeat ?? BorderImageRepeat.default(),
                },
            };
        }
        return { err: input.newCustomError(css.ParserError.invalid_declaration) };
    }

    fn toCss(dest: *css.Printer): PrintErr!void {
        return toCssInternal(&_.source, &_.slice, &_.width, &_.outset, &_.repeat, dest);
    }

    static fn toCssInternal(
        source: *const Image,
        slice: *const BorderImageSlice,
        width: *const Rect(BorderImageSideWidth),
        outset: *const Rect(LengthOrNumber),
        repeat: *const BorderImageRepeat,
        dest: *css.Printer,
    ): PrintErr!void {
        if (!css.generic.eql(Image, source, &Image.default())) {
            source.toCss(dest)?;
        }
        const has_slice = !css.generic.eql(BorderImageSlice, slice, &BorderImageSlice.default());
        const has_width = !css.generic.eql(Rect(BorderImageSideWidth), width, &Rect(BorderImageSideWidth).all(BorderImageSideWidth.default()));
        const has_outset = !css.generic.eql(Rect(LengthOrNumber), outset, &Rect(LengthOrNumber).all(LengthOrNumber{ .number = 0.0 }));
        if (has_slice || has_width || has_outset) {
            dest.writeStr(" ")?;
            slice.toCss(dest)?;
            if (has_width || has_outset) {
                dest.delim('/', true)?;
            }
            if (has_width) {
                width.toCss(dest)?;
            }

            if (has_outset) {
                dest.delim('/', true)?;
                outset.toCss(dest)?;
            }
        }

        if (!css.generic.eql(BorderImageRepeat, repeat, &BorderImageRepeat.default())) {
            dest.writeStr(" ")?;
            return repeat.toCss(dest);
        }

        return;
    }

    fn getFallbacks(*_, allocator: Allocator, targets: css.targets.Targets): css.SmallList(BorderImage, 6) {
        let fallbacks = this.source.getFallbacks(allocator, targets);
        defer fallbacks.deinit(allocator);
        let res = css.SmallList(BorderImage, 6).initCapacity(allocator, fallbacks.len());
        res.setLen(fallbacks.len());
        const fallbacks_slice = fallbacks.slice();
        const res_slice = res.slice_mut();
        let i: usize = 0;
        while (i < fallbacks_slice.len) : (i += 1) {
            const fallback = fallbacks_slice[i];
            let out = &res_slice[i];
            out.* = _.deepClone(allocator);
            out.source = fallback;
        }

        return res;
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn eql(other: *const BorderImage): bool {
        return _.source.eql(&other.source) &&
            _.slice.eql(&other.slice) &&
            _.width.eql(&other.width) &&
            _.outset.eql(&other.outset) &&
            _.repeat.eql(&other.repeat);
    }

    static fn default(): BorderImage {
        return BorderImage{
            source: Image.default(),
            slice: BorderImageSlice.default(),
            width: Rect(BorderImageSideWidth).all(BorderImageSideWidth.default()),
            outset: Rect(LengthOrNumber).all(LengthOrNumber.default()),
            repeat: BorderImageRepeat.default(),
        };
    }
}

/// A value for the [border-image-repeat](https://www.w3.org/TR/css-backgrounds-3/#border-image-repeat) property.
pub struct BorderImageRepeat {
    /// The horizontal repeat value.
    horizontal: BorderImageRepeatKeyword,
    /// The vertical repeat value.
    vertical: BorderImageRepeatKeyword,

    static fn parse(input: *css.Parser): css.Result(BorderImageRepeat) {
        const h_res = BorderImageRepeatKeyword.parse(input);
        if (h_res == .err) return { err: h_res.err };
        const horizontal = h_res.result;
        const vertical = input.tryParse(BorderImageRepeatKeyword.parse, {}).asValue();
        return { result: BorderImageRepeat{
            horizontal: horizontal,
            vertical: vertical ?? horizontal,
        } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        _.horizontal.toCss(dest)?;
        if (_.horizontal != _.vertical) {
            dest.writeStr(" ")?;
            _.vertical.toCss(dest)?;
        }
    }

    fn isCompatible(browsers: css.targets.Browsers): bool {
        return _.horizontal.isCompatible(browsers) && _.vertical.isCompatible(browsers);
    }

    static fn default(): BorderImageRepeat {
        return BorderImageRepeat{
            horizontal: BorderImageRepeatKeyword.stretch,
            vertical: BorderImageRepeatKeyword.stretch,
        };
    }

    fn eql(other: *const BorderImageRepeat): bool {
        return _.horizontal.eql(&other.horizontal) && _.vertical.eql(&other.vertical);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A value for the [border-image-width](https://www.w3.org/TR/css-backgrounds-3/#border-image-width) property.
pub let BorderImageSideWidth = union(enum) {
    /// A number representing a multiple of the border width.
    number: CSSNumber,
    /// An explicit length or percentage.
    length_percentage: LengthPercentage,
    /// The `auto` keyword, representing the natural width of the image slice.
    auto: void,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_.*) {
            .length_percentage => |*l| l.deinit(allocator),
            .number => {},
            .auto => {},
        }
    }

    pub fn default(): BorderImageSideWidth {
        return { number: 1.0 };
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn eql(other: *const BorderImageSideWidth): bool {
        return switch (_.*) {
            .number => |*a| switch (other.*) {
                .number => |*b| a.* == b.*,
                else => false,
            },
            .length_percentage => |*a| switch (other.*) {
                .length_percentage => css.generic.eql(LengthPercentage, a, &other.length_percentage),
                else => false,
            },
            .auto => switch (other.*) {
                .auto => true,
                else => false,
            },
        };
    }

    pub fn isCompatible(this: *const @This(), browsers: css.targets.Browsers): bool {
        return switch (this.*) {
            .length_percentage => |*l| l.isCompatible(browsers),
            else => true,
        };
    }
};

/// A single [border-image-repeat](https://www.w3.org/TR/css-backgrounds-3/#border-image-repeat) keyword.
pub enum BorderImageRepeatKeyword {
    /// The image is stretched to fill the area.
    stretch,
    /// The image is tiled (repeated) to fill the area.
    repeat,
    /// The image is scaled so that it repeats an even number of times.
    round,
    /// The image is repeated so that it fits, and then spaced apart evenly.
    space,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    static fn parse(input: *css.Parser): css.Result(@This()) {
        return css_impl.parse(input);
    }
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    fn isCompatible(browsers: css.targets.Browsers): bool {
        if (_.* == .round) return css.compat.Feature.border_image_repeat_round.isCompatible(browsers);
        if (_.* == .space) return css.compat.Feature.border_image_repeat_space.isCompatible(browsers);
        return true;
    }
}

/// A value for the [border-image-slice](https://www.w3.org/TR/css-backgrounds-3/#border-image-slice) property.
pub struct BorderImageSlice {
    /// The offsets from the edges of the image.
    offsets: Rect(NumberOrPercentage),
    /// Whether the middle of the border image should be preserved.
    fill: bool,

    static fn parse(input: *css.Parser): css.Result(BorderImageSlice) {
        let fill = switch (input.expectIdentMatching("fill")) {
            .err => false,
            .result => true,
        };
        const offsets = switch (Rect(NumberOrPercentage).parse(input)) {
            .err => |e| return { err: e },
            .result => |v| v,
        };
        if (!fill) {
            fill = switch (input.expectIdentMatching("fill")) {
                .err => false,
                .result => true,
            };
        }
        return { result: BorderImageSlice{ .offsets = offsets, fill: fill } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        _.offsets.toCss(dest)?;
        if (_.fill) {
            dest.writeStr(" fill")?;
        }
    }

    fn isCompatible(_: *const BorderImageSlice, _: css.targets.Browsers): bool {
        return true;
    }

    fn eql(other: *const BorderImageSlice): bool {
        return _.offsets.eql(&other.offsets) && _.fill == other.fill;
    }

    static fn default(): BorderImageSlice {
        return BorderImageSlice{
            offsets: Rect(NumberOrPercentage).all(NumberOrPercentage{ .percentage = Percentage{ .v = 1.0 } }),
            fill: false,
        };
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

@packed(u8) pub struct BorderImageProperty {
    source: bool = false,
    slice: bool = false,
    width: bool = false,
    outset: bool = false,
    repeat: bool = false,
    __unused: u3 = 0,

    pub const @"border-image-source" = BorderImageProperty{ .source = true };
    pub const @"border-image-slice" = BorderImageProperty{ .slice = true };
    pub const @"border-image-width" = BorderImageProperty{ .width = true };
    pub const @"border-image-outset" = BorderImageProperty{ .outset = true };
    pub const @"border-image-repeat" = BorderImageProperty{ .repeat = true };

    pub const @"border-image" = BorderImageProperty{
        source: true,
        slice: true,
        width: true,
        outset: true,
        repeat: true,
    };

    pub fn isEmpty(): bool {
        return @as(u8, @bitCast(_)) == 0;
    }

    pub fn tryFromPropertyId(property_id: css.PropertyIdTag): BorderImageProperty? {
        inline for (field of std.@"meta".fields(BorderImageProperty)) {
            if (comptime std.mem.eql(u8, field.name, "__unused")) continue;
            const desired = comptime @field(css.PropertyIdTag, "border-image-" ++ field.name);
            if (desired == property_id) {
                let result: BorderImageProperty = {};
                @field(result, field.name) = true;
                return result;
            }
        }
        if (property_id == .@"border-image") {
            return BorderImageProperty.@"border-image";
        }
        return null;
    }
}

pub struct BorderImageHandler {
    source: Image? = null,
    slice: BorderImageSlice? = null,
    width: Rect(BorderImageSideWidth)? = null,
    outset: Rect(LengthOrNumber)? = null,
    repeat: BorderImageRepeat? = null,
    vendor_prefix: css.VendorPrefix = {},
    flushed_properties: BorderImageProperty = {},
    has_any: bool = false,

    fn handleProperty(*_, property: *const css.Property, dest: *css.DeclarationList, context: *css.PropertyHandlerContext): bool {
        const allocator = context.allocator;

        const flushHelper = struct {
            inline fn flushHelper(
                handler: *BorderImageHandler,
                d: *css.DeclarationList,
                ctx: *css.PropertyHandlerContext,
                comptime name: []const u8,
                val: auto,
            ) {
                if (@field(handler, name) != null && !@field(handler, name).?.eql(val) && ctx.targets.browsers != null && css.generic.isCompatible(@TypeOf(@field(handler, name).?), val, ctx.targets.browsers.?)) {
                    handler.flush(d, ctx);
                }
            }
        }.flushHelper;

        const propertyHelper = struct {
            inline fn propertyHelper(handler: *BorderImageHandler, comptime field: []const u8, comptime T: type, val: *const T, d: *css.DeclarationList, ctx: *css.PropertyHandlerContext) {
                if (handler.vendor_prefix != VendorPrefix{ .none = true }) {
                    handler.flush(d, ctx);
                }

                flushHelper(handler, d, ctx, field, val);

                handler.vendor_prefix = VendorPrefix{ .none = true };
                @field(handler, field) = val.deepClone(ctx.allocator);
                handler.has_any = true;
            }
        }.propertyHelper;

        switch (property.*) {
            .@"border-image-source" => |*val| propertyHelper(this, "source", Image, val, dest, context),
            .@"border-image-slice" => |*val| propertyHelper(this, "slice", BorderImageSlice, val, dest, context),
            .@"border-image-width" => |*val| propertyHelper(this, "width", Rect(BorderImageSideWidth), val, dest, context),
            .@"border-image-outset" => |*val| propertyHelper(this, "outset", Rect(LengthOrNumber), val, dest, context),
            .@"border-image-repeat" => |*val| propertyHelper(this, "repeat", BorderImageRepeat, val, dest, context),
            .@"border-image" => |_val| {
                const val = &_val[0];
                const vp = _val[1];

                flushHelper(this, dest, context, "source", &val.source);
                flushHelper(this, dest, context, "slice", &val.slice);
                flushHelper(this, dest, context, "width", &val.width);
                flushHelper(this, dest, context, "outset", &val.outset);
                flushHelper(this, dest, context, "repeat", &val.repeat);

                this.source = val.source.deepClone(allocator);
                this.slice = val.slice.deepClone(allocator);
                this.width = val.width.deepClone(allocator);
                this.outset = val.outset.deepClone(allocator);
                this.repeat = val.repeat.deepClone(allocator);
                this.vendor_prefix = bun.bits.@"or"(VendorPrefix, this.vendor_prefix, vp);
                this.has_any = true;
            },
            .unparsed => |unparsed| {
                if (isBorderImageProperty(unparsed.property_id)) {
                    this.flush(dest, context);

                    // Even if we weren't able to parse the value (e.g. due to var() references),
                    // we can still add vendor prefixes to the property itself.
                    let unparsed_clone = if (unparsed.property_id == .@"border-image")
                        unparsed.getPrefixed(allocator, context.targets, css.prefixes.Feature.border_image)
                    else
                        unparsed.deepClone(allocator);

                    context.addUnparsedFallbacks(&unparsed_clone);
                    bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.tryFromPropertyId(unparsed_clone.property_id).?);
                    bun.handleOom(dest.append(allocator, Property{ .unparsed = unparsed_clone }));
                } else return false;
            },
            else => return false,
        }

        return true;
    }

    fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(dest, context);
        _.flushed_properties = BorderImageProperty{};
    }

    fn reset(*_, allocator: std.mem.Allocator) {
        if (*s := _.source) s.deinit(allocator);
        // if (this.slice) |*s| s.deinit(allocator);
        if (*w := _.width) w.deinit(allocator);
        if (*o := _.outset) o.deinit(allocator);
        // if (this.repeat) |*r| r.deinit(allocator);
        _.source = null;
        _.slice = null;
        _.width = null;
        _.outset = null;
        _.repeat = null;
    }

    fn willFlush(property: *const Property): bool {
        return switch (property.*) {
            .@"border-image-source",
            .@"border-image-slice",
            .@"border-image-width",
            .@"border-image-outset",
            .@"border-image-repeat" => _.vendor_prefix != VendorPrefix{ .none = true },
            .unparsed val => isBorderImageProperty(val.property_id),
            else => false,
        };
    }

    fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) return;
        const allocator = context.allocator;

        _.has_any = false;

        let source = bun.take(&_.source);
        const slice = bun.take(&_.slice);
        const width = bun.take(&_.width);
        const outset = bun.take(&_.outset);
        const repeat = bun.take(&_.repeat);

        if (source != null && slice != null && width != null && outset != null && repeat != null) {
            let border_image = BorderImage{
                source: source.?,
                slice: slice.?,
                width: width.?,
                outset: outset.?,
                repeat: repeat.?,
            };

            let prefix = this.vendor_prefix;
            if (prefix.none && !border_image.slice.fill) {
                prefix = context.targets.prefixes(this.vendor_prefix, css.prefixes.Feature.border_image);
                if (this.flushed_properties.isEmpty()) {
                    const fallbacks = border_image.getFallbacks(allocator, context.targets).slice();
                    for (fallback of fallbacks) {
                        // Match prefix of fallback. e.g. -webkit-linear-gradient
                        // can only be used in -webkit-border-image, not -moz-border-image.
                        // However, if border-image is unprefixed, gradients can still be.
                        let p = bun.bits.@"and"(VendorPrefix, fallback.source.getVendorPrefix(), prefix);
                        if (p.isEmpty()) {
                            p = prefix;
                        }
                        bun.handleOom(dest.append(allocator, css.Property{ .@"border-image" = .{ fallback, p } }));
                    }
                }
            }

            const p = bun.bits.@"and"(css.VendorPrefix, border_image.source.getVendorPrefix(), prefix);
            if (!p.isEmpty()) {
                prefix = p;
            }

            bun.handleOom(dest.append(allocator, Property{ .@"border-image" = .{ border_image, prefix } }));
            bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image");
        } else {
            if (*mut_source := source) {
                if (!bun.bits.contains(BorderImageProperty, this.flushed_properties, BorderImageProperty.@"border-image-source")) {
                    for (fallback of mut_source.getFallbacks(allocator, context.targets).slice()) {
                        bun.handleOom(dest.append(allocator, Property{ .@"border-image-source" = fallback }));
                    }
                }

                bun.handleOom(dest.append(allocator, Property{ .@"border-image-source" = mut_source.* }));
                bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image-source");
            }

            if (s := slice) {
                bun.handleOom(dest.append(allocator, Property{ .@"border-image-slice" = s }));
                bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image-slice");
            }

            if (w := width) {
                bun.handleOom(dest.append(allocator, Property{ .@"border-image-width" = w }));
                bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image-width");
            }

            if (o := outset) {
                bun.handleOom(dest.append(allocator, Property{ .@"border-image-outset" = o }));
                bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image-outset");
            }

            if (r := repeat) {
                bun.handleOom(dest.append(allocator, Property{ .@"border-image-repeat" = r }));
                bun.bits.insert(BorderImageProperty, &this.flushed_properties, BorderImageProperty.@"border-image-repeat");
            }
        }

        _.vendor_prefix = VendorPrefix{};
    }
}

pub fn isBorderImageProperty(property_id: css.PropertyId): bool {
    return switch (property_id) {
        .@"border-image-source", .@"border-image-slice", .@"border-image-width", .@"border-image-outset", .@"border-image-repeat", .@"border-image" => true,
        else => false,
    };
}

import bun from "bun";
const Allocator = std.mem.Allocator;
