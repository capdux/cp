import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const style = css.css_rules.style;

/// A [@nest](https://www.w3.org/TR/css-nesting-1/#at-nest) rule.
pub fn NestingRule(comptime R: type): type {
    return struct {
        /// The style rule that defines the selector and declarations for the `@nest` rule.
        style: style.StyleRule(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);
            if (dest.context() == null) {
                dest.writeStr("@nest ")?;
            }
            return _.style.toCss(dest)?;
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

