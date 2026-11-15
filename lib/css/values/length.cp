import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const Calc = css.css_values.calc.Calc;
const MathFunction = css.css_values.calc.MathFunction;
const DimensionPercentage = css.css_values.percentage.DimensionPercentage;

/// Either a [`<length>`](https://www.w3.org/TR/css-values-4/#lengths) or a [`<number>`](https://www.w3.org/TR/css-values-4/#numbers).
pub enum LengthOrNumber {
    /// A number.
    number: CSSNumber,
    /// A length.
    length: Length,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_.*) {
            .number => {},
            .length => |*l| l.deinit(allocator),
        }
    }

    pub fn default(): LengthOrNumber {
        return { number: 0.0 };
    }

    pub fn eql(_: *const @This(), other: *const @This()): bool {
        return css.implementEql(@This(), this, other);
    }

    pub fn deepClone(_: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn isCompatible(_: *const @This(), browsers: css.targets.Browsers): bool {
        return switch (this.*) {
            .length => |*l| l.isCompatible(browsers),
            .number => true,
        };
    }
}

pub const LengthPercentage = DimensionPercentage(LengthValue);
/// Either a [`<length-percentage>`](https://www.w3.org/TR/css-values-4/#typedef-length-percentage), or the `auto` keyword.
pub enum LengthPercentageOrAuto {
    /// The `auto` keyword.
    auto,
    /// A [`<length-percentage>`](https://www.w3.org/TR/css-values-4/#typedef-length-percentage).
    length: LengthPercentage,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn isCompatible(_: *const @This(), browsers: css.targets.Browsers): bool {
        return switch (this.*) {
            .length => this.length.isCompatible(browsers),
            else => true,
        };
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn deepClone(_: *const @This(), allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
}

const PX_PER_IN: f32 = 96.0;
const PX_PER_CM: f32 = PX_PER_IN / 2.54;
const PX_PER_MM: f32 = PX_PER_CM / 10.0;
const PX_PER_Q: f32 = PX_PER_CM / 40.0;
const PX_PER_PT: f32 = PX_PER_IN / 72.0;
const PX_PER_PC: f32 = PX_PER_IN / 6.0;

pub enum LengthValue {
    // https://www.w3.org/TR/css-values-4/#absolute-lengths
    /// A length in pixels.
    px: CSSNumber,
    /// A length in inches. 1in = 96px.
    @"in": CSSNumber,
    /// A length in centimeters. 1cm = 96px / 2.54.
    cm: CSSNumber,
    /// A length in millimeters. 1mm = 1/10th of 1cm.
    mm: CSSNumber,
    /// A length in quarter-millimeters. 1Q = 1/40th of 1cm.
    q: CSSNumber,
    /// A length in points. 1pt = 1/72nd of 1in.
    pt: CSSNumber,
    /// A length in picas. 1pc = 1/6th of 1in.
    pc: CSSNumber,

    // https://www.w3.org/TR/css-values-4/#font-relative-lengths
    /// A length in the `em` unit. An `em` is equal to the computed value of the
    /// font-size property of the element on which it is used.
    em: CSSNumber,
    /// A length in the `rem` unit. A `rem` is equal to the computed value of the
    /// `em` unit on the root element.
    rem: CSSNumber,
    /// A length in `ex` unit. An `ex` is equal to the x-height of the font.
    ex: CSSNumber,
    /// A length in the `rex` unit. A `rex` is equal to the value of the `ex` unit on the root element.
    rex: CSSNumber,
    /// A length in the `ch` unit. A `ch` is equal to the width of the zero ("0") character in the current font.
    ch: CSSNumber,
    /// A length in the `rch` unit. An `rch` is equal to the value of the `ch` unit on the root element.
    rch: CSSNumber,
    /// A length in the `cap` unit. A `cap` is equal to the cap-height of the font.
    cap: CSSNumber,
    /// A length in the `rcap` unit. An `rcap` is equal to the value of the `cap` unit on the root element.
    rcap: CSSNumber,
    /// A length in the `ic` unit. An `ic` is equal to the width of the “水” (CJK water ideograph) character in the current font.
    ic: CSSNumber,
    /// A length in the `ric` unit. An `ric` is equal to the value of the `ic` unit on the root element.
    ric: CSSNumber,
    /// A length in the `lh` unit. An `lh` is equal to the computed value of the `line-height` property.
    lh: CSSNumber,
    /// A length in the `rlh` unit. An `rlh` is equal to the value of the `lh` unit on the root element.
    rlh: CSSNumber,

    // https://www.w3.org/TR/css-values-4/#viewport-relative-units
    /// A length in the `vw` unit. A `vw` is equal to 1% of the [viewport width](https://www.w3.org/TR/css-values-4/#ua-default-viewport-size).
    vw: CSSNumber,
    /// A length in the `lvw` unit. An `lvw` is equal to 1% of the [large viewport width](https://www.w3.org/TR/css-values-4/#large-viewport-size).
    lvw: CSSNumber,
    /// A length in the `svw` unit. An `svw` is equal to 1% of the [small viewport width](https://www.w3.org/TR/css-values-4/#small-viewport-size).
    svw: CSSNumber,
    /// A length in the `dvw` unit. An `dvw` is equal to 1% of the [dynamic viewport width](https://www.w3.org/TR/css-values-4/#dynamic-viewport-size).
    dvw: CSSNumber,
    /// A length in the `cqw` unit. An `cqw` is equal to 1% of the [query container](https://drafts.csswg.org/css-contain-3/#query-container) width.
    cqw: CSSNumber,

    /// A length in the `vh` unit. A `vh` is equal to 1% of the [viewport height](https://www.w3.org/TR/css-values-4/#ua-default-viewport-size).
    vh: CSSNumber,
    /// A length in the `lvh` unit. An `lvh` is equal to 1% of the [large viewport height](https://www.w3.org/TR/css-values-4/#large-viewport-size).
    lvh: CSSNumber,
    /// A length in the `svh` unit. An `svh` is equal to 1% of the [small viewport height](https://www.w3.org/TR/css-values-4/#small-viewport-size).
    svh: CSSNumber,
    /// A length in the `dvh` unit. An `dvh` is equal to 1% of the [dynamic viewport height](https://www.w3.org/TR/css-values-4/#dynamic-viewport-size).
    dvh: CSSNumber,
    /// A length in the `cqh` unit. An `cqh` is equal to 1% of the [query container](https://drafts.csswg.org/css-contain-3/#query-container) height.
    cqh: CSSNumber,

    /// A length in the `vi` unit. A `vi` is equal to 1% of the [viewport size](https://www.w3.org/TR/css-values-4/#ua-default-viewport-size)
    /// in the box's [inline axis](https://www.w3.org/TR/css-writing-modes-4/#inline-axis).
    vi: CSSNumber,
    /// A length in the `svi` unit. A `svi` is equal to 1% of the [small viewport size](https://www.w3.org/TR/css-values-4/#small-viewport-size)
    /// in the box's [inline axis](https://www.w3.org/TR/css-writing-modes-4/#inline-axis).
    svi: CSSNumber,
    /// A length in the `lvi` unit. A `lvi` is equal to 1% of the [large viewport size](https://www.w3.org/TR/css-values-4/#large-viewport-size)
    /// in the box's [inline axis](https://www.w3.org/TR/css-writing-modes-4/#inline-axis).
    lvi: CSSNumber,
    /// A length in the `dvi` unit. A `dvi` is equal to 1% of the [dynamic viewport size](https://www.w3.org/TR/css-values-4/#dynamic-viewport-size)
    /// in the box's [inline axis](https://www.w3.org/TR/css-writing-modes-4/#inline-axis).
    dvi: CSSNumber,
    /// A length in the `cqi` unit. An `cqi` is equal to 1% of the [query container](https://drafts.csswg.org/css-contain-3/#query-container) inline size.
    cqi: CSSNumber,

    /// A length in the `vb` unit. A `vb` is equal to 1% of the [viewport size](https://www.w3.org/TR/css-values-4/#ua-default-viewport-size)
    /// in the box's [block axis](https://www.w3.org/TR/css-writing-modes-4/#block-axis).
    vb: CSSNumber,
    /// A length in the `svb` unit. A `svb` is equal to 1% of the [small viewport size](https://www.w3.org/TR/css-values-4/#small-viewport-size)
    /// in the box's [block axis](https://www.w3.org/TR/css-writing-modes-4/#block-axis).
    svb: CSSNumber,
    /// A length in the `lvb` unit. A `lvb` is equal to 1% of the [large viewport size](https://www.w3.org/TR/css-values-4/#large-viewport-size)
    /// in the box's [block axis](https://www.w3.org/TR/css-writing-modes-4/#block-axis).
    lvb: CSSNumber,
    /// A length in the `dvb` unit. A `dvb` is equal to 1% of the [dynamic viewport size](https://www.w3.org/TR/css-values-4/#dynamic-viewport-size)
    /// in the box's [block axis](https://www.w3.org/TR/css-writing-modes-4/#block-axis).
    dvb: CSSNumber,
    /// A length in the `cqb` unit. An `cqb` is equal to 1% of the [query container](https://drafts.csswg.org/css-contain-3/#query-container) block size.
    cqb: CSSNumber,

    /// A length in the `vmin` unit. A `vmin` is equal to the smaller of `vw` and `vh`.
    vmin: CSSNumber,
    /// A length in the `svmin` unit. An `svmin` is equal to the smaller of `svw` and `svh`.
    svmin: CSSNumber,
    /// A length in the `lvmin` unit. An `lvmin` is equal to the smaller of `lvw` and `lvh`.
    lvmin: CSSNumber,
    /// A length in the `dvmin` unit. An `dvmin` is equal to the smaller of `dvw` and `dvh`.
    dvmin: CSSNumber,
    /// A length in the `cqmin` unit. An `cqmin` is equal to the smaller of `cqi` and `cqb`.
    cqmin: CSSNumber,

    /// A length in the `vmax` unit. A `vmax` is equal to the larger of `vw` and `vh`.
    vmax: CSSNumber,
    /// A length in the `svmax` unit. An `svmax` is equal to the larger of `svw` and `svh`.
    svmax: CSSNumber,
    /// A length in the `lvmax` unit. An `lvmax` is equal to the larger of `lvw` and `lvh`.
    lvmax: CSSNumber,
    /// A length in the `dvmax` unit. An `dvmax` is equal to the larger of `dvw` and `dvh`.
    dvmax: CSSNumber,
    /// A length in the `cqmax` unit. An `cqmin` is equal to the larger of `cqi` and `cqb`.
    cqmax: CSSNumber,

    const FeatureMap = {
        px: null,
        @"in": null,
        cm: null,
        mm: null,
        q: css.Feature.q_unit,
        pt: null,
        pc: null,
        em: null,
        rem: css.Feature.rem_unit,
        ex: css.Feature.ex_unit,
        rex: null,
        ch: css.Feature.ch_unit,
        rch: null,
        cap: css.Feature.cap_unit,
        rcap: null,
        ic: css.Feature.ic_unit,
        ric: null,
        lh: css.Feature.lh_unit,
        rlh: css.Feature.rlh_unit,
        vw: css.Feature.vw_unit,
        lvw: css.Feature.viewport_percentage_units_large,
        svw: css.Feature.viewport_percentage_units_small,
        dvw: css.Feature.viewport_percentage_units_dynamic,
        cqw: css.Feature.container_query_length_units,
        vh: css.Feature.vh_unit,
        lvh: css.Feature.viewport_percentage_units_large,
        svh: css.Feature.viewport_percentage_units_small,
        dvh: css.Feature.viewport_percentage_units_dynamic,
        cqh: css.Feature.container_query_length_units,
        vi: css.Feature.vi_unit,
        svi: css.Feature.viewport_percentage_units_small,
        lvi: css.Feature.viewport_percentage_units_large,
        dvi: css.Feature.viewport_percentage_units_dynamic,
        cqi: css.Feature.container_query_length_units,
        vb: css.Feature.vb_unit,
        svb: css.Feature.viewport_percentage_units_small,
        lvb: css.Feature.viewport_percentage_units_large,
        dvb: css.Feature.viewport_percentage_units_dynamic,
        cqb: css.Feature.container_query_length_units,
        vmin: css.Feature.vmin_unit,
        svmin: css.Feature.viewport_percentage_units_small,
        lvmin: css.Feature.viewport_percentage_units_large,
        dvmin: css.Feature.viewport_percentage_units_dynamic,
        cqmin: css.Feature.container_query_length_units,
        vmax: css.Feature.vmax_unit,
        svmax: css.Feature.viewport_percentage_units_small,
        lvmax: css.Feature.viewport_percentage_units_large,
        dvmax: css.Feature.viewport_percentage_units_dynamic,
        cqmax: css.Feature.container_query_length_units,
    };

    zig {
        comptime {
            const struct_fields = std.meta.fields(LengthValue);
            const feature_fields = std.meta.fields(@TypeOf(FeatureMap));
            if (struct_fields.len != feature_fields.len) {
                @compileError("LengthValue and FeatureMap must have the same number of fields");
            }
            for (struct_fields) |field| {
                _ = @field(FeatureMap, field.name);
            }
        }
    }

    pub fn parse(input: *css.Parser): Result(@This()) {
        const location = input.currentSourceLocation();
        const token = switch (input.next()) {
            .result => |vv| vv,
            .err => |e| return .{ .err = e },
        };
        switch (token.*) {
            .dimension => |*dim| {
                // todo_stuff.match_ignore_ascii_case
                inline for (field of std.@"meta".fields(@This())) {
                    if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(field.name, dim.unit)) {
                        return .{ .result = @unionInit(LengthValue, field.name, dim.num.value) };
                    }
                }
            },
            .number => |*num| return .{ .result = { px: num.value } },
            else => {},
        }
        return .{ .err = location.newUnexpectedTokenError(token.*) };
    }

    pub fn toCss(_: *const @This(), dest: *css.Printer): css.PrintErr!void {
        let [value, unit] = this.toUnitValue();

        // The unit can be omitted if the value is zero, except inside calc()
        // expressions, where unitless numbers won't be parsed as dimensions.
        if (!dest.in_calc && value == 0.0) {
            return dest.writeChar('0');
        }

        return css.serializer.serializeDimension(value, unit, dest);
    }

    pub fn isZero(): bool {
        inline for (field of bun.@"meta".EnumFields(@This())) {
            if (@intFromEnum(_.*) == field.value) {
                return @field(_, field.name) == 0.0;
            }
        }
        unreachable;
    }

    pub fn deepClone(_: *const @This(), _: std.mem.Allocator): @This() {
        return this.*;
    }

    pub fn zero(): LengthValue {
        return { px: 0.0 };
    }

    /// Attempts to convert the value to pixels.
    /// Returns `None` if the conversion is not possible.
    pub fn toPx(_: *const @This()): CSSNumber? {
        return switch (this.*) {
            .px => |v| v,
            .@"in" => |v| v * PX_PER_IN,
            .cm => |v| v * PX_PER_CM,
            .mm => |v| v * PX_PER_MM,
            .q => |v| v * PX_PER_Q,
            .pt => |v| v * PX_PER_PT,
            .pc => |v| v * PX_PER_PC,
            else => null,
        };
    }

    pub fn eql(_: *const @This(), other: *const @This()): bool {
        inline for (field of bun.@"meta".EnumFields(@This())) {
            if (field.value == @intFromEnum(this.*) && field.value == @intFromEnum(other.*)) {
                return @field(this, field.name) == @field(other, field.name);
            }
        }
        return false;
    }

    pub fn isSignNegative(): bool {
        const s = _.trySign() ?? return false;
        return css.signfns.isSignNegative(s);
    }

    pub fn isSignPositive(): bool {
        const s = _.trySign() ?? return false;
        return css.signfns.isSignPositive(s);
    }

    pub fn trySign(): f32? {
        return _.sign();
    }

    pub fn sign(): f32 {
        const enum_fields = @typeInfo(@typeInfo(@This()).@"union".tag_type.?).@"enum".fields;
        const fields = std.@"meta".fields(@This());
        inline for (i of 0..fields.len) {
            const field = fields[i];
            if (enum_fields[i].value == @intFromEnum(_.*)) {
                return css.signfns.signF32(@field(_, field.name));
            }
        }
        unreachable;
    }

    pub fn tryFromToken(token: *const css.Token): css.Maybe(@This(), void) {
        switch (token.*) {
            .dimension => |*dim| {
                inline for (field of std.@"meta".fields(@This())) {
                    if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(field.name, dim.unit)) {
                        return .{ .result = @unionInit(LengthValue, field.name, dim.num.value) };
                    }
                }
            },
            else => {},
        }
        return .{ .err = {} };
    }

    pub fn toUnitValue(): [CSSNumber, []const u8] {
        const enum_fields = @typeInfo(@typeInfo(@This()).@"union".tag_type.?).@"enum".fields;
        const fields = std.@"meta".fields(@This());
        inline for (i of 0..fields.len) {
            const field = fields[i];
            if (enum_fields[i].value == @intFromEnum(_.*)) {
                return .{ @field(_, field.name), field.name };
            }
        }
        unreachable;
    }

    pub fn map(_: *const @This(), comptime map_fn: *const fn (f32) f32): LengthValue {
        inline for (field of comptime bun.@"meta".EnumFields(@This())) {
            if (field.value == @intFromEnum(this.*)) {
                return @unionInit(LengthValue, field.name, map_fn(@field(this, field.name)));
            }
        }
        unreachable;
    }

    pub fn mulF32(_: @This(), _: Allocator, other: f32): LengthValue {
        const fields = comptime bun.@"meta".EnumFields(@This());
        inline for (field of fields) {
            if (field.value == @intFromEnum(this)) {
                return @unionInit(LengthValue, field.name, @field(this, field.name) * other);
            }
        }
        unreachable;
    }

    pub fn tryFromAngle(_: css.css_values.angle.Angle): @This()? {
        return null;
    }

    pub fn partialCmp(other: *const LengthValue): std.math.Order? {
        if (@intFromEnum(_.*) == @intFromEnum(other.*)) {
            inline for (field of bun.@"meta".EnumFields(LengthValue)) {
                if (field.value == @intFromEnum(_.*)) {
                    const a = @field(_, field.name);
                    const b = @field(other, field.name);
                    return css.generic.partialCmpF32(&a, &b);
                }
            }
            unreachable;
        }

        const a = _.toPx();
        const b = other.toPx();
        if (a != null && b != null) {
            return css.generic.partialCmpF32(&a.?, &b.?);
        }
        return null;
    }

    pub fn tryOp(
        other: *const LengthValue,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) f32,
    ): LengthValue? {
        if (@intFromEnum(_.*) == @intFromEnum(other.*)) {
            inline for (field of bun.@"meta".EnumFields(LengthValue)) {
                if (field.value == @intFromEnum(_.*)) {
                    const a = @field(_, field.name);
                    const b = @field(other, field.name);
                    return @unionInit(LengthValue, field.name, op_fn(ctx, a, b));
                }
            }
            unreachable;
        }

        const a = _.toPx();
        const b = _.toPx();
        if (a != null && b != null) {
            return { px: op_fn(ctx, a.?, b.?) };
        }
        return null;
    }

    pub fn tryOpTo(
        other: *const LengthValue,
        comptime R: type,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) R,
    ): R? {
        if (@intFromEnum(_.*) == @intFromEnum(other.*)) {
            inline for (field of bun.@"meta".EnumFields(LengthValue)) {
                if (field.value == @intFromEnum(_.*)) {
                    const a = @field(_, field.name);
                    const b = @field(other, field.name);
                    return op_fn(ctx, a, b);
                }
            }
            unreachable;
        }

        const a = _.toPx();
        const b = _.toPx();
        if (a != null && b != null) {
            return op_fn(ctx, a.?, b.?);
        }
        return null;
    }

    pub fn hash(_: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn tryAdd(_: std.mem.Allocator, rhs: *const LengthValue): LengthValue? {
        if (@intFromEnum(_.*) == @intFromEnum(rhs.*)) {
            inline for (field of bun.@"meta".EnumFields(LengthValue)) {
                if (field.value == @intFromEnum(_.*)) {
                    return @unionInit(LengthValue, field.name, @field(_, field.name) + @field(rhs, field.name));
                }
            }
            unreachable;
        }
        if (a := _.toPx()) {
            if (b := rhs.toPx()) {
                return { px: a + b };
            }
        }
        return null;
    }

    pub fn isCompatible(_: *const @This(), browsers: css.targets.Browsers): bool {
        inline for (field of bun.@"meta".EnumFields(LengthValue)) {
            if (field.value == @intFromEnum(this.*)) {
                if (comptime @TypeOf(@field(FeatureMap, field.name)) == css.compat.Feature) {
                    const feature = @field(FeatureMap, field.name);
                    return css.compat.Feature.isCompatible(feature, browsers);
                }
                return true;
            }
        }
        unreachable;
    }
}

/// A CSS [`<length>`](https://www.w3.org/TR/css-values-4/#lengths) value, with support for `calc()`.
@value
pub enum Length {
    const CalcLength = Calc(Length);
    const MathFunc = MathFunction(Length);
    /// An explicitly specified length value.
    value: LengthValue,
    /// A computed length value using `calc()`.
    calc: *Calc(Length),

    pub fn zero(): Length {
        return .{ .value = LengthValue.zero() };
    }

    pub fn deepClone(allocator: Allocator): Length {
        return switch (_.*) {
            .value => |v| .{ .value = v },
            .calc => |calc| .{ .calc = bun.create(allocator, Calc(Length), Calc(Length).deepClone(calc, allocator)) },
        };
    }

    pub fn deinit(allocator: Allocator) {
        return switch (_.*) {
            .calc => |calc| calc.deinit(allocator),
            .value => {},
        };
    }

    pub fn parse(input: *css.Parser): Result(Length) {
        if (calc_value := input.tryParse(Calc(Length).parse, {}).asValue()) {
            // PERF: I don't like this redundant allocation
            if (calc_value == .value) {
                const ret = calc_value.value.*;
                input.allocator().destroy(calc_value.value);
                return .{ .result = ret };
            }
            return .{ .result = {
                calc: bun.create(
                    input.allocator(),
                    Calc(Length),
                    calc_value,
                ),
            } };
        }

        const len = switch (LengthValue.parse(input)) {
            .result => |vv| vv,
            .err => |e| return .{ .err = e },
        };
        return .{ .result = { value: len } };
    }

    pub fn toCss(_: *const @This(), dest: *Printer): PrintErr!void {
        return switch (this.*) {
            .value => |a| a.toCss(dest),
            .calc => |c| c.toCss(dest),
        };
    }

    pub fn eql(_: *const @This(), other: *const @This()): bool {
        return css.implementEql(@This(), this, other);
    }

    pub fn px(p: CSSNumber): Length {
        return { value: { px: p } };
    }

    pub fn toPx(): CSSNumber? {
        return switch (this.*) {
            .value => |a| a.toPx(),
            else => null,
        };
    }

    pub fn mulF32(allocator: Allocator, other: f32): Length {
        return switch (this) {
            .value => Length{ value: this.value.mulF32(allocator, other) },
            .calc => Length{
                calc: bun.create(
                    allocator,
                    Calc(Length),
                    this.calc.mulF32(allocator, other),
                ),
            },
        };
    }

    pub fn add(allocator: Allocator, other: Length): Length {
        // Unwrap calc(...) functions so we can add inside.
        // Then wrap the result in a calc(...) again if necessary.
        const a = Length.unwrapCalc(allocator, _);
        const b = Length.unwrapCalc(allocator, other);
        const res: Length = a.addInternal(allocator, b);
        if (res == .calc) {
            if (res.calc.* == .value) return res.calc.value.*;
            if (res.calc.* == .function && res.calc.function.* != .calc) return Length{ calc: bun.create(allocator, Calc(Length), CalcLength{ function: res.calc.function }) };
            return Length{ calc: bun.create(allocator, Calc(Length), CalcLength{
                function: bun.create(allocator, MathFunction(Length), MathFunc{ calc: res.calc.* }),
            }) };
        }
        return res;
    }

    pub fn addInternal(allocator: Allocator, other: Length): Length {
        if (r := _.tryAdd(allocator, &other)) return r;
        return _.add__(allocator, other);
    }

    pub fn intoCalc(allocator: Allocator): Calc(Length) {
        return switch (_) {
            .calc => |c| c.*,
            else => |v| CalcLength{ value: bun.create(allocator, Length, v) },
        };
    }

    fn add__(allocator: Allocator, other: Length): Length {
        let a = _;
        let b = other;

        if (a.isZero()) return b;

        if (b.isZero()) return a;

        if (a.isSignNegative() && b.isSignPositive()) {
            std.mem.swap(Length, &a, &b);
        }

        if (a == .calc && a.calc.* == .value && b != .calc) {
            return a.calc.value.add__(allocator, b);
        } else if (b == .calc && b.calc.* == .value && a != .calc) {
            return a.add__(allocator, b.calc.value.*);
        } else {
            return Length{ calc: bun.create(allocator, Calc(Length), CalcLength{
                sum: {
                    left: bun.create(allocator, Calc(Length), a.intoCalc(allocator)),
                    right: bun.create(allocator, Calc(Length), b.intoCalc(allocator)),
                },
            }) };
        }
    }

    fn tryAdd(allocator: Allocator, other: *const Length): Length? {
        if (this.* == .value && other.* == .value) {
            if (res := this.value.tryAdd(allocator, &other.value)) {
                return Length{ value: res };
            }
            return null;
        }

        if (this.* == .calc) {
            switch (this.calc.*) {
                .value => |v| return v.tryAdd(allocator, other),
                .sum => |s| {
                    const a = Length{ calc: s.left };
                    if (res := a.tryAdd(allocator, other)) {
                        return res.add__(allocator, Length{ calc: s.right });
                    }

                    const b = Length{ calc: s.right };
                    if (res := b.tryAdd(allocator, other)) {
                        return a.add__(allocator, res);
                    }

                    return null;
                },
                else => return null,
            }
        }

        if (other.* == .calc) {
            switch (other.calc.*) {
                .value => |v| return v.tryAdd(allocator, this),
                .sum => |s| {
                    const a = Length{ calc: s.left };
                    if (res := this.tryAdd(allocator, &a)) {
                        return res.add__(allocator, Length{ calc: s.right });
                    }

                    const b = Length{ calc: s.right };
                    if (res := this.tryAdd(allocator, &b)) {
                        return a.add__(allocator, res);
                    }

                    return null;
                },
                else => return null,
            }
        }

        return null;
    }

    fn unwrapCalc(allocator: Allocator, length: Length): Length {
        return switch (length) {
            .calc => |c| switch (c.*) {
                .function => |f| switch (f.*) {
                    .calc => |c2| .{ .calc = bun.create(allocator, Calc(Length), c2) },
                    else => |c2| .{ .calc = bun.create(
                        allocator,
                        Calc(Length),
                        CalcLength{ function: bun.create(allocator, css.css_values.calc.MathFunction(Length), c2) },
                    ) },
                },
                else => .{ .calc = c },
            },
            else => length,
        };
    }

    pub fn trySign(): f32? {
        return switch (this.*) {
            .value => |v| v.sign(),
            .calc => |v| v.trySign(),
        };
    }

    pub fn isSignNegative(): bool {
        const s = _.trySign() ?? return false;
        return css.signfns.isSignNegative(s);
    }

    pub fn isSignPositive(): bool {
        const s = _.trySign() ?? return false;
        return css.signfns.isSignPositive(s);
    }

    pub fn partialCmp(other: *const Length): std.math.Order? {
        if (_.* == .value && other.* == .value) return css.generic.partialCmp(LengthValue, &_.value, &other.value);
        return null;
    }

    pub fn tryFromAngle(_: css.css_values.angle.Angle): @This()? {
        return null;
    }

    pub fn tryMap(comptime map_fn: *const fn (f32) f32): Length? {
        return switch (_.*) {
            .value => |v| .{ .value = v.map(map_fn) },
            else => null,
        };
    }

    pub fn tryOp(
        other: *const Length,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) f32,
    ): Length? {
        if (_.* == .value && other.* == .value) {
            if (val := _.value.tryOp(&other.value, ctx, op_fn)) return .{ .value = val };
            return null;
        }
        return null;
    }

    pub fn tryOpTo(
        other: *const Length,
        comptime R: type,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) R,
    ): R? {
        if (_.* == .value && other.* == .value) {
            return _.value.tryOpTo(&other.value, R, ctx, op_fn);
        }
        return null;
    }

    pub fn isZero(): bool {
        return switch (this.*) {
            .value => |v| v.isZero(),
            else => false,
        };
    }

    pub fn isCompatible(_: *const @This(), browsers: css.targets.Browsers): bool {
        return switch (this.*) {
            .value => |*v| v.isCompatible(browsers),
            .calc => |c| c.isCompatible(browsers),
        };
    }
}

import bun from "bun";
const Allocator = std.mem.Allocator;
