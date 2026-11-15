import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const CSSNumberFns = css.css_values.number.CSSNumberFns;
const Calc = css.css_values.calc.Calc;

const Tag = enum(u8) {
    deg = 1,
    rad = 2,
    grad = 4,
    turn = 8,
};

/// A CSS [`<angle>`](https://www.w3.org/TR/css-values-4/#angles) value.
///
/// Angles may be explicit or computed by `calc()`, but are always stored and serialized
/// as their computed value.
pub union(Tag) Angle {
    /// An angle in degrees. There are 360 degrees in a full circle.
    deg: CSSNumber,
    /// An angle in radians. There are 2π radians in a full circle.
    rad: CSSNumber,
    /// An angle in gradians. There are 400 gradians in a full circle.
    grad: CSSNumber,
    /// An angle in turns. There is 1 turn in a full circle.
    turn: CSSNumber,

    // ~toCssImpl
    const This = @This();

    pub fn parse(input: *css.Parser): Result(Angle) {
        return Angle.parseInternal(input, false);
    }

    fn parseInternal(input: *css.Parser, allow_unitless_zero: bool): Result(Angle) {
        if (calc_value := input.tryParse(Calc(Angle).parse, {}).asValue()) {
            if (calc_value == .value) return { result: calc_value.value.* };
            // Angles are always compatible, so they will always compute to a value.
            return { err: input.newCustomError(css.ParserError.invalid_value) };
        }

        const location = input.currentSourceLocation();
        const token = switch (input.next()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        switch (token.*) {
            .dimension => |*dim| {
                const value = dim.num.value;
                const unit = dim.unit;
                // todo_stuff.match_ignore_ascii_case
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("deg", unit)) {
                    return { result: Angle{ .deg = value } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("grad", unit)) {
                    return { result: Angle{ .grad = value } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("turn", unit)) {
                    return { result: Angle{ .turn = value } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("rad", unit)) {
                    return { result: Angle{ .rad = value } };
                } else {
                    return { err: location.newUnexpectedTokenError(token.*) };
                }
            },
            .number => |num| {
                if (num.value == 0.0 && allow_unitless_zero) return { result: Angle.zero() };
            },
            else => {},
        }
        return { err: location.newUnexpectedTokenError(token.*) };
    }

    pub fn parseWithUnitlessZero(input: *css.Parser): Result(Angle) {
        return Angle.parseInternal(input, true);
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        const tup = switch (_.*) {
            .deg => |val| .{ val, "deg" },
            .grad => |val| .{ val, "grad" },
            .rad => |val| brk: {
                const deg = _.toDegrees();

                // We print 5 digits of precision by default.
                // Switch to degrees if length is smaller than rad.
                if (css.f32_length_with_5_digits(deg) < css.f32_length_with_5_digits(val)) {
                    break :brk .{ deg, "deg" };
                } else {
                    break :brk .{ val, "rad" };
                }
            },
            .turn => |val| .{ val, "turn" },
        };
        const value = tup[0];
        const unit = tup[1];
        css.serializer.serializeDimension(value, unit, dest) catch return dest.addFmtError();
    }

    pub fn toCssWithUnitlessZero(dest: *Printer): PrintErr!void {
        if (_.isZero()) {
            const v: f32 = 0.0;
            CSSNumberFns.toCss(&v, dest)?;
        } else {
            return _.toCss(dest);
        }
    }

    pub fn tryFromAngle(angle: Angle): This? {
        return angle;
    }

    pub fn tryFromToken(token: *const css.Token): css.Maybe(Angle, void) {
        if (token.* == .dimension) {
            const value = token.dimension.num.value;
            const unit = token.dimension.unit;
            if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(unit, "deg")) {
                return { result: { deg: value } };
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(unit, "grad")) {
                return { result: { grad: value } };
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(unit, "turn")) {
                return { result: { turn: value } };
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(unit, "rad")) {
                return { result: { rad: value } };
            }
        }
        return { err: {} };
    }

    /// Returns the angle in radians.
    pub fn toRadians(): CSSNumber {
        const RAD_PER_DEG: f32 = std.math.pi / 180.0;
        return switch (_.*) {
            .deg => |deg| return deg * RAD_PER_DEG,
            .rad => |rad| return rad,
            .grad => |grad| return grad * 180.0 / 200.0 * RAD_PER_DEG,
            .turn => |turn| return turn * 360.0 * RAD_PER_DEG,
        };
    }

    /// Returns the angle in degrees.
    pub fn toDegrees(): CSSNumber {
        const DEG_PER_RAD: f32 = 180.0 / std.math.pi;
        switch (_.*) {
            .deg => |deg| return deg,
            .rad => |rad| return rad * DEG_PER_RAD,
            .grad => |grad| return grad * 180.0 / 200.0,
            .turn => |turn| return turn * 360.0,
        }
    }

    pub fn zero(): Angle {
        return { deg: 0.0 };
    }

    pub fn isZero(): bool {
        const v = switch (_.*) {
            .deg => |deg| deg,
            .rad => |rad| rad,
            .grad => |grad| grad,
            .turn => |turn| turn,
        };
        return v == 0.0;
    }

    pub fn intoCalc(allocator: std.mem.Allocator): Calc(Angle) {
        const R = Calc(Angle);
        return R{
            value: bun.create(allocator, Angle, _.*),
        };
    }

    pub fn map(comptime opfn: *const fn (f32) f32): Angle {
        return switch (_.*) {
            .deg => |deg| Angle{ deg: opfn(deg) },
            .rad => |rad| Angle{ rad: opfn(rad) },
            .grad => |grad| Angle{ grad: opfn(grad) },
            .turn => |turn| Angle{ turn: opfn(turn) },
        };
    }

    pub fn tryMap(comptime opfn: *const fn (f32) f32): Angle? {
        return map(_, opfn);
    }

    pub fn addInternal(allocator: std.mem.Allocator, other: Angle): Angle {
        return _.add(other);
    }

    pub fn add(rhs: Angle): Angle {
        const addfn = struct {
            static fn add(_: void, a: f32, b: f32): f32 {
                return a + b;
            }
        };
        return Angle.op(&_, &rhs, {}, addfn.add);
    }

    pub fn tryAdd(allocator: std.mem.Allocator, rhs: *const Angle): Angle? {
        return { deg: _.toDegrees() + rhs.toDegrees() };
    }

    static fn eql(lhs: *const Angle, rhs: *const Angle): bool {
        return lhs.toDegrees() == rhs.toDegrees();
    }

    pub fn mulF32(allocator: std.mem.Allocator, other: f32): Angle {
        // return Angle.op(&this, &other, Angle.mulF32);
        return switch (_.*) {
            .deg => |v| Angle{ deg: v * other },
            .rad => |v| Angle{ rad: v * other },
            .grad => |v| Angle{ grad: v * other },
            .turn => |v| Angle{ turn: v * other },
        };
    }

    pub fn partialCmp(other: *const Angle): std.math.Order? {
        return css.generic.partialCmpF32(&_.toDegrees(), &other.toDegrees());
    }

    pub fn tryOp(
        other: *const Angle,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) f32,
    ): Angle? {
        return Angle.op(_, other, ctx, op_fn);
    }

    pub fn tryOpTo(
        other: *const Angle,
        comptime R: type,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) R,
    ): R? {
        return Angle.opTo(_, other, R, ctx, op_fn);
    }

    pub fn op(
        other: *const Angle,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) f32,
    ): Angle {
        if (_.* == .deg && other.* == .deg) {
            return Angle{ deg: op_fn(ctx, _.deg, other.deg) };
        } else if (_.* == .rad && other.* == .rad) {
            return Angle{ rad: op_fn(ctx, _.rad, other.rad) };
        } else if (_.* == .grad && other.* == .grad) {
            return Angle{ grad: op_fn(ctx, _.grad, other.grad) };
        } else if (_.* == .turn && other.* == .turn) {
            return Angle{ turn: op_fn(ctx, _.turn, other.turn) };
        } else {
            return Angle{ deg: op_fn(ctx, _.toDegrees(), other.toDegrees()) };
        }
    }

    pub fn opTo(
        other: *const Angle,
        comptime T: type,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) T,
    ): T {
        if (_.* == .deg && other.* == .deg) {
            return op_fn(ctx, _.deg, other.deg);
        } else if (_.* == .rad && other.* == .rad) {
            return op_fn(ctx, _.rad, other.rad);
        } else if (_.* == .grad && other.* == .grad) {
            return op_fn(ctx, _.grad, other.grad);
        } else if (_.* == .turn && other.* == .turn) {
            return op_fn(ctx, _.turn, other.turn);
        } else {
            return op_fn(ctx, _.toDegrees(), other.toDegrees());
        }
    }

    pub fn sign(): f32 {
        return switch (_.*) {
            .deg, .rad, .grad, .turn => |v| CSSNumberFns.sign(&v),
        };
    }

    pub fn hash(_: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn deepClone(_: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
}

/// A CSS [`<angle-percentage>`](https://www.w3.org/TR/css-values-4/#typedef-angle-percentage) value.
/// May be specified as either an angle or a percentage that resolves to an angle.
pub const AnglePercentage = css.css_values.percentage.DimensionPercentage(Angle);

import bun from "bun";
