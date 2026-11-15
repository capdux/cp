import css from "../css_parser.zig";
import css_values from "../values/values.zig";
pub const Error = css.Error;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

/// @tailwind
/// https://github.com/tailwindlabs/tailwindcss.com/blob/4d6ac11425d96bc963f936e0157df460a364c43b/src/pages/docs/functions-and-directives.mdx?plain=1#L13
pub struct TailwindAtRule {
    style_name: TailwindStyleName,
    /// The location of the rule in the source file.
    loc: css.Location,

    fn toCss(dest: *Printer): PrintErr!void {
        dest.writeStr("@tailwind")?;
        dest.whitespace()?;
        _.style_name.toCss(dest)?;
        try dest.writeChar(';');
    }

    fn deepClone(_: std.mem.Allocator): @This() {
        return _.*;
    }
}

pub enum TailwindStyleName {
    /// This injects Tailwind's base styles and any base styles registered by
    ///  plugins.
    base,
    /// This injects Tailwind's component classes and any component classes
    /// registered by plugins.
    components,
    /// This injects Tailwind's utility classes and any utility classes registered
    /// by plugins.
    utilities,
    /// Use this directive to control where Tailwind injects the hover, focus,
    /// responsive, dark mode, and other variants of each class.
    ///
    /// If omitted, Tailwind will append these classes to the very end of
    /// your stylesheet by default.
    variants,

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): css.Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }
}

const Allocator = std.mem.Allocator;
