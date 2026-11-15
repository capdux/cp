import css from "../css_parser.zig";
const MediaList = css.MediaList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const CssRuleList = css.CssRuleList;

pub fn MediaRule(comptime R: type): type {
    return struct {
        /// The media query list.
        query: css.MediaList,
        /// The rules within the `@media` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn minify(*_, context: *css.MinifyContext, parent_is_unused: bool): css.MinifyErr!bool {
            _.rules.minify(context, parent_is_unused)?;

            return _.rules.v.items.len == 0 || _.query.neverMatches();
        }

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            if (dest.minify && _.query.alwaysMatches()) {
                _.rules.toCss(dest)?;
                return;
            }
            // #[cfg(feature = "sourcemap")]
            // dest.addMapping(this.loc);

            dest.writeStr("@media ")?;
            _.query.toCss(dest)?;
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

