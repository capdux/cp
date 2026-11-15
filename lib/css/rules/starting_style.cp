import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const style = css.css_rules.style;
const CssRuleList = css.CssRuleList;

/// A [@starting-style](https://drafts.csswg.org/css-transitions-2/#defining-before-change-style-the-starting-style-rule) rule.
pub fn StartingStyleRule(comptime R: type): type {
    return struct {
        /// Nested rules within the `@starting-style` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);

            dest.writeStr("@starting-style")?;
            dest.whitespace()?;
            dest.writeChar('{')?;
            dest.indent();
            dest.newline()?;
            _.rules.toCss(dest)?;
            dest.dedent();
            dest.newline()?;
            dest.writeChar('}')?;
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

