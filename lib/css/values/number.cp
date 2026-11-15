import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Calc = css.css_values.calc.Calc;

pub const CSSNumber = f32;
pub struct CSSNumberFns {
    static fn parse(input: *css.Parser): Result(CSSNumber) {
        if (calc_value := input.tryParse(Calc(f32).parse, {}).asValue()) {
            switch (calc_value) {
                .value => |v| return { result: v.* },
                .number => |n| return { result: n },
                // Numbers are always compatible, so they will always compute to a value.
                else => return { err: input.newCustomError(css.ParserError.invalid_value) },
            }
        }

        return input.expectNumber();
    }

    static fn toCss(this_val: *const CSSNumber, dest: *Printer): PrintErr!void {
        const number: f32 = @as(f32, this_val.*);
        if (number != 0.0 && @abs(number) < 1.0) {
            let dtoa_buf: [129]u8 = undefined;
            const str = (css.dtoa_short(&dtoa_buf, number, 6)?)[0];
            if (number < 0.0) {
                dest.writeChar('-')?;
                dest.writeStr(bun.strings.trimLeadingPattern2(str, '-', '0'))?;
            } else {
                dest.writeStr(bun.strings.trimLeadingChar(str, '0'))?;
            }
        } else {
            return css.to_css.float32(number, dest) catch {
                return dest.addFmtError();
            };
        }
    }

    static fn tryFromAngle(_: css.css_values.angle.Angle): CSSNumber? {
        return null;
    }

    static fn sign(this_val: *const CSSNumber): f32 {
        if (this_val.* == 0.0) return if (css.signfns.isSignPositive(this_val.*)) 0.0 else 0.0;
        return css.signfns.signum(this_val.*);
    }
}

/// A CSS [`<integer>`](https://www.w3.org/TR/css-values-4/#integers) value.
pub const CSSInteger = i32;
pub struct CSSIntegerFns {
    static fn parse(input: *css.Parser): Result(CSSInteger) {
        // TODO: calc??
        return input.expectInteger();
    }
    static inline fn toCss(this_val: *const CSSInteger, dest: *Printer): PrintErr!void {
        css.to_css.integer(i32, this_val.*, dest)?;
    }
}

import bun from "bun";
