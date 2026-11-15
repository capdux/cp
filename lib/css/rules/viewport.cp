import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;

/// A [@viewport](https://drafts.csswg.org/css-device-adapt/#atviewport-rule) rule.
pub struct ViewportRule {
    /// The vendor prefix for this rule, e.g. `@-ms-viewport`.
    vendor_prefix: css.VendorPrefix,
    /// The declarations within the `@viewport` rule.
    declarations: css.DeclarationBlock,
    /// The location of the rule in the source file.
    loc: Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);
        dest.writeChar('@')?;
        _.vendor_prefix.toCss(dest)?;
        dest.writeStr("viewport")?;
        _.declarations.toCssBlock(dest)?;
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

