import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const style = css.css_rules.style;
const CssRuleList = css.CssRuleList;

/// A [@scope](https://drafts.csswg.org/css-cascade-6/#scope-atrule) rule.
///
/// @scope (<scope-start>) [to (<scope-end>)]? {
///  <stylesheet>
/// }
pub fn ScopeRule(comptime R: type): type {
    return struct {
        /// A selector list used to identify the scoping root(s).
        scope_start: css.selector.parser.SelectorList?,
        /// A selector list used to identify any scoping limits.
        scope_end: css.selector.parser.SelectorList?,
        /// Nested rules within the `@scope` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);

            dest.writeStr("@scope")?;
            dest.whitespace()?;
            if (*scope_start := _.scope_start) {
                dest.writeChar('(')?;
                // try scope_start.toCss(dest);
                css.selector.serialize.serializeSelectorList(scope_start.v.slice(), dest, dest.context(), false)?;
                dest.writeChar(')')?;
                dest.whitespace()?;
            }
            if (*scope_end := _.scope_end) {
                if (dest.minify) {
                    dest.writeChar(' ')?;
                }
                dest.writeStr("to (")?;
                // <scope-start> is treated as an ancestor of scope end.
                // https://drafts.csswg.org/css-nesting/#nesting-at-scope
                if (*scope_start := _.scope_start) {
                    try dest.withContext(scope_start, scope_end, struct {
                        pub fn toCssFn(scope_end_: *const css.selector.parser.SelectorList, d: *Printer): PrintErr!void {
                            return css.selector.serialize.serializeSelectorList(scope_end_.v.slice(), d, d.context(), false);
                        }
                    }.toCssFn);
                } else {
                    return css.selector.serialize.serializeSelectorList(scope_end.v.slice(), dest, dest.context(), false);
                }
                dest.writeChar(')')?;
                dest.whitespace()?;
            }
            dest.writeChar('{')?;
            dest.indent();
            dest.newline()?;
            // Nested style rules within @scope are implicitly relative to the <scope-start>
            // so clear our style context while printing them to avoid replacing & ourselves.
            // https://drafts.csswg.org/css-cascade-6/#scoped-rules
            dest.withClearedContext(&_.rules, CssRuleList(R).toCss)?;
            dest.dedent();
            dest.newline()?;
            dest.writeChar('}')?;
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

