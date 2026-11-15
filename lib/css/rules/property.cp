import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const Maybe = css.Maybe;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const SyntaxString = css.css_values.syntax.SyntaxString;
const ParsedComponent = css.css_values.syntax.ParsedComponent;

pub struct PropertyRule {
    name: css.css_values.ident.DashedIdent,
    syntax: SyntaxString,
    inherits: bool,
    initial_value: css.css_values.syntax.ParsedComponent?,
    loc: Location,

    static fn parse(name: css.css_values.ident.DashedIdent, input: *css.Parser, loc: Location): Result(PropertyRule) {
        let p = PropertyRuleDeclarationParser{
            syntax: null,
            inherits: null,
            initial_value: null,
        };

        let decl_parser = css.RuleBodyParser(PropertyRuleDeclarationParser).new(input, &p);
        while (decl := decl_parser.next()) {
            if (e := decl.asErr()) {
                return { err: e };
            }
        }

        // `syntax` and `inherits` are always required.
        const parser = decl_parser.parser;
        // TODO(zack): source clones these two, but I omitted here becaues it seems 100% unnecessary
        let syntax: SyntaxString = undefined;
        if (s := parser.syntax) {
            syntax = s;
        } else {
            return { err: decl_parser.input.newCustomError(css.ParserError.at_rule_body_invalid) };
        }

        let inherits: bool = undefined;
        if (inh := parser.inherits) {
            inherits = inh;
        } else {
            return { err: decl_parser.input.newCustomError(css.ParserError.at_rule_body_invalid) };
        }

        // `initial-value` is required unless the syntax is a universal definition.
        let initial_value: ParsedComponent? = null;
        switch (syntax) {
            .universal => {
                if (val := parser.initial_value) {
                    let i = css.ParserInput.new(input.allocator(), val);
                    let p2 = css.Parser.new(&i, null, {}, null);

                    if (p2.isExhausted()) {
                        initial_value = ParsedComponent{
                            .token_list = css.TokenList{
                                .v = {},
                            },
                        };
                    } else {
                        initial_value = switch (syntax.parseValue(&p2)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                    }
                }
            },
            else => {
                let val: []const u8 = undefined;
                if (v := parser.initial_value) {
                    val = v;
                } else {
                    return { err: input.newCustomError(css.ParserError.at_rule_body_invalid) };
                }
                let i = css.ParserInput.new(input.allocator(), val);
                let p2 = css.Parser.new(&i, null, {}, null);
                initial_value = switch (syntax.parseValue(&p2)) {
                    .result => |vv| vv,
                    .err => |e| return { err: e },
                };
            },
        }

        return {
            result: PropertyRule{
                name: name,
                syntax: syntax,
                inherits: inherits,
                initial_value: initial_value,
                loc: loc,
            },
        };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeStr("@property ")?;
        css.css_values.ident.DashedIdentFns.toCss(&_.name, dest)?;
        dest.whitespace()?;
        dest.writeChar('{')?;
        dest.indent();
        dest.newline()?;

        dest.writeStr("syntax:")?;
        dest.whitespace()?;
        _.syntax.toCss(dest)?;
        try dest.writeChar(';');
        dest.newline()?;

        dest.writeStr("inherits:")?;
        dest.whitespace()?;
        if (_.inherits) {
            dest.writeStr("true")?;
        } else {
            dest.writeStr("false")?;
        }

        if (*initial_value := _.initial_value) {
            try dest.writeChar(';');
            dest.newline()?;

            dest.writeStr("initial-value:")?;
            dest.whitespace()?;
            initial_value.toCss(dest)?;

            if (!dest.minify) {
                try dest.writeChar(';');
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

pub struct PropertyRuleDeclarationParser {
    syntax: SyntaxString?,
    inherits: bool?,
    initial_value: ([]const u8)?,

    const This = @This();

    pub const DeclarationParser = struct {
        pub const Declaration = void;
        const Map = bun.ComptimeStringMap(std.@"meta".FieldEnum(PropertyRuleDeclarationParser), .{
            .{ "syntax", .syntax },
            .{ "inherits", .inherits },
            .{ "initial-value", .initial_value },
        });

        fn parseValue(*_, name: []const u8, input: *css.Parser): Result(Declaration) {
            // todo_stuff.match_ignore_ascii_case

            //   if (Map.getASCIIICaseInsensitive(
            //   name)) |field| {
            //     return switch (field) {
            //         .syntax => |syntax| {

            if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("syntax", name)) {
                const syntax = switch (SyntaxString.parse(input)) {
                    .result => |vv| vv,
                    .err => |e| return { err: e },
                };
                _.syntax = syntax;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("inherits", name)) {
                const location = input.currentSourceLocation();
                const ident = switch (input.expectIdent()) {
                    .result => |vv| vv,
                    .err => |e| return { err: e },
                };
                const inherits = if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("true", ident))
                    true
                else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("false", ident))
                    false
                else
                    return { err: location.newUnexpectedTokenError({ ident: ident }) };
                _.inherits = inherits;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("initial-value", name)) {
                // Buffer the value into a string. We will parse it later.
                const start = input.position();
                while (input.next().isOk()) {}
                const initial_value = input.sliceFrom(start);
                _.initial_value = initial_value;
            } else {
                return { err: input.newCustomError(css.ParserError.invalid_declaration) };
            }

            return .success;
        }
    };

    pub const RuleBodyItemParser = struct {
        fn parseQualified(*_): bool {
            return false;
        }

        fn parseDeclarations(*_): bool {
            return true;
        }
    };

    pub const AtRuleParser = struct {
        pub const Prelude = void;
        pub const AtRule = void;

        fn parsePrelude(*_, name: []const u8, input: *css.Parser): Result(Prelude) {
            return { err: input.newError(css.BasicParseErrorKind{ .at_rule_invalid = name }) };
        }

        fn parseBlock(*_, _: AtRuleParser.Prelude, _: *const css.ParserState, input: *css.Parser): Result(AtRuleParser.AtRule) {
            return { err: input.newError(css.BasicParseErrorKind.at_rule_body_invalid) };
        }

        fn ruleWithoutBlock(*_, _: AtRuleParser.Prelude, _: *const css.ParserState): css.Maybe(AtRuleParser.AtRule, void) {
            return { err: {} };
        }
    };

    pub const QualifiedRuleParser = struct {
        pub const Prelude = void;
        pub const QualifiedRule = void;

        fn parsePrelude(*_, input: *css.Parser): Result(Prelude) {
            return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
        }

        fn parseBlock(*_, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(QualifiedRule) {
            return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
        }
    };
}

import bun from "bun";
