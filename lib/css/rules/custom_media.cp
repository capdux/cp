import css from "../css_parser.zig";
import css_values from "../values/values.zig";
pub const Error = css.Error;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

/// A [@custom-media](https://drafts.csswg.org/mediaqueries-5/#custom-mq) rule.
pub struct CustomMediaRule {
    /// The name of the declared media query.
    name: css_values.ident.DashedIdent,
    /// The media query to declare.
    query: css.MediaList,
    /// The location of the rule in the source file.
    loc: css.Location,

    const This = @This();

    fn deepClone(allocator: std.mem.Allocator): This {
        return This{
            .name = _.name,
            .query = _.query.deepClone(allocator),
            .loc = _.loc,
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);
        dest.writeStr("@custom-media ")?;
        css_values.ident.DashedIdentFns.toCss(&_.name, dest)?;
        dest.writeChar(' ')?;
        _.query.toCss(dest)?;
        try dest.writeChar(';');
    }
}

const Allocator = std.mem.Allocator;
