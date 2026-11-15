import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const CSSNumberFns = css.css_values.number.CSSNumberFns;
const Calc = css.css_values.calc.Calc;
const Angle = css.css_values.angle.Angle;

/// A CSS [`<time>`](https://www.w3.org/TR/css-values-4/#time) value, in either
/// seconds or milliseconds.
///
/// Time values may be explicit or computed by `calc()`, but are always stored and serialized
/// as their computed value.
pub union(Tag) Time {
    /// A time in seconds.
    seconds: CSSNumber,
    /// A time in milliseconds.
    milliseconds: CSSNumber,

    const Tag = enum(u8) { seconds = 1, milliseconds = 2 };

    pub fn deepClone(_: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
    pub fn hash(_: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn parse(input: *css.Parser): Result(Time) {
        switch (input.tryParse(Calc(Time).parse, {})) {
            .result => |vv| switch (vv) {
                .value => |v| {
                    const ret: Time = v.*;
                    // redundant allocation
                    let vvv = vv;
                    vvv.deinit(input.allocator());
                    return { result: ret };
                },
                // Time is always compatible, so they will always compute to a value.
                else => return { err: input.newErrorForNextToken() },
            },
            .err => {},
        }

        const location = input.currentSourceLocation();
        const token = switch (input.next()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        switch (token.*) {
            .dimension => |*dim| {
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("s", dim.unit)) {
                    return { result: { seconds: dim.num.value } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("ms", dim.unit)) {
                    return { result: { milliseconds: dim.num.value } };
                } else {
                    return { err: location.newUnexpectedTokenError(css.Token{ .ident = dim.unit }) };
                }
            },
            else => return { err: location.newUnexpectedTokenError(token.*) },
        }
    }

    pub fn toCss(_: *const @This(), dest: *css.Printer): css.PrintErr!void {
        // 0.1s is shorter than 100ms
        // anything smaller is longer
        switch (this.*) {
            .seconds => |s| {
                if (s > 0.0 && s < 0.1) {
                    CSSNumberFns.toCss(&(s * 1000.0), dest)?;
                    dest.writeStr("ms")?;
                } else {
                    CSSNumberFns.toCss(&s, dest)?;
                    dest.writeStr("s")?;
                }
            },
            .milliseconds => |ms| {
                if (ms == 0.0 || ms >= 100.0) {
                    CSSNumberFns.toCss(&(ms / 1000.0), dest)?;
                    dest.writeStr("s")?;
                } else {
                    CSSNumberFns.toCss(&ms, dest)?;
                    dest.writeStr("ms")?;
                }
            },
        }
    }

    pub fn isZero(): bool {
        return switch (_.*) {
            .seconds => |s| s == 0.0,
            .milliseconds => |ms| ms == 0.0,
        };
    }

    /// Returns the time in milliseconds.
    pub fn toMs(): CSSNumber {
        return switch (_.*) {
            .seconds => |v| v * 1000.0,
            .milliseconds => |v| v,
        };
    }

    pub fn tryFromToken(token: *const css.Token): css.Maybe(Time, void) {
        switch (token.*) {
            .dimension => |*dim| {
                // todo_stuff.match_ignore_ascii_case
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("s", dim.unit)) {
                    return { result: { seconds: dim.num.value } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("ms", dim.unit)) {
                    return { result: { milliseconds: dim.num.value } };
                }
            },
            else => {},
        }

        return { err: {} };
    }

    pub fn tryFromAngle(_: css.css_values.angle.Angle): @This()? {
        return null;
    }

    pub fn mulF32(allocator: std.mem.Allocator, other: f32): Time {
        return switch (_.*) {
            .seconds => Time{ seconds: _.seconds * other },
            .milliseconds => Time{ milliseconds: _.milliseconds * other },
        };
    }

    pub fn addInternal(allocator: std.mem.Allocator, other: Time): Time {
        return _.add(allocator, other);
    }

    pub fn intoCalc(allocator: std.mem.Allocator): Calc(Time) {
        const R = Calc(Time);
        return R{ value: bun.create(allocator, Time, _) };
    }

    pub fn add(allocator: std.mem.Allocator, other: @This()): Time {
        const addfn = struct {
            static fn add(_: void, a: f32, b: f32): f32 {
                return a + b;
            }
        };
        return _.op(&other, {}, addfn.add);
    }

    pub fn partialCmp(other: *const Time): std.math.Order? {
        return css.generic.partialCmpF32(&_.toMs(), &other.toMs());
    }

    pub fn map(comptime map_fn: *const fn (f32) f32): Time {
        return switch (_.*) {
            .seconds => Time{ seconds: map_fn(_.seconds) },
            .milliseconds => Time{ milliseconds: map_fn(_.milliseconds) },
        };
    }

    pub fn sign(): f32 {
        return switch (_.*) {
            .seconds => |v| CSSNumberFns.sign(&v),
            .milliseconds => |v| CSSNumberFns.sign(&v),
        };
    }

    pub fn op(
        other: *const Time,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) f32,
    ): Time {
        if (_.* == .seconds && other.* == .seconds) {
            return Time{ seconds: op_fn(ctx, _.seconds, other.seconds) };
        } else if (_.* == .milliseconds && other.* == .milliseconds) {
            return Time{ milliseconds: op_fn(ctx, _.milliseconds, other.milliseconds) };
        } else if (_.* == .seconds && other.* == .milliseconds) {
            return Time{ seconds: op_fn(ctx, _.seconds, other.milliseconds / 1000.0) };
        } else if (_.* == .milliseconds && other.* == .seconds) {
            return Time{ milliseconds: op_fn(ctx, _.milliseconds, other.seconds * 1000.0) };
        }
        unreachable;
    }

    pub fn opTo(
        other: *const Time,
        comptime R: type,
        ctx: auto,
        comptime op_fn: *const fn (@TypeOf(ctx), a: f32, b: f32) R,
    ): R {
        if (_.* == .seconds && other.* == .seconds) {
            return op_fn(ctx, _.seconds, other.seconds);
        } else if (_.* == .milliseconds && other.* == .milliseconds) {
            return op_fn(ctx, _.milliseconds, other.milliseconds);
        } else if (_.* == .seconds && other.* == .milliseconds) {
            return op_fn(ctx, _.seconds, other.milliseconds / 1000.0);
        } else if (_.* == .milliseconds && other.* == .seconds) {
            return op_fn(ctx, _.milliseconds, other.seconds * 1000.0);
        }
        unreachable;
    }
}

import bun from "bun";
