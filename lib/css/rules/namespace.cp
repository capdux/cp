import css from "../css_parser.zig";
import css_values from "../values/values.zig";
pub const Error = css.Error;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

/// A [@namespace](https://drafts.csswg.org/css-namespaces/#declaration) rule.
pub struct NamespaceRule {
    /// An optional namespace prefix to declare, or `None` to declare the default namespace.
    prefix: css.Ident?,
    /// The url of the namespace.
    url: css.CSSString,
    /// The location of the rule in the source file.
    loc: css.Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeStr("@namespace ")?;
        if (*prefix := _.prefix) {
            css.css_values.ident.IdentFns.toCss(prefix, dest)?;
            dest.writeChar(' ')?;
        }

        css.css_values.string.CSSStringFns.toCss(&_.url, dest)?;
        try dest.writeChar(';');
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

const Allocator = std.mem.Allocator;
