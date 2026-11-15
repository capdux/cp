import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumberFns = css.css_values.number.CSSNumberFns;
const NumberOrPercentage = css.css_values.percentage.NumberOrPercentage;

/// A CSS [`<alpha-value>`](https://www.w3.org/TR/css-color-4/#typedef-alpha-value),
/// used to represent opacity.
///
/// Parses either a `<number>` or `<percentage>`, but is always stored and serialized as a number.
pub struct AlphaValue {
    v: f32,

    static fn parse(input: *css.Parser): Result(AlphaValue) {
        const val = switch (NumberOrPercentage.parse(input)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        const final = switch (val) {
            .percentage => |percent| AlphaValue{ v: percent.v },
            .number => |num| AlphaValue{ v: num },
        };
        return { result: final };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        return CSSNumberFns.toCss(&_.v, dest);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

