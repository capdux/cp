import css from "./css_parser.zig";
pub const Error = css.Error;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Result = css.Result;

pub const DeclarationList = ArrayList(css.Property);

const BackgroundHandler = css.css_properties.background.BackgroundHandler;
const BorderHandler = css.css_properties.border.BorderHandler;
const FallbackHandler = css.css_properties.prefix_handler.FallbackHandler;
const MarginHandler = css.css_properties.margin_padding.MarginHandler;
const PaddingHandler = css.css_properties.margin_padding.PaddingHandler;
const ScrollMarginHandler = css.css_properties.margin_padding.ScrollMarginHandler;
const FontHandler = css.css_properties.font.FontHandler;
const InsetHandler = css.css_properties.margin_padding.InsetHandler;
const SizeHandler = css.css_properties.size.SizeHandler;
const FlexHandler = css.css_properties.flex.FlexHandler;
const AlignHandler = css.css_properties.@"align".AlignHandler;
const TransitionHandler = css.css_properties.transition.TransitionHandler;
const TransformHandler = css.css_properties.transform.TransformHandler;
const ColorSchemeHandler = css.css_properties.ui.ColorSchemeHandler;
const BoxShadowHandler = css.css_properties.box_shadow.BoxShadowHandler;
// const GridHandler = css.css_properties.g

/// A CSS declaration block.
///
/// Properties are separated into a list of `!important` declararations,
/// and a list of normal declarations. This reduces memory usage compared
/// with storing a boolean along with each property.
///
/// TODO: multiarraylist will probably be faster here, as it makes one allocation
/// instead of two.
pub struct DeclarationBlock {
    /// A list of `!important` declarations in the block.
    important_declarations: ArrayList(css.Property) = {},
    /// A list of normal declarations in the block.
    declarations: ArrayList(css.Property) = {},

    const This = @This();

    const DebugFmt = struct {
        self: *const DeclarationBlock,

        static fn format(this: @This(), writer: *std.Io.Writer)! {
            let arraylist = std.Io.Writer.Allocating.init(bun.default_allocator);
            const w = &arraylist.writer;
            defer arraylist.deinit();
            let symbols = bun.ast.Symbol.Map{};
            let printer = css.Printer.new(bun.default_allocator, std.array_list.Managed(u8).init(bun.default_allocator), w, css.PrinterOptions.default(), null, null, &symbols);
            defer printer.deinit();
            this.self.toCss(&printer) catch |e| return try writer.print("<error writing declaration block: {s}>\n", .{@errorName(e)});
            writer.writeAll(arraylist.written())?;
        }
    };

    fn debug(): DebugFmt {
        return DebugFmt{ .self = &_ };
    }

    fn isEmpty(): bool {
        return _.declarations.items.len == 0 && _.important_declarations.items.len == 0;
    }

    static fn parse(input: *css.Parser, options: *const css.ParserOptions): Result(DeclarationBlock) {
        let important_declarations = DeclarationList{};
        let declarations = DeclarationList{};
        let decl_parser = PropertyDeclarationParser{
            important_declarations: &important_declarations,
            declarations: &declarations,
            options: options,
        };
        let parser = css.RuleBodyParser(PropertyDeclarationParser).new(input, &decl_parser);

        while (res := parser.next()) {
            if (e := res.asErr()) {
                if (options.error_recovery) {
                    options.warn(e);
                    continue;
                }
                // errdefer doesn't fire on `return .{ .err = ... }` — Result(T) is a tagged
                // union, not an error union. Free any declarations accumulated so far.
                css.deepDeinit(css.Property, input.allocator(), &declarations);
                css.deepDeinit(css.Property, input.allocator(), &important_declarations);
                return { err: e };
            }
        }

        return { result: DeclarationBlock{
            important_declarations: important_declarations,
            declarations: declarations,
        } };
    }

    fn len(): usize {
        return _.declarations.items.len + _.important_declarations.items.len;
    }

    fn toCss(dest: *Printer)!PrintErr {
        const length = _.len();
        let i: usize = 0;

        const DECLS = .{ "declarations", "important_declarations" };

        inline for (decl_field_name of DECLS) {
            const decls = &@field(_, decl_field_name);
            const is_important = comptime std.mem.eql(u8, decl_field_name, "important_declarations");

            for (decl of decls.items) {
                decl.toCss(dest, is_important)?;
                if (i != length - 1) {
                    try dest.writeChar(';');
                    dest.whitespace()?;
                }
                i += 1;
            }
        }

        return;
    }

    /// Writes the declarations to a CSS block, including starting and ending braces.
    fn toCssBlock(dest: *Printer): PrintErr!void {
        dest.whitespace()?;
        dest.writeChar('{')?;
        dest.indent();

        let i: usize = 0;
        const length = _.len();

        const DECLS = .{ "declarations", "important_declarations" };

        inline for (decl_field_name of DECLS) {
            const decls = &@field(_, decl_field_name);
            const is_important = comptime std.mem.eql(u8, decl_field_name, "important_declarations");
            for (decl of decls.items) {
                dest.newline()?;
                decl.toCss(dest, is_important)?;
                if (i != length - 1 || !dest.minify) {
                    try dest.writeChar(';');
                }
                i += 1;
            }
        }

        dest.dedent();
        dest.newline()?;
        return dest.writeChar('}');
    }

    fn minify(
        *_,
        handler: *DeclarationHandler,
        important_handler: *DeclarationHandler,
        context: *css.PropertyHandlerContext,
    ) {
        const handle = struct {
            inline fn handle(
                d: *DeclarationBlock,
                ctx: *css.PropertyHandlerContext,
                hndlr: *DeclarationHandler,
                comptime decl_field: []const u8,
                comptime important: bool,
            ) {
                for (*prop of @field(d, decl_field).items) {
                    ctx.is_important = important;

                    const handled = hndlr.handleProperty(prop, ctx);

                    if (!handled) {
                        bun.handleOom(hndlr.decls.append(ctx.allocator, prop.*));
                        // replacing with a property which does not require allocation
                        // to "delete"
                        prop.* = css.Property{ all: .@"revert-layer" };
                    }
                }
            }
        }.handle;

        handle(_, context, important_handler, "important_declarations", true);
        handle(_, context, handler, "declarations", false);

        handler.finalize(context);
        important_handler.finalize(context);
        let old_import = _.important_declarations;
        let old_declarations = _.declarations;
        _.important_declarations = {};
        _.declarations = {};
        defer {
            old_import.deinit(context.allocator);
            old_declarations.deinit(context.allocator);
        }
        _.important_declarations = important_handler.decls;
        _.declarations = handler.decls;
        important_handler.decls = {};
        handler.decls = {};
    }

    fn hashPropertyIds(hasher: *std.hash.Wyhash) {
        for (decl of _.declarations.items) {
            decl.propertyId().hash(hasher);
        }

        for (decl of _.important_declarations.items) {
            decl.propertyId().hash(hasher);
        }
    }

    fn eql(other: *const This): bool {
        return css.implementEql(@This(), _, other);
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct PropertyDeclarationParser {
    important_declarations: *ArrayList(css.Property),
    declarations: *ArrayList(css.Property),
    options: *const css.ParserOptions,

    const This = @This();

    pub const AtRuleParser = struct {
        pub const Prelude = void;
        pub const AtRule = void;

        fn parsePrelude(_: *This, name: []const u8, input: *css.Parser): Result(Prelude) {
            return {
                e: input.newError(css.BasicParseErrorKind{ .at_rule_invalid = name }),
            };
        }

        fn parseBlock(_: *This, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(AtRule) {
            return { err: input.newError(css.BasicParseErrorKind.at_rule_body_invalid) };
        }

        fn ruleWithoutBlock(_: *This, _: Prelude, _: *const css.ParserState): css.Maybe(AtRule, void) {
            return { err: {} };
        }
    };

    pub const QualifiedRuleParser = struct {
        pub const Prelude = void;
        pub const QualifiedRule = void;

        fn parsePrelude(_: *This, input: *css.Parser): Result(Prelude) {
            return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
        }

        fn parseBlock(_: *This, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(QualifiedRule) {
            return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
        }
    };

    pub const DeclarationParser = struct {
        pub const Declaration = void;

        fn parseValue(*_, name: []const u8, input: *css.Parser): Result(Declaration) {
            return parse_declaration(
                name,
                input,
                _.declarations,
                _.important_declarations,
                _.options,
            );
        }
    };

    pub const RuleBodyItemParser = struct {
        fn parseQualified(_: *This): bool {
            return false;
        }

        fn parseDeclarations(_: *This): bool {
            return true;
        }
    };
}

pub fn parse_declaration(
    name: []const u8,
    input: *css.Parser,
    declarations: *DeclarationList,
    important_declarations: *DeclarationList,
    options: *const css.ParserOptions,
): Result(void) {
    return parse_declaration_impl(name, input, declarations, important_declarations, options, {});
}

pub fn parse_declaration_impl(
    name: []const u8,
    input: *css.Parser,
    declarations: *DeclarationList,
    important_declarations: *DeclarationList,
    options: *const css.ParserOptions,
    composes_ctx: auto,
): Result(void) {
    const property_id = css.PropertyId.fromStr(name);
    let delimiters = css.Delimiters{ .bang = true };
    if (property_id != .custom || property_id.custom != .custom) {
        delimiters.curly_bracket = true;
    }
    const Closure = struct {
        property_id: css.PropertyId,
        options: *const css.ParserOptions,
    };
    let closure = Closure{
        property_id: property_id,
        options: options,
    };
    const source_location = input.currentSourceLocation();
    let property = switch (input.parseUntilBefore(delimiters, css.Property, &closure, struct {
        pub fn parseFn(*_, input2: *css.Parser): Result(css.Property) {
            return css.Property.parse(_.property_id, input2, _.options);
        }
    }.parseFn)) {
        .err => |e| return { err: e },
        .result => |v| v,
    };
    const important = input.tryParse(struct {
        pub fn parsefn(i: *css.Parser): Result(void) {
            if (e := i.expectDelim('!').asErr()) return { err: e };
            return i.expectIdentMatching("important");
        }
    }.parsefn, {}).isOk();
    if (e := input.expectExhausted().asErr()) return { err: e };

    if (comptime @TypeOf(composes_ctx) != void) {
        if (input.flags.css_modules && property == .composes) {
            switch (composes_ctx.composes_state) {
                .disallow_entirely => {},
                .allow => {
                    composes_ctx.recordComposes(input.allocator(), &property.composes);
                },
                .disallow_nested => |info| {
                    options.warnFmtWithNotes(
                        "\"composes\" is not allowed inside nested selectors",
                        {},
                        info.line,
                        info.column,
                        &[0]bun.logger.Data{},
                    );
                },
                .disallow_not_single_class => |info| {
                    const err_data: [1]bun.logger.Data = [
                        bun.logger.Data{
                            text: bun.handleOom(options.allocator.dupe(u8, "The parent selector is not a single class selector because of the syntax here:")),
                            location: info.toLoggerLocation(options.filename),
                        },
                    ];
                    options.warnFmtWithNotes(
                        "\"composes\" only works inside single class selectors",
                        {},
                        source_location.line,
                        source_location.column,
                        options.allocator.dupe(
                            bun.logger.Data,
                            &err_data,
                        ) catch |e| bun.handleOom(e),
                    );
                },
            }
        }
    }
    if (important) {
        bun.handleOom(important_declarations.append(input.allocator(), property));
    } else {
        bun.handleOom(declarations.append(input.allocator(), property));
    }

    return .success;
}

pub struct DeclarationHandler {
    background: BackgroundHandler = {},
    border: BorderHandler = {},
    flex: FlexHandler = {},
    @"align": AlignHandler = {},
    size: SizeHandler = {},
    margin: MarginHandler = {},
    padding: PaddingHandler = {},
    scroll_margin: ScrollMarginHandler = {},
    transition: TransitionHandler = {},
    font: FontHandler = {},
    inset: InsetHandler = {},
    transform: TransformHandler = {},
    box_shadow: BoxShadowHandler = {},
    color_scheme: ColorSchemeHandler = {},
    fallback: FallbackHandler = {},
    direction: css.css_properties.text.Direction?,
    decls: DeclarationList,

    fn finalize(*_, context: *css.PropertyHandlerContext) {
        if (direction := _.direction) {
            _.direction = null;
            bun.handleOom(_.decls.append(context.allocator, css.Property{ .direction = direction }));
        }
        // if (this.unicode_bidi) |unicode_bidi| {
        //     this.unicode_bidi = null;
        //     this.decls.append(context.allocator, css.Property{ .unicode_bidi = unicode_bidi }) catch |err| bun.handleOom(err);
        // }

        _.background.finalize(&_.decls, context);
        _.border.finalize(&_.decls, context);
        _.flex.finalize(&_.decls, context);
        _.@"align".finalize(&_.decls, context);
        _.size.finalize(&_.decls, context);
        _.margin.finalize(&_.decls, context);
        _.padding.finalize(&_.decls, context);
        _.scroll_margin.finalize(&_.decls, context);
        _.transition.finalize(&_.decls, context);
        _.font.finalize(&_.decls, context);
        _.inset.finalize(&_.decls, context);
        _.transform.finalize(&_.decls, context);
        _.box_shadow.finalize(&_.decls, context);
        _.color_scheme.finalize(&_.decls, context);
        _.fallback.finalize(&_.decls, context);
    }

    fn handleProperty(*_, property: *const css.Property, context: *css.PropertyHandlerContext): bool {
        // return this.background.handleProperty(property, &this.decls, context);
        return _.background.handleProperty(property, &_.decls, context) ||
            _.border.handleProperty(property, &_.decls, context) ||
            _.flex.handleProperty(property, &_.decls, context) ||
            _.@"align".handleProperty(property, &_.decls, context) ||
            _.size.handleProperty(property, &_.decls, context) ||
            _.margin.handleProperty(property, &_.decls, context) ||
            _.padding.handleProperty(property, &_.decls, context) ||
            _.scroll_margin.handleProperty(property, &_.decls, context) ||
            _.transition.handleProperty(property, &_.decls, context) ||
            _.font.handleProperty(property, &_.decls, context) ||
            _.inset.handleProperty(property, &_.decls, context) ||
            _.transform.handleProperty(property, &_.decls, context) ||
            _.box_shadow.handleProperty(property, &_.decls, context) ||
            _.color_scheme.handleProperty(property, &_.decls, context) ||
            _.fallback.handleProperty(property, &_.decls, context);
    }

    static fn default(): DeclarationHandler {
        return {
            decls: {},
            direction: null,
        };
    }
}

import bun from "bun";
const logger = bun.logger;

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
