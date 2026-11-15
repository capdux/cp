import css from "../css_parser.zig";

const Property = css.Property;
const VendorPrefix = css.VendorPrefix;
const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const LengthPercentageOrAuto = css.css_values.length.LengthPercentageOrAuto;
const Image = css.css_values.image.Image;
const CssColor = css.css_values.color.CssColor;
const Ratio = css.css_values.ratio.Ratio;
const HorizontalPosition = css.css_values.position.HorizontalPosition;
const VerticalPosition = css.css_values.position.VerticalPosition;

const Position = css.css_values.position.Position;

/// A value for the [background](https://www.w3.org/TR/css-backgrounds-3/#background) shorthand property.
pub struct Background {
    /// The background image.
    image: Image,
    /// The background color.
    color: CssColor,
    /// The background position.
    position: BackgroundPosition,
    /// How the background image should repeat.
    repeat: BackgroundRepeat,
    /// The size of the background image.
    size: BackgroundSize,
    /// The background attachment.
    attachment: BackgroundAttachment,
    /// The background origin.
    origin: BackgroundOrigin,
    /// How the background should be clipped.
    clip: BackgroundClip,

    fn deinit(_: *@This(), _: Allocator) {
        // TODO: implement this
        // not necessary right now because all allocations in CSS parser are in arena
    }

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let color: CssColor? = null;
        let position: BackgroundPosition? = null;
        let size: BackgroundSize? = null;
        let image: Image? = null;
        let repeat: BackgroundRepeat? = null;
        let attachment: BackgroundAttachment? = null;
        let origin: BackgroundOrigin? = null;
        let clip: BackgroundClip? = null;

        while (true) {
            // TODO: only allowed on the last background.
            if (color == null) {
                if (value := input.tryParse(CssColor.parse, {}).asValue()) {
                    color = value;
                    continue;
                }
            }

            if (position == null) {
                if (value := input.tryParse(BackgroundPosition.parse, {}).asValue()) {
                    position = value;

                    size = input.tryParse(struct {
                        static fn parse(i: *css.Parser): css.Result(BackgroundSize) {
                            if (e := i.expectDelim('/').asErr()) return { err: e };
                            return BackgroundSize.parse(i);
                        }
                    }.parse, {}).asValue();

                    continue;
                }
            }

            if (image == null) {
                if (value := input.tryParse(Image.parse, {}).asValue()) {
                    image = value;
                    continue;
                }
            }

            if (repeat == null) {
                if (value := input.tryParse(BackgroundRepeat.parse, {}).asValue()) {
                    repeat = value;
                    continue;
                }
            }

            if (attachment == null) {
                if (value := input.tryParse(BackgroundAttachment.parse, {}).asValue()) {
                    attachment = value;
                    continue;
                }
            }

            if (origin == null) {
                if (value := input.tryParse(BackgroundOrigin.parse, {}).asValue()) {
                    origin = value;
                    continue;
                }
            }

            if (clip == null) {
                if (value := input.tryParse(BackgroundClip.parse, {}).asValue()) {
                    clip = value;
                    continue;
                }
            }

            break;
        }

        if (clip == null) {
            if (o := origin) {
                clip = @as(BackgroundClip, @enumFromInt(@intFromEnum(o)));
            }
        }

        return { result: {
            image: image ?? Image.default(),
            color: color ?? CssColor.default(),
            position: position ?? BackgroundPosition.default(),
            repeat: repeat ?? BackgroundRepeat.default(),
            size: size ?? BackgroundSize.default(),
            attachment: attachment ?? BackgroundAttachment.default(),
            origin: origin ?? .@"padding-box",
            clip: clip ?? .@"border-box",
        } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        let has_output = false;

        if (!_.color.eql(&CssColor.default())) {
            _.color.toCss(dest)?;
            has_output = true;
        }

        if (!_.image.eql(&Image.default())) {
            if (has_output) try dest.writeStr(" ");
            _.image.toCss(dest)?;
            has_output = true;
        }

        const position: Position = _.position.intoPosition();
        if (!position.isZero() || !_.size.eql(&BackgroundSize.default())) {
            if (has_output) {
                dest.writeStr(" ")?;
            }
            position.toCss(dest)?;

            if (!_.size.eql(&BackgroundSize.default())) {
                dest.delim('/', true)?;
                _.size.toCss(dest)?;
            }

            has_output = true;
        }

        if (!_.repeat.eql(&BackgroundRepeat.default())) {
            if (has_output) try dest.writeStr(" ");
            _.repeat.toCss(dest)?;
            has_output = true;
        }

        if (!_.attachment.eql(&BackgroundAttachment.default())) {
            if (has_output) try dest.writeStr(" ");
            _.attachment.toCss(dest)?;
            has_output = true;
        }

        const output_padding_box = !_.origin.eql(&BackgroundOrigin.@"padding-box") ||
            (!_.clip.eqlOrigin(&BackgroundOrigin.@"border-box") && _.clip.isBackgroundBox());

        if (output_padding_box) {
            if (has_output) try dest.writeStr(" ");
            _.origin.toCss(dest)?;
            has_output = true;
        }

        if ((output_padding_box && !_.clip.eqlOrigin(&BackgroundOrigin.@"border-box")) ||
            !_.clip.eqlOrigin(&BackgroundOrigin.@"border-box"))
        {
            if (has_output) try dest.writeStr(" ");

            _.clip.toCss(dest)?;
            has_output = true;
        }

        // If nothing was output, then this is the initial value, e.g. background: transparent
        if (!has_output) {
            if (dest.minify) {
                // `0 0` is the shortest valid background value
                _.position.toCss(dest)?;
            } else {
                dest.writeStr("none")?;
            }
        }
    }

    fn getImage(): *const Image {
        return &_.image;
    }

    fn withImage(allocator: Allocator, image: Image): @This() {
        let ret = _.*;
        ret.image = .none;
        ret = ret.deepClone(allocator);
        ret.image = image;
        return ret;
    }

    fn getFallback(allocator: Allocator, kind: css.ColorFallbackKind): Background {
        let ret: Background = _.*;
        // Dummy values for the clone
        ret.color = CssColor.default();
        ret.image = Image.default();
        ret = ret.deepClone(allocator);
        ret.color = _.color.getFallback(allocator, kind);
        ret.image = _.image.getFallback(allocator, kind);
        return ret;
    }

    fn getNecessaryFallbacks(targets: css.targets.Targets): css.ColorFallbackKind {
        return bun.bits.@"or"(
            css.ColorFallbackKind,
            _.color.getNecessaryFallbacks(targets),
            _.getImage().getNecessaryFallbacks(targets),
        );
    }

    inline fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [background-size](https://www.w3.org/TR/css-backgrounds-3/#background-size) property.
pub const BackgroundSize = union(enum) {
    /// An explicit background size.
    explicit: struct {
        /// The width of the background.
        width: css.css_values.length.LengthPercentageOrAuto,
        /// The height of the background.
        height: css.css_values.length.LengthPercentageOrAuto,

        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }

        pub inline fn deepClone(this: *const @This(), allocator: Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }
    },
    /// The `cover` keyword. Scales the background image to cover both the width and height of the element.
    cover,
    /// The `contain` keyword. Scales the background image so that it fits within the element.
    contain,

    pub fn parse(input: *css.Parser) css.Result(@This()) {
        if (input.tryParse(LengthPercentageOrAuto.parse, .{}).asValue()) |width| {
            const height = input.tryParse(LengthPercentageOrAuto.parse, .{}).unwrapOr(.auto);
            return .{ .result = .{ .explicit = .{ .width = width, .height = height } } };
        }

        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |v| v,
            .err => |e| return .{ .err = e },
        };

        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "cover")) {
            return .{ .result = .cover };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "contain")) {
            return .{ .result = .contain };
        } else {
            return .{ .err = location.newBasicUnexpectedTokenError(.{ .ident = ident }) };
        }
    }

    pub fn toCss(this: *const BackgroundSize, dest: *Printer) PrintErr!void {
        return switch (this.*) {
            .cover => dest.writeStr("cover"),
            .contain => dest.writeStr("contain"),
            .explicit => |explicit| {
                try explicit.width.toCss(dest);
                if (explicit.height != .auto) {
                    try dest.writeStr(" ");
                    try explicit.height.toCss(dest);
                }
                return;
            },
        };
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn default() @This() {
        return BackgroundSize{ .explicit = .{
            .width = .auto,
            .height = .auto,
        } };
    }

    pub inline fn deepClone(this: *const @This(), allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

/// A value for the [background-position](https://drafts.csswg.org/css-backgrounds/#background-position) shorthand property.
pub struct BackgroundPosition {
    /// The x-position.
    x: HorizontalPosition,
    /// The y-position.
    y: VerticalPosition,

    const PropertyFieldMap = {
        x: css.PropertyIdTag.@"background-position-x",
        y: css.PropertyIdTag.@"background-position-y",
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        const pos = switch (css.css_values.position.Position.parse(input)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        return { result: BackgroundPosition.fromPosition(pos) };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        const pos = _.intoPosition();
        return pos.toCss(dest);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn default(): @This() {
        return BackgroundPosition.fromPosition(Position.default());
    }

    static fn fromPosition(pos: Position): BackgroundPosition {
        return BackgroundPosition{ .x = pos.x, .y = pos.y };
    }

    fn intoPosition(): Position {
        return Position{ .x = _.x, .y = _.y };
    }

    pub inline fn deepClone(_: *const @This(), allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
}


pub enum BackgroundRepeatKeyword {
    repeat,
    space,
    round,
    @"no-repeat",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [background-repeat](https://www.w3.org/TR/css-backgrounds-3/#background-repeat) property.
pub struct BackgroundRepeat {
    /// A repeat style for the x direction.
    x: BackgroundRepeatKeyword,
    /// A repeat style for the y direction.
    y: BackgroundRepeatKeyword,

    static fn default(): @This() {
        return BackgroundRepeat{
            x: .repeat,
            y: .repeat,
        };
    }

    static fn parse(input: *css.Parser): css.Result(@This()) {
        const state = input.state();
        const ident = switch (input.expectIdent()) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "repeat-x")) {
            return { result: { x: .repeat, y: .@"no-repeat" } };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "repeat-y")) {
            return { result: { x: .@"no-repeat", y: .repeat } };
        }

        input.reset(&state);

        const x = switch (@call(.auto, @field(BackgroundRepeatKeyword, "parse"), .{input})) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        const y = input.tryParse(BackgroundRepeatKeyword.parse, {}).unwrapOr(x);

        return { result: { x: x, y: y } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        const Repeat = BackgroundRepeatKeyword.repeat;
        const NoRepeat = BackgroundRepeatKeyword.@"no-repeat";

        if (_.x == Repeat && _.y == NoRepeat) {
            return dest.writeStr("repeat-x");
        } else if (_.x == NoRepeat && _.y == Repeat) {
            return dest.writeStr("repeat-y");
        } else {
            _.x.toCss(dest)?;
            if (_.y != _.x) {
                dest.writeStr(" ")?;
                _.y.toCss(dest)?;
            }
        }
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}


/// A value for the [background-attachment](https://www.w3.org/TR/css-backgrounds-3/#background-attachment) property.
pub enum BackgroundAttachment {
    /// The background scrolls with the container.
    scroll,
    /// The background is fixed to the viewport.
    fixed,
    /// The background is fixed with regard to the element's contents.
    local,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): @This() {
        return .scroll;
    }
}

/// A value for the [background-origin](https://www.w3.org/TR/css-backgrounds-3/#background-origin) property.
pub enum BackgroundOrigin {
    /// The position is relative to the border box.
    @"border-box",
    /// The position is relative to the padding box.
    @"padding-box",
    /// The position is relative to the content box.
    @"content-box",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [background-clip](https://drafts.csswg.org/css-backgrounds-4/#background-clip) property.
pub enum BackgroundClip {
    /// The background is clipped to the border box.
    @"border-box",
    /// The background is clipped to the padding box.
    @"padding-box",
    /// The background is clipped to the content box.
    @"content-box",
    /// The background is clipped to the area painted by the border.
    border,
    /// The background is clipped to the text content of the element.
    text,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): BackgroundClip {
        return .@"border-box";
    }

    fn eqlOrigin(other: *const BackgroundOrigin): bool {
        return switch (_.*) {
            .@"border-box" => other.* == .@"border-box",
            .@"padding-box" => other.* == .@"padding-box",
            .@"content-box" => other.* == .@"content-box",
            else => false,
        };
    }

    fn isBackgroundBox(): bool {
        return switch (_.*) {
            .@"border-box", .@"padding-box", .@"content-box" => true,
            else => false,
        };
    }
}

/// A value for the [aspect-ratio](https://drafts.csswg.org/css-sizing-4/#aspect-ratio) property.
pub struct AspectRatio {
    /// The `auto` keyword.
    auto: bool,
    /// A preferred aspect ratio for the box, specified as width / height.
    ratio: Ratio?,
}

@packed(u16) pub struct BackgroundProperty {
    color: bool = false,
    image: bool = false,
    @"position-x": bool = false,
    @"position-y": bool = false,
    repeat: bool = false,
    size: bool = false,
    attachment: bool = false,
    origin: bool = false,
    clip: bool = false,
    __unused: u7 = 0,

    pub const @"background-color" = BackgroundProperty{ .color = true };
    pub const @"background-image" = BackgroundProperty{ .image = true };
    pub const @"background-position-x" = BackgroundProperty{ .@"position-x" = true };
    pub const @"background-position-y" = BackgroundProperty{ .@"position-y" = true };
    pub const @"background-position" = BackgroundProperty{ .@"position-x" = true, .@"position-y" = true };
    pub const @"background-repeat" = BackgroundProperty{ .repeat = true };
    pub const @"background-size" = BackgroundProperty{ .size = true };
    pub const @"background-attachment" = BackgroundProperty{ .attachment = true };
    pub const @"background-origin" = BackgroundProperty{ .origin = true };
    pub const @"background-clip" = BackgroundProperty{ .clip = true };

    pub const background = BackgroundProperty{
        .color = true,
        .image = true,
        .@"position-x" = true,
        .@"position-y" = true,
        .repeat = true,
        .size = true,
        .attachment = true,
        .origin = true,
        .clip = true,
    };

    fn isEmpty(): bool {
        return bun.bits.asInt(@This(), _) == 0;
    }

    static fn tryFromPropertyId(property_id: css.PropertyId): BackgroundProperty? {
        return (switch (property_id) {
            .@"background-color" => BackgroundProperty.@"background-color",
            .@"background-image" => BackgroundProperty.@"background-image",
            .@"background-position-x" => BackgroundProperty.@"background-position-x",
            .@"background-position-y" => BackgroundProperty.@"background-position-y",
            .@"background-position" => BackgroundProperty.@"background-position",
            .@"background-repeat" => BackgroundProperty.@"background-repeat",
            .@"background-size" => BackgroundProperty.@"background-size",
            .@"background-attachment" => BackgroundProperty.@"background-attachment",
            .@"background-origin" => BackgroundProperty.@"background-origin",
            .background => BackgroundProperty.background,
            else => null,
        });
    }
}

const ClipsAndPrefix = struct { css.SmallList(BackgroundClip, 1), VendorPrefix };

pub struct BackgroundHandler {
    color: CssColor? = null,
    images: css.SmallList(Image, 1)? = null,
    has_prefix: bool = false,
    x_positions: css.SmallList(HorizontalPosition, 1)? = null,
    y_positions: css.SmallList(VerticalPosition, 1)? = null,
    repeats: css.SmallList(BackgroundRepeat, 1)? = null,
    sizes: css.SmallList(BackgroundSize, 1)? = null,
    attachments: css.SmallList(BackgroundAttachment, 1)? = null,
    origins: css.SmallList(BackgroundOrigin, 1)? = null,
    clips: ClipsAndPrefix? = null,
    decls: ArrayList(Property) = undefined,
    flushed_properties: BackgroundProperty = undefined,
    has_any: bool = false,

    fn handleProperty(
        *_,
        property: *const Property,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ): bool {
        const allocator = context.allocator;
        switch (property.*) {
            .@"background-color" => |*val| {
                _.flushHelper(allocator, "color", CssColor, val, dest, context);
                _.color = val.deepClone(allocator);
            },
            .@"background-image" => |*val| {
                _.backgroundHelper(allocator, SmallList(Image, 1), val, property, dest, context);
                _.images = val.deepClone(allocator);
            },
            .@"background-position" => |val| {
                let x_positions = _.initSmallListHelper(HorizontalPosition, 1, "x_positions", allocator, val.len());
                let y_positions = _.initSmallListHelper(VerticalPosition, 1, "y_positions", allocator, val.len());
                let i: usize = 0;
                while (i < val.len()) : (i += 1) {
                    x_positions[i] = val.slice()[i].x.deepClone(allocator);
                    y_positions[i] = val.slice()[i].y.deepClone(allocator);
                }
            },
            .@"background-position-x" => |val| {
                if (*x_positions := _.x_positions) x_positions.deinit(allocator);
                _.x_positions = val.deepClone(allocator);
            },
            .@"background-position-y" => |val| {
                if (*y_positions := _.y_positions) y_positions.deinit(allocator);
                _.y_positions = val.deepClone(allocator);
            },
            .@"background-repeat" => |val| {
                if (*repeats := _.repeats) repeats.deinit(allocator);
                _.repeats = val.deepClone(allocator);
            },
            .@"background-size" => |val| {
                if (*sizes := _.sizes) sizes.deinit(allocator);
                _.sizes = val.deepClone(allocator);
            },
            .@"background-attachment" => |val| {
                if (*attachments := _.attachments) attachments.deinit(allocator);
                _.attachments = val.deepClone(allocator);
            },
            .@"background-origin" => |val| {
                if (*origins := _.origins) origins.deinit(allocator);
                _.origins = val.deepClone(allocator);
            },
            .@"background-clip" => |*x| {
                const val: *const SmallList(BackgroundClip, 1) = &x.*[0];
                const vendor_prefix: VendorPrefix = x.*[1];
                if (*clips_and_vp := _.clips) {
                    let clips: *SmallList(BackgroundClip, 1) = &clips_and_vp.*[0];
                    const vp: *VendorPrefix = &clips_and_vp.*[1];
                    if (vendor_prefix != vp.* && !val.eql(clips)) {
                        // `flush()` takes ownership of `this.clips` via `bun.take` and
                        // frees it, so `clips` (which aliases the payload of `this.clips`)
                        // is a stale pointer once `flush()` returns. Do not touch it.
                        _.flush(allocator, dest, context);
                        _.clips = .{ val.deepClone(allocator), vendor_prefix };
                    } else {
                        if (!val.eql(clips)) {
                            clips.deinit(allocator);
                            clips.* = val.deepClone(allocator);
                        }
                        bun.bits.insert(VendorPrefix, vp, vendor_prefix);
                    }
                } else {
                    _.clips = .{ val.deepClone(allocator), vendor_prefix };
                }
            },
            .background => |*val| {
                let images = SmallList(Image, 1).initCapacity(allocator, val.len());
                for (*b of val.slice()) {
                    images.appendAssumeCapacity(b.image.deepClone(allocator));
                }
                _.backgroundHelper(allocator, SmallList(Image, 1), &images, property, dest, context);
                const color = val.last().?.color.deepClone(allocator);
                _.flushHelper(allocator, "color", CssColor, &color, dest, context);
                let clips = SmallList(BackgroundClip, 1).initCapacity(allocator, val.len());
                for (*b of val.slice()) {
                    clips.appendAssumeCapacity(b.clip.deepClone(allocator));
                }
                let clips_vp = VendorPrefix{ .none = true };
                if (*clips_and_vp := _.clips) {
                    if (clips_vp != clips_and_vp.*[1] && !clips.eql(&clips_and_vp.*[0])) {
                        _.flush(allocator, dest, context);
                    } else {
                        bun.bits.insert(VendorPrefix, &clips_vp, clips_and_vp.*[1]);
                    }
                }

                if (*c := _.color) c.deinit(allocator);
                _.color = color;
                if (*i := _.images) i.deinit(allocator);
                _.images = images;
                let x_positions = _.initSmallListHelper(HorizontalPosition, 1, "x_positions", allocator, val.len());
                let y_positions = _.initSmallListHelper(VerticalPosition, 1, "y_positions", allocator, val.len());
                let repeats = _.initSmallListHelper(BackgroundRepeat, 1, "repeats", allocator, val.len());
                let sizes = _.initSmallListHelper(BackgroundSize, 1, "sizes", allocator, val.len());
                let attachments = _.initSmallListHelper(BackgroundAttachment, 1, "attachments", allocator, val.len());
                let origins = _.initSmallListHelper(BackgroundOrigin, 1, "origins", allocator, val.len());

                let i: usize = 0;
                while (i < val.len()) : (i += 1) {
                    const b = &val.slice()[i];
                    x_positions[i] = b.position.x.deepClone(allocator);
                    y_positions[i] = b.position.y.deepClone(allocator);
                    repeats[i] = b.repeat.deepClone(allocator);
                    sizes[i] = b.size.deepClone(allocator);
                    attachments[i] = b.attachment.deepClone(allocator);
                    origins[i] = b.origin.deepClone(allocator);
                }

                _.clips = .{ clips, clips_vp };
            },
            .unparsed => |*val| {
                if (isBackgroundProperty(val.property_id)) {
                    _.flush(allocator, dest, context);
                    let unparsed = val.deepClone(allocator);
                    context.addUnparsedFallbacks(&unparsed);
                    if (prop := BackgroundProperty.tryFromPropertyId(val.property_id)) {
                        bun.bits.insert(BackgroundProperty, &_.flushed_properties, prop);
                    }

                    bun.handleOom(dest.append(allocator, Property{ .unparsed = unparsed }));
                } else return false;
            },
            else => return false,
        }

        _.has_any = true;
        return true;
    }

    // Either get the value from the field on `this` or initialize a new one
    fn initSmallListHelper(
        *_,
        comptime T: type,
        comptime N: comptime_int,
        comptime field: []const u8,
        allocator: Allocator,
        length: u32,
    ): []T {
        if (*list := @field(_, field)) {
            list.clearRetainingCapacity();
            list.ensureTotalCapacity(allocator, length);
            list.setLen(length);
            return list.slice_mut();
        } else {
            @field(_, field) = SmallList(T, N).initCapacity(allocator, length);
            @field(_, field).?.setLen(length);
            return @field(_, field).?.slice_mut();
        }
    }

    fn backgroundHelper(
        *_,
        allocator: Allocator,
        comptime T: type,
        val: *const T,
        property: *const Property,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ) {
        _.flushHelper(allocator, "images", T, val, dest, context);

        // Store prefixed properties. Clear if we hit an unprefixed property and we have
        // targets. In this case, the necessary prefixes will be generated.
        _.has_prefix = val.any(struct {
            static fn predicate(item: *const Image): bool {
                return item.hasVendorPrefix();
            }
        }.predicate);
        if (_.has_prefix) {
            bun.handleOom(_.decls.append(allocator, property.deepClone(allocator)));
        } else if (context.targets.browsers != null) {
            _.decls.clearRetainingCapacity();
        }
    }

    fn flushHelper(
        *_,
        allocator: Allocator,
        comptime field: []const u8,
        comptime T: type,
        val: *const T,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ) {
        if (@field(_, field) != null &&
            !@field(_, field).?.eql(val) &&
            context.targets.browsers != null && !val.isCompatible(context.targets.browsers.?))
        {
            _.flush(allocator, dest, context);
        }
    }

    fn flush(*_, allocator: Allocator, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) return;
        _.has_any = false;
        const push = struct {
            static fn push(handler: *BackgroundHandler, alloc: Allocator, d: *css.DeclarationList, comptime property_field_name: []const u8, val: auto) {
                bun.handleOom(d.append(alloc, @unionInit(Property, property_field_name, val)));
                const prop = @field(BackgroundProperty, property_field_name);
                bun.bits.insert(BackgroundProperty, &handler.flushed_properties, prop);
            }
        }.push;

        let maybe_color: CssColor? = bun.take(&_.color);
        let maybe_images: css.SmallList(Image, 1)? = bun.take(&_.images);
        let maybe_x_positions: css.SmallList(HorizontalPosition, 1)? = bun.take(&_.x_positions);
        let maybe_y_positions: css.SmallList(VerticalPosition, 1)? = bun.take(&_.y_positions);
        let maybe_repeats: css.SmallList(BackgroundRepeat, 1)? = bun.take(&_.repeats);
        let maybe_sizes: css.SmallList(BackgroundSize, 1)? = bun.take(&_.sizes);
        let maybe_attachments: css.SmallList(BackgroundAttachment, 1)? = bun.take(&_.attachments);
        let maybe_origins: css.SmallList(BackgroundOrigin, 1)? = bun.take(&_.origins);
        let maybe_clips: ClipsAndPrefix? = bun.take(&_.clips);
        defer {
            if (*c := maybe_color) c.deinit(allocator);
            if (*i := maybe_images) i.deinit(allocator);
            if (*x := maybe_x_positions) x.deinit(allocator);
            if (*y := maybe_y_positions) y.deinit(allocator);
            if (*r := maybe_repeats) r.deinit(allocator);
            if (*s := maybe_sizes) s.deinit(allocator);
            if (*a := maybe_attachments) a.deinit(allocator);
            if (*o := maybe_origins) o.deinit(allocator);
            if (*c := maybe_clips) c.*[0].deinit(allocator);
        }

        if (maybe_color != null &&
            maybe_images != null &&
            maybe_x_positions != null &&
            maybe_y_positions != null &&
            maybe_repeats != null &&
            maybe_sizes != null &&
            maybe_attachments != null &&
            maybe_origins != null &&
            maybe_clips != null)
        {
            const color = &maybe_color.?;
            let images = &maybe_images.?;
            let x_positions = &maybe_x_positions.?;
            let y_positions = &maybe_y_positions.?;
            let repeats = &maybe_repeats.?;
            let sizes = &maybe_sizes.?;
            let attachments = &maybe_attachments.?;
            let origins = &maybe_origins.?;
            let clips = &maybe_clips.?;

            // Only use shorthand syntax if the number of layers matches on all properties.
            const len = images.len();
            if (x_positions.len() == len &&
                y_positions.len() == len &&
                repeats.len() == len &&
                sizes.len() == len && attachments.len() == len && origins.len() == len && clips[0].len() == len)
            {
                const clip_prefixes = if (clips.*[0].any(struct {
                    static fn predicate(clip: *const BackgroundClip): bool {
                        return clip.* == BackgroundClip.text;
                    }
                }.predicate)) context.targets.prefixes(clips.*[1], .background_clip) else clips.*[1];
                const clip_property = if (clip_prefixes != css.VendorPrefix{ .none = true })
                    css.Property{ .@"background-clip" = .{ clips.*[0].deepClone(allocator), clip_prefixes } }
                else
                    null;

                let backgrounds = SmallList(Background, 1).initCapacity(allocator, len);
                let i: usize = 0;
                while (i < len) : (i += 1) {
                    backgrounds.appendAssumeCapacity(Background{
                        color: if (i == len - 1) color.deepClone(allocator) else CssColor.default(),
                        image: images.slice()[i],
                        position: BackgroundPosition{ .x = x_positions.slice()[i], .y = y_positions.slice()[i] },
                        repeat: repeats.slice()[i],
                        size: sizes.slice()[i],
                        attachment: attachments.slice()[i],
                        origin: origins.slice()[i],
                        clip: if (clip_prefixes == css.VendorPrefix{ .none = true }) clips.*[0].slice()[i] else BackgroundClip.default(),
                    });
                }
                defer {
                    images.clearRetainingCapacity();
                    x_positions.clearRetainingCapacity();
                    y_positions.clearRetainingCapacity();
                    repeats.clearRetainingCapacity();
                    sizes.clearRetainingCapacity();
                    attachments.clearRetainingCapacity();
                    origins.clearRetainingCapacity();
                    clips.*[0].clearRetainingCapacity();
                }

                if (_.flushed_properties.isEmpty()) {
                    for (fallback of backgrounds.getFallbacks(allocator, context.targets).slice()) {
                        push(_, allocator, dest, "background", fallback);
                    }
                }

                push(_, allocator, dest, "background", backgrounds);

                if (clip := clip_property) {
                    bun.handleOom(dest.append(allocator, clip));
                    _.flushed_properties.clip = true;
                }

                _.reset(allocator);
                return;
            }
        }

        if (color_const := bun.take(&maybe_color)) {
            let color: CssColor = color_const;
            if (!_.flushed_properties.color) {
                for (fallback of color.getFallbacks(allocator, context.targets).slice()) {
                    push(_, allocator, dest, "background-color", fallback);
                }
            }
            push(_, allocator, dest, "background-color", color);
        }

        if (images_ := bun.take(&maybe_images)) {
            let images: css.SmallList(Image, 1) = images_;
            if (!_.flushed_properties.image) {
                let fallbacks = images.getFallbacks(allocator, context.targets);
                for (fallback of fallbacks.slice()) {
                    push(_, allocator, dest, "background-image", fallback);
                }
            }
            push(_, allocator, dest, "background-image", images);
        }

        if (maybe_x_positions != null && maybe_y_positions != null && maybe_x_positions.?.len() == maybe_y_positions.?.len()) {
            let positions = SmallList(BackgroundPosition, 1).initCapacity(allocator, maybe_x_positions.?.len());
            let i: usize = 0;
            const x_slice = maybe_x_positions.?.slice();
            const y_slice = maybe_y_positions.?.slice();
            while (i < maybe_x_positions.?.len()) : (i += 1) {
                positions.appendAssumeCapacity(BackgroundPosition{ .x = x_slice[i], .y = y_slice[i] });
            }
            maybe_x_positions.?.clearRetainingCapacity();
            maybe_y_positions.?.clearRetainingCapacity();
            push(_, allocator, dest, "background-position", positions);
        } else {
            if (x := bun.take(&maybe_x_positions)) {
                push(_, allocator, dest, "background-position-x", x);
            }
            if (y := bun.take(&maybe_y_positions)) {
                push(_, allocator, dest, "background-position-y", y);
            }
        }

        if (rep := bun.take(&maybe_repeats)) {
            push(_, allocator, dest, "background-repeat", rep);
        }

        if (rep := bun.take(&maybe_sizes)) {
            push(this, allocator, dest, "background-size", rep);
        }

        if (rep := bun.take(&maybe_attachments)) {
            push(this, allocator, dest, "background-attachment", rep);
        }

        if (rep := bun.take(&maybe_origins)) {
            push(this, allocator, dest, "background-origin", rep);
        }

        if (c := bun.take(&maybe_clips)) {
            const clips = c[0];
            const vp = c[1];
            const prefixes = if (clips.any(struct {
                static fn predicate(clip: *const BackgroundClip): bool {
                    return clip.* == BackgroundClip.text;
                }
            }.predicate)) context.targets.prefixes(vp, css.prefixes.Feature.background_clip) else vp;
            dest.append(
                allocator,
                Property{
                    .@"background-clip" = .{ clips.deepClone(allocator), prefixes },
                },
            ) catch |e| bun.handleOom(e);
            _.flushed_properties.clip = true;
        }

        _.reset(allocator);
    }

    fn reset(*_, allocator: Allocator) {
        if (c := _.color) c.deinit(allocator);
        _.color = null;
        if (*i := _.images) i.deinit(allocator);
        _.images = null;
        if (*x := _.x_positions) x.deinit(allocator);
        _.x_positions = null;
        if (*y := _.y_positions) y.deinit(allocator);
        _.y_positions = null;
        if (*r := _.repeats) r.deinit(allocator);
        _.repeats = null;
        if (*s := _.sizes) s.deinit(allocator);
        _.sizes = null;
        if (*a := _.attachments) a.deinit(allocator);
        _.attachments = null;
        if (*o := _.origins) o.deinit(allocator);
        _.origins = null;
        if (*c := _.clips) c.*[0].deinit(allocator);
        _.clips = null;
    }

    fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        const allocator = context.allocator;
        // If the last declaration is prefixed, pop the last value
        // so it isn't duplicated when we flush.
        if (_.has_prefix) {
            let maybe_prop = _.decls.pop();
            if (*prop := maybe_prop) {
                prop.deinit(allocator);
            }
        }

        bun.handleOom(dest.appendSlice(allocator, _.decls.items));
        _.decls.clearRetainingCapacity();

        _.flush(allocator, dest, context);
        _.flushed_properties = BackgroundProperty{};
    }
}

fn isBackgroundProperty(property_id: css.PropertyId): bool {
    return switch (property_id) {
        .@"background-color",
        .@"background-image",
        .@"background-position",
        .@"background-position-x",
        .@"background-position-y",
        .@"background-repeat",
        .@"background-size",
        .@"background-attachment",
        .@"background-origin",
        .@"background-clip",
        .background => true,
        else => false,
    };
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
