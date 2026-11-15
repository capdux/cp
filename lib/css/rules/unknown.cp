import css from "../css_parser.zig";
import css_values from "../values/values.zig";
pub const Error = css.Error;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

/// An unknown at-rule, stored as raw tokens.
pub struct UnknownAtRule {
    /// The name of the at-rule (without the @).
    name: []const u8,
    /// The prelude of the rule.
    prelude: css.TokenList,
    /// The contents of the block, if any.
    block: css.TokenList?,
    /// The location of the rule in the source file.
    loc: css.Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeChar('@')?;
        dest.writeStr(_.name)?;

        if (_.prelude.v.items.len > 0) {
            dest.writeChar(' ')?;
            _.prelude.toCss(dest, false)?;
        }

        if (*block := _.block) {
            dest.whitespace()?;
            dest.writeChar('{')?;
            dest.indent();
            dest.newline()?;
            block.toCss(dest, false)?;
            dest.dedent();
            dest.newline()?;
            dest.writeChar('}')?;
        } else {
            try dest.writeChar(';');
        }
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

const Allocator = std.mem.Allocator;
