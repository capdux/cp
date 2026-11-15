import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;

const LengthPercentage = css.css_values.length.LengthPercentage;
const Image = css.css_values.image.Image;
const Rect = css.css_values.rect.Rect;
const Url = css.css_values.url.Url;
const LengthOrNumber = css.css_values.length.LengthOrNumber;
const Position = css.css_values.position.Position;

const BorderRadius = css.css_properties.border_radius.BorderRadius;
const FillRule = css.css_properties.shape.FillRule;

const BackgroundSize = css.css_properties.background.BackgroundSize;
const BackgroundRepeat = css.css_properties.background.BackgroundRepeat;
const BorderImageSlice = css.css_properties.border_image.BorderImageSlice;
const BorderImageSideWidth = css.css_properties.border_image.BorderImageSideWidth;
const BorderImageRepeat = css.css_properties.border_image.BorderImageRepeat;
const BorderImage = css.css_properties.border_image.BorderImage;

const VendorPrefix = css.VendorPrefix;

/// A value for the [clip-path](https://www.w3.org/TR/css-masking-1/#the-clip-path) property.
const ClipPath = union(enum) {
    /// No clip path.
    None,
    /// A url reference to an SVG path element.
    Url: Url,
    /// A basic shape, positioned according to the reference box.
    Shape: struct {
        /// A basic shape.
        // todo_stuff.think_about_mem_mgmt
        shape: *BasicShape,
        /// A reference box that the shape is positioned according to.
        reference_box: GeometryBox,
    },
    /// A reference box.
    Box: GeometryBox,
};

/// A [`<geometry-box>`](https://www.w3.org/TR/css-masking-1/#typedef-geometry-box) value
/// as used in the `mask-clip` and `clip-path` properties.
pub enum GeometryBox {
    /// The painted content is clipped to the content box.
    @"border-box",
    /// The painted content is clipped to the padding box.
    @"padding-box",
    /// The painted content is clipped to the border box.
    @"content-box",
    /// The painted content is clipped to the margin box.
    @"margin-box",
    /// The painted content is clipped to the object bounding box.
    @"fill-box",
    /// The painted content is clipped to the stroke bounding box.
    @"stroke-box",
    /// Uses the nearest SVG viewport as reference box.
    @"view-box",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    fn intoMaskClip(): MaskClip {
        return MaskClip{ .@"geometry-box" = _ };
    }

    static fn default(): GeometryBox {
        return .@"border-box";
    }
}

/// A CSS [`<basic-shape>`](https://www.w3.org/TR/css-shapes-1/#basic-shape-functions) value.
pub const BasicShape = union(enum) {
    /// An inset rectangle.
    Inset: InsetRect,
    /// A circle.
    Circle: Circle,
    /// An ellipse.
    Ellipse: Ellipse,
    /// A polygon.
    Polygon: Polygon,
};

/// An [`inset()`](https://www.w3.org/TR/css-shapes-1/#funcdef-inset) rectangle shape.
struct InsetRect {
    /// The rectangle.
    rect: Rect(LengthPercentage),
    /// A corner radius for the rectangle.
    radius: BorderRadius,
}

/// A [`circle()`](https://www.w3.org/TR/css-shapes-1/#funcdef-circle) shape.
pub struct Circle {
    /// The radius of the circle.
    radius: ShapeRadius,
    /// The position of the center of the circle.
    position: Position,
}

/// An [`ellipse()`](https://www.w3.org/TR/css-shapes-1/#funcdef-ellipse) shape.
pub struct Ellipse {
    /// The x-radius of the ellipse.
    radius_x: ShapeRadius,
    /// The y-radius of the ellipse.
    radius_y: ShapeRadius,
    /// The position of the center of the ellipse.
    position: Position,
}

/// A [`polygon()`](https://www.w3.org/TR/css-shapes-1/#funcdef-polygon) shape.
pub struct Polygon {
    /// The fill rule used to determine the interior of the polygon.
    fill_rule: FillRule,
    /// The points of each vertex of the polygon.
    points: ArrayList(Point),
}

/// A [`<shape-radius>`](https://www.w3.org/TR/css-shapes-1/#typedef-shape-radius) value
/// that defines the radius of a `circle()` or `ellipse()` shape.
pub const ShapeRadius = union(enum) {
    /// An explicit length or percentage.
    LengthPercentage: LengthPercentage,
    /// The length from the center to the closest side of the box.
    ClosestSide,
    /// The length from the center to the farthest side of the box.
    FarthestSide,
};

/// A point within a `polygon()` shape.
///
/// See [Polygon](Polygon).
pub struct Point {
    /// The x position of the point.
    x: LengthPercentage,
    /// The y position of the point.
    y: LengthPercentage,
}

/// A value for the [mask-mode](https://www.w3.org/TR/css-masking-1/#the-mask-mode) property.
pub enum MaskMode {
    /// The luminance values of the mask image is used.
    luminance,
    /// The alpha values of the mask image is used.
    alpha,
    /// If an SVG source is used, the value matches the `mask-type` property. Otherwise, the alpha values are used.
    @"match-source",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): MaskMode {
        return .@"match-source";
    }
}

/// A value for the [mask-clip](https://www.w3.org/TR/css-masking-1/#the-mask-clip) property.
pub const MaskClip = union(enum) {
    /// A geometry box.
    @"geometry-box": GeometryBox,
    /// The painted content is not clipped.
    @"no-clip",

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

/// A value for the [mask-composite](https://www.w3.org/TR/css-masking-1/#the-mask-composite) property.
pub enum MaskComposite {
    /// The source is placed over the destination.
    add,
    /// The source is placed, where it falls outside of the destination.
    subtract,
    /// The parts of source that overlap the destination, replace the destination.
    intersect,
    /// The non-overlapping regions of source and destination are combined.
    exclude,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): MaskComposite {
        return .add;
    }
}

/// A value for the [mask-type](https://www.w3.org/TR/css-masking-1/#the-mask-type) property.
pub enum MaskType {
    /// The luminance values of the mask is used.
    luminance,
    /// The alpha values of the mask is used.
    alpha,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [mask](https://www.w3.org/TR/css-masking-1/#the-mask) shorthand property.
pub struct Mask {
    /// The mask image.
    image: Image,
    /// The position of the mask.
    position: Position,
    /// The size of the mask image.
    size: BackgroundSize,
    /// How the mask repeats.
    repeat: BackgroundRepeat,
    /// The box in which the mask is clipped.
    clip: MaskClip,
    /// The origin of the mask.
    origin: GeometryBox,
    /// How the mask is composited with the element.
    composite: MaskComposite,
    /// How the mask image is interpreted.
    mode: MaskMode,

    pub const PropertyFieldMap = {
        image: css.PropertyIdTag.@"mask-image",
        position: css.PropertyIdTag.@"mask-position",
        size: css.PropertyIdTag.@"mask-size",
        repeat: css.PropertyIdTag.@"mask-repeat",
        clip: css.PropertyIdTag.@"mask-clip",
        origin: css.PropertyIdTag.@"mask-origin",
        composite: css.PropertyIdTag.@"mask-composite",
        mode: css.PropertyIdTag.@"mask-mode",
    };

    pub const VendorPrefixMap = {
        image: true,
        position: true,
        size: true,
        repeat: true,
        clip: true,
        origin: true,
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let image: Image? = null;
        let position: Position? = null;
        let size: BackgroundSize? = null;
        let repeat: BackgroundRepeat? = null;
        let clip: MaskClip? = null;
        let origin: GeometryBox? = null;
        let composite: MaskComposite? = null;
        let mode: MaskMode? = null;

        while (true) {
            if (image == null) {
                if (value := input.tryParse(Image.parse, {}).asValue()) {
                    image = value;
                    continue;
                }
            }

            if (position == null) {
                if (value := input.tryParse(Position.parse, {}).asValue()) {
                    position = value;
                    size = input.tryParse(struct {
                        pub inline fn parseFn(i: *css.Parser): css.Result(BackgroundSize) {
                            if (e := i.expectDelim('/').asErr()) return { err: e };
                            return BackgroundSize.parse(i);
                        }
                    }.parseFn, {}).asValue();
                    continue;
                }
            }

            if (repeat == null) {
                if (value := input.tryParse(BackgroundRepeat.parse, {}).asValue()) {
                    repeat = value;
                    continue;
                }
            }

            if (origin == null) {
                if (value := input.tryParse(GeometryBox.parse, {}).asValue()) {
                    origin = value;
                    continue;
                }
            }

            if (clip == null) {
                if (value := input.tryParse(MaskClip.parse, {}).asValue()) {
                    clip = value;
                    continue;
                }
            }

            if (composite == null) {
                if (value := input.tryParse(MaskComposite.parse, {}).asValue()) {
                    composite = value;
                    continue;
                }
            }

            if (mode == null) {
                if (value := input.tryParse(MaskMode.parse, {}).asValue()) {
                    mode = value;
                    continue;
                }
            }

            break;
        }

        if (clip == null) {
            if (o := origin) {
                clip = o.intoMaskClip();
            }
        }

        return { result: {
            image: image ?? Image.default(),
            position: position ?? Position.default(),
            repeat: repeat ?? BackgroundRepeat.default(),
            size: size ?? BackgroundSize.default(),
            origin: origin ?? .@"border-box",
            clip: clip ?? GeometryBox.@"border-box".intoMaskClip(),
            composite: composite ?? .add,
            mode: mode ?? .@"match-source",
        } };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        _.image.toCss(dest)?;

        if (!_.position.eql(&Position.default()) || !_.size.eql(&BackgroundSize.default())) {
            dest.writeChar(' ')?;
            _.position.toCss(dest)?;

            if (!_.size.eql(&BackgroundSize.default())) {
                dest.delim('/', true)?;
                _.size.toCss(dest)?;
            }
        }

        if (!_.repeat.eql(&BackgroundRepeat.default())) {
            dest.writeChar(' ')?;
            _.repeat.toCss(dest)?;
        }

        if (!_.origin.eql(&GeometryBox.@"border-box") || !_.clip.eql(&GeometryBox.@"border-box".intoMaskClip())) {
            dest.writeChar(' ')?;
            _.origin.toCss(dest)?;

            if (!_.clip.eql(&_.origin.intoMaskClip())) {
                dest.writeChar(' ')?;
                _.clip.toCss(dest)?;
            }
        }

        if (!_.composite.eql(&MaskComposite.default())) {
            dest.writeChar(' ')?;
            _.composite.toCss(dest)?;
        }

        if (!_.mode.eql(&MaskMode.default())) {
            dest.writeChar(' ')?;
            _.mode.toCss(dest)?;
        }

        return;
    }
    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn deepClone(val: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), val, allocator);
    }
}

/// A value for the [mask-border-mode](https://www.w3.org/TR/css-masking-1/#the-mask-border-mode) property.
pub enum MaskBorderMode {
    /// The luminance values of the mask image is used.
    luminance,
    /// The alpha values of the mask image is used.
    alpha,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): @This() {
        return .alpha;
    }
}

/// A value for the [mask-border](https://www.w3.org/TR/css-masking-1/#the-mask-border) shorthand property.
/// A value for the [mask-border](https://www.w3.org/TR/css-masking-1/#the-mask-border) shorthand property.
pub struct MaskBorder {
    /// The mask image.
    source: Image,
    /// The offsets that define where the image is sliced.
    slice: BorderImageSlice,
    /// The width of the mask image.
    width: Rect(BorderImageSideWidth),
    /// The amount that the image extends beyond the border box.
    outset: Rect(LengthOrNumber),
    /// How the mask image is scaled and tiled.
    repeat: BorderImageRepeat,
    /// How the mask image is interpreted.
    mode: MaskBorderMode,

    // (old using name space) css.DefineShorthand(@This(), css.PropertyIdTag.@"mask-border", PropertyFieldMap);

    pub const PropertyFieldMap = {
        source: css.PropertyIdTag.@"mask-border-source",
        slice: css.PropertyIdTag.@"mask-border-slice",
        width: css.PropertyIdTag.@"mask-border-width",
        outset: css.PropertyIdTag.@"mask-border-outset",
        repeat: css.PropertyIdTag.@"mask-border-repeat",
        mode: css.PropertyIdTag.@"mask-border-mode",
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        const Closure = struct {
            mode: MaskBorderMode? = null,
        };
        let closure = Closure{ .mode = null };
        const border_image = BorderImage.parseWithCallback(input, &closure, struct {
            inline fn callback(c: *Closure, p: *css.Parser): bool {
                if (c.mode == null) {
                    if (value := p.tryParse(MaskBorderMode.parse, {}).asValue()) {
                        c.mode = value;
                        return true;
                    }
                }
                return false;
            }
        }.callback);

        if (border_image.isOk() || closure.mode != null) {
            const bi = border_image.unwrapOr(comptime BorderImage.default());
            return { result: MaskBorder{
                source: bi.source,
                slice: bi.slice,
                width: bi.width,
                outset: bi.outset,
                repeat: bi.repeat,
                mode: closure.mode ?? MaskBorderMode.default(),
            } };
        } else {
            return { err: input.newCustomError(.invalid_declaration) };
        }
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        try BorderImage.toCssInternal(
            &_.source,
            &_.slice,
            &_.width,
            &_.outset,
            &_.repeat,
            dest,
        );
        if (!_.mode.eql(&MaskBorderMode.default())) {
            dest.writeChar(' ')?;
            _.mode.toCss(dest)?;
        }
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn deepClone(val: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), val, allocator);
    }
}

/// A value for the [-webkit-mask-composite](https://developer.mozilla.org/en-US/docs/Web/CSS/-webkit-mask-composite)
/// property.
///
/// See also [MaskComposite](MaskComposite).
/// A value for the [-webkit-mask-composite](https://developer.mozilla.org/en-US/docs/Web/CSS/-webkit-mask-composite)
/// property.
///
/// See also [MaskComposite](MaskComposite).
pub enum WebKitMaskComposite {
    clear,
    copy,
    /// Equivalent to `add` in the standard `mask-composite` syntax.
    @"source-over",
    /// Equivalent to `intersect` in the standard `mask-composite` syntax.
    @"source-in",
    /// Equivalent to `subtract` in the standard `mask-composite` syntax.
    @"source-out",
    @"source-atop",
    @"destination-over",
    @"destination-in",
    @"destination-out",
    @"destination-atop",
    /// Equivalent to `exclude` in the standard `mask-composite` syntax.
    xor,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [-webkit-mask-source-type](https://github.com/WebKit/WebKit/blob/6eece09a1c31e47489811edd003d1e36910e9fd3/Source/WebCore/css/CSSProperties.json#L6578-L6587)
/// property.
///
/// See also [MaskMode](MaskMode).
/// A value for the [-webkit-mask-source-type](https://github.com/WebKit/WebKit/blob/6eece09a1c31e47489811edd003d1e36910e9fd3/Source/WebCore/css/CSSProperties.json#L6578-L6587)
/// property.
///
/// See also [MaskMode](MaskMode).
pub enum WebKitMaskSourceType {
    /// Equivalent to `match-source` in the standard `mask-mode` syntax.
    auto,
    /// The luminance values of the mask image is used.
    luminance,
    /// The alpha values of the mask image is used.
    alpha,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

pub fn getWebkitMaskProperty(property_id: *const css.PropertyId): css.PropertyId? {
    return switch (property_id.*) {
        .@"mask-border-source" => .{ .@"mask-box-image-source" = VendorPrefix.WEBKIT },
        .@"mask-border-slice" => .{ .@"mask-box-image-slice" = VendorPrefix.WEBKIT },
        .@"mask-border-width" => .{ .@"mask-box-image-width" = VendorPrefix.WEBKIT },
        .@"mask-border-outset" => .{ .@"mask-box-image-outset" = VendorPrefix.WEBKIT },
        .@"mask-border-repeat" => .{ .@"mask-box-image-repeat" = VendorPrefix.WEBKIT },
        .@"mask-border" => .{ .@"mask-box-image" = VendorPrefix.WEBKIT },
        .@"mask-composite" => css.PropertyId.@"-webkit-mask-composite",
        .@"mask-mode" => .{ .@"mask-source-type" = VendorPrefix.WEBKIT },
        else => null,
    };
}

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
