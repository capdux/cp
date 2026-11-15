import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;

/// A [@counter-style](https://drafts.csswg.org/css-counter-styles/#the-counter-style-rule) rule.
pub struct CounterStyleRule {
    /// The name of the counter style to declare.
    name: css.css_values.ident.CustomIdent,
    /// Declarations in the `@counter-style` rule.
    declarations: css.DeclarationBlock,
    /// The location of the rule in the source file.
    loc: Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeStr("@counter-style")?;
        css.css_values.ident.CustomIdentFns.toCss(&_.name, dest)?;
        _.declarations.toCssBlock(dest)?;
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

