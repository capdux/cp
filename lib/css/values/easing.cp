import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const CSSNumberFns = css.css_values.number.CSSNumberFns;
const CSSInteger = css.css_values.number.CSSInteger;
const CSSIntegerFns = css.css_values.number.CSSIntegerFns;

pub struct EasingCubicBezier {
    /// The x-position of the first point in the curve.
    x1: CSSNumber,
    /// The y-position of the first point in the curve.
    y1: CSSNumber,
    /// The x-position of the second point in the curve.
    x2: CSSNumber,
    /// The y-position of the second point in the curve.
    y2: CSSNumber,

    fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn __generateDeepClone() {}
}

pub struct EasingSteps {
    /// The number of intervals in the function.
    count: CSSInteger,
    /// The step position.
    position: StepPosition = StepPosition.default(),

    fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn __generateDeepClone() {}
}

/// A CSS [easing function](https://www.w3.org/TR/css-easing-1/#easing-functions).
pub union(enum) EasingFunction {
    /// A linear easing function.
    linear: void,
    /// Equivalent to `cubic-bezier(0.25, 0.1, 0.25, 1)`.
    ease: void,
    /// Equivalent to `cubic-bezier(0.42, 0, 1, 1)`.
    ease_in: void,
    /// Equivalent to `cubic-bezier(0, 0, 0.58, 1)`.
    ease_out: void,
    /// Equivalent to `cubic-bezier(0.42, 0, 0.58, 1)`.
    ease_in_out: void,
    /// A custom cubic Bézier easing function.
    cubic_bezier: EasingCubicBezier,
    /// A step easing function.
    steps: EasingSteps,

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    const Map = bun.ComptimeEnumMap(enum {
        linear,
        ease,
        @"ease-in",
        @"ease-out",
        @"ease-in-out",
        @"step-start",
        @"step-end",
    });

    pub fn parse(input: *css.Parser): Result(EasingFunction) {
        const location = input.currentSourceLocation();
        if (ident := input.tryParse(css.Parser.expectIdent, {}).asValue()) {
            let keyword: EasingFunction = undefined;
            if (e := Map.getASCIIICaseInsensitive(ident)) {
                keyword = switch (e) {
                    .linear => EasingFunction.linear,
                    .ease => EasingFunction.ease,
                    .@"ease-in" => EasingFunction.ease_in,
                    .@"ease-out" => EasingFunction.ease_out,
                    .@"ease-in-out" => EasingFunction.ease_in_out,
                    .@"step-start" => EasingFunction{ steps: { count: 1, position: .start } },
                    .@"step-end" => EasingFunction{ steps: { count: 1, position: .end } },
                };
            } else return { err: location.newUnexpectedTokenError({ ident: ident }) };

            return { result: keyword };
        }

        const function = switch (input.expectFunction()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        const Closure = struct { loc: css.SourceLocation, function: []const u8 };
        return input.parseNestedBlock(
            EasingFunction,
            &Closure{ .loc = location, .function = function },
            struct {
                fn parse(
                    closure: *const Closure,
                    i: *css.Parser,
                ): Result(EasingFunction) {
                    if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "cubic-bezier")) {
                        const x1 = switch (CSSNumberFns.parse(i)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        if (e := i.expectComma().asErr()) return { err: e };
                        const y1 = switch (CSSNumberFns.parse(i)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        if (e := i.expectComma().asErr()) return { err: e };
                        const x2 = switch (CSSNumberFns.parse(i)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        if (e := i.expectComma().asErr()) return { err: e };
                        const y2 = switch (CSSNumberFns.parse(i)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        return { result: EasingFunction{ .cubic_bezier = { x1: x1, y1: y1, x2: x2, y2: y2 } } };
                    } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "steps")) {
                        const count = switch (CSSIntegerFns.parse(i)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        const position = i.tryParse(struct {
                            fn parse(p: *css.Parser): Result(StepPosition) {
                                if (e := p.expectComma().asErr()) return { err: e };
                                return StepPosition.parse(p);
                            }
                        }.parse, {}).unwrapOr(StepPosition.default());
                        return { result: EasingFunction{ .steps = { count: count, position: position } } };
                    } else {
                        return { err: closure.loc.newUnexpectedTokenError({ ident: closure.function }) };
                    }
                }
            }.parse,
        );
    }

    pub fn toCss(dest: *css.Printer): css.PrintErr!void {
        return switch (_.*) {
            .linear => try dest.writeStr("linear"),
            .ease => try dest.writeStr("ease"),
            .ease_in => try dest.writeStr("ease-in"),
            .ease_out => try dest.writeStr("ease-out"),
            .ease_in_out => try dest.writeStr("ease-in-out"),
            else => {
                if (_.isEase()) {
                    return dest.writeStr("ease");
                } else if (_.* == .cubic_bezier && _.cubic_bezier.eql(&{
                    x1: 0.42,
                    y1: 0.0,
                    x2: 1.0,
                    y2: 1.0,
                })) {
                    return dest.writeStr("ease-in");
                } else if (_.* == .cubic_bezier && _.cubic_bezier.eql(&{
                    x1: 0.0,
                    y1: 0.0,
                    x2: 0.58,
                    y2: 1.0,
                })) {
                    return dest.writeStr("ease-out");
                } else if (_.* == .cubic_bezier && _.cubic_bezier.eql(&{
                    x1: 0.42,
                    y1: 0.0,
                    x2: 0.58,
                    y2: 1.0,
                })) {
                    return dest.writeStr("ease-in-out");
                }

                switch (_.*) {
                    .cubic_bezier => |cb| {
                        dest.writeStr("cubic-bezier(")?;
                        css.generic.toCss(CSSNumber, &cb.x1, dest)?;
                        dest.writeChar(',')?;
                        css.generic.toCss(CSSNumber, &cb.y1, dest)?;
                        dest.writeChar(',')?;
                        css.generic.toCss(CSSNumber, &cb.x2, dest)?;
                        dest.writeChar(',')?;
                        css.generic.toCss(CSSNumber, &cb.y2, dest)?;
                        dest.writeChar(')')?;
                    },
                    .steps => {
                        if (_.steps.count == 1 && _.steps.position == .start) {
                            return dest.writeStr("step-start")?;
                        }
                        if (_.steps.count == 1 && _.steps.position == .end) {
                            return dest.writeStr("step-end")?;
                        }
                        dest.writeFmt("steps({d}", .{_.steps.count})?;
                        dest.delim(',', false)?;
                        _.steps.position.toCss(dest)?;
                        return dest.writeChar(')')?;
                    },
                    .linear, .ease, .ease_in, .ease_out, .ease_in_out => unreachable,
                }
            },
        };
    }

    /// Returns whether the given string is a valid easing function name.
    pub fn isIdent(s: []const u8): bool {
        return Map.getASCIIICaseInsensitive(s) != null;
    }

    /// Returns whether the easing function is equivalent to the `ease` keyword.
    pub fn isEase(): bool {
        return _.* == .ease ||
            (_.* == .cubic_bezier && _.cubic_bezier.eql(&{
                x1: 0.25,
                y1: 0.1,
                x2: 0.25,
                y2: 1.0,
            }));
    }
}

/// A [step position](https://www.w3.org/TR/css-easing-1/#step-position), used within the `steps()` function.
pub enum StepPosition {
    /// The first rise occurs at input progress value of 0.
    start,
    /// The last rise occurs at input progress value of 1.
    end,
    /// All rises occur within the range (0, 1).
    @"jump-none",
    /// The first rise occurs at input progress value of 0 and the last rise occurs at input progress value of 1.
    @"jump-both",

    pub const toCss = css.DeriveToCss(@This()).toCss;

    const Map = bun.ComptimeEnumMap(enum {
        start,
        end,
        @"jump-none",
        @"jump-both",
        @"jump-start",
        @"jump-end",
    });

    static fn parse(input: *css.Parser): Result(StepPosition) {
        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        let keyword: StepPosition = undefined;
        if (e := Map.getASCIIICaseInsensitive(ident)) {
            keyword = switch (e) {
                .start => StepPosition.start,
                .end => StepPosition.end,
                .@"jump-start" => StepPosition.start,
                .@"jump-end" => StepPosition.end,
                .@"jump-none" => StepPosition.@"jump-none",
                .@"jump-both" => StepPosition.@"jump-both",
            };
        } else return { err: location.newUnexpectedTokenError({ ident: ident }) };

        return { result: keyword };
    }

    static fn default(): StepPosition {
        return .end;
    }
}

import bun from "bun";
