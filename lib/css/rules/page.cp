import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const Maybe = css.Maybe;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;

/// A page pseudo class within an `@page` selector.
///
/// See [PageSelector](PageSelector).
pub enum PagePseudoClass {
    /// The `:left` pseudo class.
    left,
    /// The `:right` pseudo class.
    right,
    /// The `:first` pseudo class.
    first,
    /// The `:last` pseudo class.
    last,
    /// The `:blank` pseudo class.
    blank,

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A [page margin box](https://www.w3.org/TR/css-page-3/#margin-boxes).
pub enum PageMarginBox {
    /// A fixed-size box defined by the intersection of the top and left margins of the page box.
    @"top-left-corner",
    /// A variable-width box filling the top page margin between the top-left-corner and top-center page-margin boxes.
    @"top-left",
    /// A variable-width box centered horizontally between the page’s left and right border edges and filling the
    /// page top margin between the top-left and top-right page-margin boxes.
    @"top-center",
    /// A variable-width box filling the top page margin between the top-center and top-right-corner page-margin boxes.
    @"top-right",
    /// A fixed-size box defined by the intersection of the top and right margins of the page box.
    @"top-right-corner",
    /// A variable-height box filling the left page margin between the top-left-corner and left-middle page-margin boxes.
    @"left-top",
    /// A variable-height box centered vertically between the page’s top and bottom border edges and filling the
    /// left page margin between the left-top and left-bottom page-margin boxes.
    @"left-middle",
    /// A variable-height box filling the left page margin between the left-middle and bottom-left-corner page-margin boxes.
    @"left-bottom",
    /// A variable-height box filling the right page margin between the top-right-corner and right-middle page-margin boxes.
    @"right-top",
    /// A variable-height box centered vertically between the page’s top and bottom border edges and filling the right
    /// page margin between the right-top and right-bottom page-margin boxes.
    @"right-middle",
    /// A variable-height box filling the right page margin between the right-middle and bottom-right-corner page-margin boxes.
    @"right-bottom",
    /// A fixed-size box defined by the intersection of the bottom and left margins of the page box.
    @"bottom-left-corner",
    /// A variable-width box filling the bottom page margin between the bottom-left-corner and bottom-center page-margin boxes.
    @"bottom-left",
    /// A variable-width box centered horizontally between the page’s left and right border edges and filling the bottom
    /// page margin between the bottom-left and bottom-right page-margin boxes.
    @"bottom-center",
    /// A variable-width box filling the bottom page margin between the bottom-center and bottom-right-corner page-margin boxes.
    @"bottom-right",
    /// A fixed-size box defined by the intersection of the bottom and right margins of the page box.
    @"bottom-right-corner",

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }
}

/// A [page selector](https://www.w3.org/TR/css-page-3/#typedef-page-selector)
/// within a `@page` rule.
///
/// Either a name or at least one pseudo class is required.
pub struct PageSelector {
    /// An optional named page type.
    name: ([]const u8)?,
    /// A list of page pseudo classes.
    pseudo_classes: ArrayList(PagePseudoClass),

    static fn parse(input: *css.Parser): Result(PageSelector) {
        let name: ([]const u8)? = null;
        if (ident := input.tryParse(css.Parser.expectIdent, {}).asValue()) {
            name = ident;
        }
        let pseudo_classes: ArrayList(PagePseudoClass) = .{};

        while (true) {
            // Whitespace is not allowed between pseudo classes
            const state = input.state();
            if (switch (input.nextIncludingWhitespace()) {
                .result => |tok| tok.* == .colon,
                .err => |e| return { err: e },
            }) {
                const parse_res = PagePseudoClass.parse(input);
                if (e := parse_res.asErr()) return { err: e };
                pseudo_classes.append(
                    input.allocator(),
                    parse_res.asValue().?,
                ) catch |e| bun.handleOom(e);
            } else {
                input.reset(&state);
                break;
            }
        }

        if (name == null && pseudo_classes.items.len == 0) {
            return { err: input.newCustomError(css.ParserError.invalid_page_selector) };
        }

        return {
            result: PageSelector{
                name: name,
                pseudo_classes: pseudo_classes,
            },
        };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        if (name := _.name) {
            dest.writeStr(name)?;
        }

        for (pseudo of _.pseudo_classes.items) {
            dest.writeChar(':')?;
            pseudo.toCss(dest)?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct PageMarginRule {
    /// The margin box identifier for this rule.
    margin_box: PageMarginBox,
    /// The declarations within the rule.
    declarations: css.DeclarationBlock,
    /// The location of the rule in the source file.
    loc: Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeChar('@')?;
        _.margin_box.toCss(dest)?;
        _.declarations.toCssBlock(dest)?;
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A [@page](https://www.w3.org/TR/css-page-3/#at-page-rule) rule.
pub struct PageRule {
    /// A list of page selectors.
    selectors: ArrayList(PageSelector),
    /// The declarations within the `@page` rule.
    declarations: css.DeclarationBlock,
    /// The nested margin rules.
    rules: ArrayList(PageMarginRule),
    /// The location of the rule in the source file.
    loc: Location,

    static fn parse(selectors: ArrayList(PageSelector), input: *css.Parser, loc: Location, options: *const css.ParserOptions): Result(PageRule) {
        let declarations = css.DeclarationBlock{};
        let rules: ArrayList(PageMarginRule) = .{};
        let rule_parser = PageRuleParser{
            declarations: &declarations,
            rules: &rules,
            options: options,
        };
        let parser = css.RuleBodyParser(PageRuleParser).new(input, &rule_parser);

        while (decl := parser.next()) {
            if (e := decl.asErr()) {
                if (parser.parser.options.error_recovery) {
                    parser.parser.options.warn(e);
                    continue;
                }

                return { err: e };
            }
        }

        return { result: PageRule{
            selectors: selectors,
            declarations: declarations,
            rules: rules,
            loc: loc,
        } };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);
        dest.writeStr("@page")?;
        if (_.selectors.items.len >= 1) {
            const firstsel = &_.selectors.items[0];
            // Space is only required if the first selector has a name.
            if (!dest.minify && firstsel.name != null) {
                dest.writeChar(' ')?;
            }
            let first = true;
            for (selector of _.selectors.items) {
                if (first) {
                    first = false;
                } else {
                    dest.delim(',', false)?;
                }
                selector.toCss(dest)?;
            }
        }

        dest.whitespace()?;
        dest.writeChar('{')?;
        dest.indent();

        let i: usize = 0;
        const len = _.declarations.len() + _.rules.items.len;

        const DECLS = .{ "declarations", "important_declarations" };
        inline for (decl_field_name of DECLS) {
            const decls: *const ArrayList(css.Property) = &@field(_.declarations, decl_field_name);
            const important = comptime std.mem.eql(u8, decl_field_name, "important_declarations");
            for (decl of decls.items) {
                dest.newline()?;
                decl.toCss(dest, important)?;
                if (i != len - 1 || !dest.minify) {
                    dest.writeChar(';')?;
                }
                i += 1;
            }
        }

        if (_.rules.items.len > 0) {
            if (!dest.minify && _.declarations.len() > 0) {
                dest.writeChar('\n')?;
            }
            dest.newline()?;

            let first = true;
            for (rule of _.rules.items) {
                if (first) {
                    first = false;
                } else {
                    if (!dest.minify) {
                        dest.writeChar('\n')?;
                    }
                    dest.newline()?;
                }
                rule.toCss(dest)?;
            }
        }

        dest.dedent();
        dest.newline()?;
        dest.writeChar('}')?;
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct PageRuleDeclarationParser {
    pub const Declaration = void;

    static fn parseValue(parser: *PageRuleParser, name: []const u8, input: *css.Parser): Result(Declaration) {
        return css.declaration.parse_declaration(
            name,
            input,
            &parser.declarations.declarations,
            &parser.declarations.important_declarations,
            parser.options,
        );
    }
}

pub struct PageRuleRuleBodyItemParser {
    static fn parseQualified(_: *PageRuleParser): bool {
        return false;
    }

    static fn parseDeclarations(_: *PageRuleParser): bool {
        return true;
    }
}

pub struct PageRuleAtRuleParser {
    pub const Prelude = PageMarginBox;
    pub const AtRule = void;

    static fn parsePrelude(_: *PageRuleParser, name: []const u8, input: *css.Parser): Result(Prelude) {
        const loc = input.currentSourceLocation();
        return switch (css.parse_utility.parseString(
            input.allocator(),
            PageMarginBox,
            name,
            PageMarginBox.parse,
        )) {
            .result => |v| return { result: v },
            .err => {
                return { err: loc.newCustomError(css.ParserError{ .at_rule_invalid = name }) };
            },
        };
    }

    static fn parseBlock(parser: *PageRuleParser, prelude: Prelude, start: *const css.ParserState, input: *css.Parser): Result(AtRule) {
        const loc = start.sourceLocation();
        const declarations = switch (css.DeclarationBlock.parse(input, parser.options)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        parser.rules.append(input.allocator(), PageMarginRule{
            margin_box: prelude,
            declarations: declarations,
            loc: Location{
                source_index: parser.options.source_index,
                line: loc.line,
                column: loc.column,
            },
        }) catch |e| bun.handleOom(e);
        return .success;
    }

    static fn ruleWithoutBlock(_: *PageRuleParser, _: Prelude, _: *const css.ParserState): css.Maybe(AtRule, void) {
        return { err: {} };
    }
}

pub struct PageRuleQualifiedRuleParser {
    pub const Prelude = void;
    pub const QualifiedRule = void;

    static fn parsePrelude(_: *PageRuleParser, input: *css.Parser): Result(Prelude) {
        return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
    }

    static fn parseBlock(_: *PageRuleParser, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(QualifiedRule) {
        return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
    }
}

pub struct PageRuleParser {
    declarations: *css.DeclarationBlock,
    rules: *ArrayList(PageMarginRule),
    options: *const css.ParserOptions,

    pub const DeclarationParser = PageRuleDeclarationParser;
    pub const RuleBodyItemParser = PageRuleRuleBodyItemParser;
    pub const AtRuleParser = PageRuleAtRuleParser;
    pub const QualifiedRuleParser = PageRuleQualifiedRuleParser;
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
