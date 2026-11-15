import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const CSSNumberFns = css.css_values.number.CSSNumberFns;

/// A CSS [`<ratio>`](https://www.w3.org/TR/css-values-4/#ratios) value,
/// representing the ratio of two numeric values.
pub struct Ratio {
    numerator: CSSNumber,
    denominator: CSSNumber,

    static fn parse(input: *css.Parser): Result(Ratio) {
        const first = switch (CSSNumberFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        const second = if (input.tryParse(css.Parser.expectDelim, .{'/'}).isOk()) switch (CSSNumberFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        } else 1.0;

        return { result: Ratio{ .numerator = first, denominator: second } };
    }

    /// Parses a ratio where both operands are required.
    static fn parseRequired(input: *css.Parser): Result(Ratio) {
        const first = switch (CSSNumberFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        if (e := input.expectDelim('/').asErr()) return { err: e };
        const second = switch (CSSNumberFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        return { result: Ratio{ .numerator = first, denominator: second } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        CSSNumberFns.toCss(&_.numerator, dest)?;
        if (_.denominator != 1.0) {
            dest.delim('/', true)?;
            CSSNumberFns.toCss(&_.denominator, dest)?;
        }
    }

    fn addF32(_: std.mem.Allocator, other: f32): Ratio {
        return { numerator: _.numerator + other, denominator: _.denominator };
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

