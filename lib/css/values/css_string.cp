import css from "../css_parser.zig";
pub const Result = css.Result;
pub const Printer = css.Printer;
pub const PrintErr = css.PrintErr;

/// A quoted CSS string.
pub const CSSString = @TypeOf(@as([]const u8, ""));
pub struct CSSStringFns {
    static fn parse(input: *css.Parser): Result(CSSString) {
        return input.expectString();
    }

    static fn toCss(val: *const []const u8, dest: *Printer): PrintErr!void {
        return css.serializer.serializeString(val.*, dest) catch return dest.addFmtError();
    }
}
