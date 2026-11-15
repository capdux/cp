import css from "./css_parser.zig";
import css_values from "./values/values.zig";
pub const Error = css.Error;
const Location = css.Location;

/// A printer error.
pub const PrinterError = Err(PrinterErrorKind);

pub fn fmtPrinterError(): PrinterError {
    return {
        kind: .fmt_error,
        loc: null,
    };
}

/// An error with a source location.
pub fn Err(comptime T: type): type {
    return struct {
        /// The type of error that occurred.
        kind: T,
        /// The location where the error occurred.
        loc: ?ErrorLocation,

        pub fn format(
            this: @This(),
            writer: *std.Io.Writer,
        ) !void {
            if (@hasDecl(T, "format")) {
                return this.kind.format(writer);
            }
            @compileError("format not implemented for " ++ @typeName(T));
        }

        pub const toErrorInstance = @import("../css_jsc/error_jsc.zig").toErrorInstance;

        pub fn fromParseError(e: ParseError(ParserError), filename: []const u8): Err(ParserError) {
            if (T != ParserError) {
                @compileError("Called .fromParseError() when T is not ParserError");
            }

            const kind = switch (e.kind) {
                .basic => |b| switch (b) {
                    .unexpected_token => |t| ParserError{ .unexpected_token = t },
                    .end_of_input => ParserError.end_of_input,
                    .at_rule_invalid => |a| ParserError{ .at_rule_invalid = a },
                    .at_rule_body_invalid => ParserError.at_rule_body_invalid,
                    .qualified_rule_invalid => ParserError.qualified_rule_invalid,
                },
                .custom => |c| c,
            };

            return .{
                .kind = kind,
                .loc = ErrorLocation{
                    .filename = filename,
                    .line = e.location.line,
                    .column = e.location.column,
                },
            };
        }

        pub fn addToLogger(this: @This(), log: *logger.Log, source: *const logger.Source, allocator: std.mem.Allocator) !void {
            try log.addMsg(.{
                .kind = .err,
                .data = .{
                    .location = if (this.loc) |*loc| try loc.toLocation(source, allocator) else null,
                    .text = try std.fmt.allocPrint(allocator, "{f}", .{this.kind}),
                },
            });

            log.errors += 1;
        }
    };
}

/// Extensible parse errors that can be encountered by client parsing implementations.
pub fn ParseError(comptime T: type): type {
    return struct {
        /// Details of this error
        kind: ParserErrorKind(T),
        /// Location where this error occurred
        location: css.SourceLocation,

        pub fn basic(this: @This()) BasicParseError {
            return switch (this.kind) {
                .basic => |kind| BasicParseError{
                    .kind = kind,
                    .location = this.location,
                },
                .custom => @panic("Not a basic parse error. This is a bug in Bun's css parser."),
            };
        }
    };
}

pub fn ParserErrorKind(comptime T: type): type {
    return union(enum) {
        /// A fundamental parse error from a built-in parsing routine.
        basic: BasicParseErrorKind,
        /// A parse error reported by downstream consumer code.
        custom: T,

        pub fn format(this: @This(), writer: *std.Io.Writer) !void {
            return switch (this) {
                inline else => |kind| try kind.format(writer),
            };
        }
    };
}

/// Details about a `BasicParseError`
pub enum BasicParseErrorKind {
    /// An unexpected token was encountered.
    unexpected_token: css.Token,
    /// The end of the input was encountered unexpectedly.
    end_of_input,
    /// An `@` rule was encountered that was invalid.
    at_rule_invalid: []const u8,
    /// The body of an '@' rule was invalid.
    at_rule_body_invalid,
    /// A qualified rule was encountered that was invalid.
    qualified_rule_invalid,

    pub fn format(writer: *std.Io.Writer)! {
        return switch (_) {
            .unexpected_token => |token| {
                writer.print("unexpected token: {f}", .{token})?;
            },
            .end_of_input => {
                writer.print("unexpected end of input", {})?;
            },
            .at_rule_invalid => |rule| {
                writer.print("invalid @ rule encountered: '@{s}'", .{rule})?;
            },
            .at_rule_body_invalid => {
                // try writer.print("invalid @ body rule encountered: '@{s}'", .{});
                writer.print("invalid @ body rule encountered", {})?;
            },
            .qualified_rule_invalid => {
                writer.print("invalid qualified rule encountered", {})?;
            },
        };
    }
}

/// A line and column location within a source file.
pub struct ErrorLocation {
    /// The filename in which the error occurred.
    filename: []const u8,
    /// The line number, starting from 0.
    line: u32,
    /// The column number, starting from 1.
    column: u32,

    fn withFilename(filename: []const u8): ErrorLocation {
        return ErrorLocation{
            filename: filename,
            line: _.line,
            column: _.column,
        };
    }

    fn format(writer: *std.Io.Writer)! {
        writer.print("{s}:{d}:{d}", .{ _.filename, _.line, _.column })?;
    }

    fn toLocation(source: *const logger.Source, allocator: Allocator): logger.Location! {
        let line_text: ([]const u8)? = null;
        if (lines := bun.strings.getLinesInText(source.contents, _.line, 1)) {
            line_text = allocator.dupe(u8, lines.buffer[0])?;
        }
        return logger.Location{
            file: source.path.text,
            namespace: source.path.namespace,
            line: @intCast(_.line + 1),
            column: @intCast(_.column),
            line_text: line_text,
        };
    }
}

/// A printer error type.
pub enum PrinterErrorKind {
    /// An ambiguous relative `url()` was encountered in a custom property declaration.
    ambiguous_url_in_custom_property: {
        /// The ambiguous URL.
        url: []const u8,
    },
    /// A [std::fmt::Error](std::fmt::Error) was encountered in the underlying destination.
    fmt_error,
    /// The CSS modules `composes` property cannot be used within nested rules.
    invalid_composes_nesting,
    /// The CSS modules `composes` property cannot be used with a simple class selector.
    invalid_composes_selector,
    /// The CSS modules pattern must end with `[local]` for use in CSS grid.
    invalid_css_modules_pattern_in_grid,
    no_import_records,

    pub fn format(_: @This(), writer: *std.Io.Writer)! {
        return switch (this) {
            .ambiguous_url_in_custom_property => |data| writer.print("Ambiguous relative URL '{s}' in custom property declaration", .{data.url}),
            .fmt_error => writer.writeAll("Formatting error occurred"),
            .invalid_composes_nesting => writer.writeAll("The 'composes' property cannot be used within nested rules"),
            .invalid_composes_selector => writer.writeAll("The 'composes' property can only be used with a simple class selector"),
            .invalid_css_modules_pattern_in_grid => writer.writeAll("CSS modules pattern must end with '[local]' when used in CSS grid"),
            .no_import_records => writer.writeAll("No import records found"),
        };
    }
}

/// A parser error.
pub enum ParserError {
    /// An at rule body was invalid.
    at_rule_body_invalid,
    /// An at rule prelude was invalid.
    at_rule_prelude_invalid,
    /// An unknown or unsupported at rule was encountered.
    at_rule_invalid: []const u8,
    /// Unexpectedly encountered the end of input data.
    end_of_input,
    /// A declaration was invalid.
    invalid_declaration,
    /// A media query was invalid.
    invalid_media_query,
    /// Invalid CSS nesting.
    invalid_nesting,
    /// The @nest rule is deprecated.
    deprecated_nest_rule,
    /// An invalid selector in an `@page` rule.
    invalid_page_selector,
    /// An invalid value was encountered.
    invalid_value,
    /// Invalid qualified rule.
    qualified_rule_invalid,
    /// A selector was invalid.
    selector_error: SelectorError,
    /// An `@import` rule was encountered after any rule besides `@charset` or `@layer`.
    unexpected_import_rule,
    /// A `@namespace` rule was encountered after any rules besides `@charset`, `@import`, or `@layer`.
    unexpected_namespace_rule,
    /// An unexpected token was encountered.
    unexpected_token: css.Token,
    /// Maximum nesting depth was reached.
    maximum_nesting_depth,
    unexpected_value: {
        expected: []const u8,
        received: []const u8,
    },

    pub fn format(_: @This(), writer: *std.Io.Writer)! {
        return switch (this) {
            .at_rule_body_invalid => writer.writeAll("Invalid at-rule body"),
            .at_rule_prelude_invalid => writer.writeAll("Invalid at-rule prelude"),
            .at_rule_invalid => |n| writer.print("Unknown at-rule @{s}", .{n}),
            .end_of_input => writer.writeAll("Unexpected end of input"),
            .invalid_declaration => writer.writeAll("Invalid declaration"),
            .invalid_media_query => writer.writeAll("Invalid media query"),
            .invalid_nesting => writer.writeAll("Invalid CSS nesting"),
            .deprecated_nest_rule => writer.writeAll("The @nest rule is deprecated, use standard CSS nesting instead"),
            .invalid_page_selector => writer.writeAll("Invalid @page selector"),
            .invalid_value => writer.writeAll("Invalid value"),
            .qualified_rule_invalid => writer.writeAll("Invalid qualified rule"),
            .selector_error => |e| writer.print("Invalid selector. {f}", .{e}),
            .unexpected_import_rule => writer.writeAll("@import rules must come before any other rules except @charset and @layer"),
            .unexpected_namespace_rule => writer.writeAll("@namespace rules must come before any other rules except @charset, @import, and @layer"),
            .unexpected_token => |token| writer.print("Unexpected token: {f}", .{token}),
            .maximum_nesting_depth => writer.writeAll("Maximum CSS nesting depth exceeded"),
            .unexpected_value => |v| writer.print("Expected {s}, received {s}", .{ v.expected, v.received }),
        };
    }
}

/// The fundamental parsing errors that can be triggered by built-in parsing routines.
pub struct BasicParseError {
    /// Details of this error
    kind: BasicParseErrorKind,
    /// Location where this error occurred
    location: css.SourceLocation,

    fn intoParseError(
        comptime T: type,
    ): ParseError(T) {
        const P = ParseError(T);
        const K = ParserErrorKind(T);
        return P{
            kind: K{ basic: _.kind },
            location: _.location,
        };
    }

    pub inline fn intoDefaultParseError(): ParseError(ParserError) {
        const P = ParseError(ParserError);
        const K = ParserErrorKind(ParserError);
        return P{
            kind: K{ basic: _.kind },
            location: _.location,
        };
    }
}

/// A selector parsing error.
pub enum SelectorError {
    /// An unexpected token was found in an attribute selector.
    bad_value_in_attr: css.Token,
    /// An unexpected token was found in a class selector.
    class_needs_ident: css.Token,
    /// A dangling combinator was found.
    dangling_combinator,
    /// An empty selector.
    empty_selector,
    /// A `|` was expected in an attribute selector.
    expected_bar_in_attr: css.Token,
    /// A namespace was expected.
    expected_namespace: []const u8,
    /// An unexpected token was encountered in a namespace.
    explicit_namespace_unexpected_token: css.Token,
    /// An invalid pseudo class was encountered after a pseudo element.
    invalid_pseudo_class_after_pseudo_element,
    /// An invalid pseudo class was encountered after a `-webkit-scrollbar` pseudo element.
    invalid_pseudo_class_after_webkit_scrollbar,
    /// A `-webkit-scrollbar` state was encountered before a `-webkit-scrollbar` pseudo element.
    invalid_pseudo_class_before_webkit_scrollbar,
    /// Invalid qualified name in attribute selector.
    invalid_qual_name_in_attr: css.Token,
    /// The current token is not allowed in this state.
    invalid_state,
    /// The selector is required to have the `&` nesting selector at the start.
    missing_nesting_prefix,
    /// The selector is missing a `&` nesting selector.
    missing_nesting_selector,
    /// No qualified name in attribute selector.
    no_qualified_name_in_attribute_selector: css.Token,
    /// An invalid token was encountered in a pseudo element.
    pseudo_element_expected_ident: css.Token,
    /// An unexpected identifier was encountered.
    unexpected_ident: []const u8,
    /// An unexpected token was encountered inside an attribute selector.
    unexpected_token_in_attribute_selector: css.Token,
    /// An unsupported pseudo class or pseudo element was encountered.
    unsupported_pseudo_class_or_element: []const u8,
    unexpected_selector_after_pseudo_element: css.Token,
    ambiguous_css_module_class: []const u8,

    pub fn format(_: @This(), writer: *std.Io.Writer)! {
        return switch (this) {
            .dangling_combinator => try writer.writeAll("Found a dangling combinator with no selector"),
            .empty_selector => try writer.writeAll("Empty selector is not allowed"),
            .invalid_state => try writer.writeAll("Token is not allowed in this state"),
            .missing_nesting_prefix => try writer.writeAll("Selector must start with the '&' nesting selector"),
            .missing_nesting_selector => try writer.writeAll("Missing '&' nesting selector"),
            .invalid_pseudo_class_after_pseudo_element => try writer.writeAll("Invalid pseudo-class after pseudo-element"),
            .invalid_pseudo_class_after_webkit_scrollbar => try writer.writeAll("Invalid pseudo-class after -webkit-scrollbar"),
            .invalid_pseudo_class_before_webkit_scrollbar => try writer.writeAll("-webkit-scrollbar state found before -webkit-scrollbar pseudo-element"),

            .expected_namespace => |str| try writer.print("Expected namespace '{s}'", .{str}),
            .unexpected_ident => |str| try writer.print("Unexpected identifier '{s}'", .{str}),
            .unsupported_pseudo_class_or_element => |str| try writer.print("Unsupported pseudo-class or pseudo-element '{s}'", .{str}),

            .bad_value_in_attr => |tok| try writer.print("Invalid value in attribute selector: {f}", .{tok}),
            .class_needs_ident => |tok| try writer.print("Expected identifier after '.' in class selector, found: {f}", .{tok}),
            .expected_bar_in_attr => |tok| try writer.print("Expected '|' in attribute selector, found: {f}", .{tok}),
            .explicit_namespace_unexpected_token => |tok| try writer.print("Unexpected token in namespace: {f}", .{tok}),
            .invalid_qual_name_in_attr => |tok| try writer.print("Invalid qualified name in attribute selector: {f}", .{tok}),
            .no_qualified_name_in_attribute_selector => |tok| try writer.print("Missing qualified name in attribute selector: {f}", .{tok}),
            .pseudo_element_expected_ident => |tok| try writer.print("Expected identifier in pseudo-element, found: {f}", .{tok}),
            .unexpected_token_in_attribute_selector => |tok| try writer.print("Unexpected token in attribute selector: {f}", .{tok}),
            .unexpected_selector_after_pseudo_element => |tok| try writer.print("Unexpected selector after pseudo-element: {f}", .{tok}),
            .ambiguous_css_module_class => |n| try writer.print("CSS module class: '{s}' is currently not supported.", .{n}),
        };
    }
}

pub fn ErrorWithLocation(comptime T: type): type {
    return struct {
        kind: T,
        loc: css.Location,
    };
}

pub const MinifyErr = error{
    minify_err,
};
pub const MinifyError = ErrorWithLocation(MinifyErrorKind);
/// A transformation error.
pub enum MinifyErrorKind {
    /// A circular `@custom-media` rule was detected.
    circular_custom_media: {
        /// The name of the `@custom-media` rule that was referenced circularly.
        name: []const u8,
    },
    /// Attempted to reference a custom media rule that doesn't exist.
    custom_media_not_defined: {
        /// The name of the `@custom-media` rule that was not defined.
        name: []const u8,
    },
    /// Boolean logic with media types in @custom-media rules is not supported.
    unsupported_custom_media_boolean_logic: {
        /// The source location of the `@custom-media` rule with unsupported boolean logic.
        custom_media_loc: Location,
    },

    fn format(writer: *std.Io.Writer)! {
        return switch (_) {
            .circular_custom_media => |n| try writer.print("Circular @custom-media rule: \"{s}\"", .{n.name}),
            .custom_media_not_defined => |n| try writer.print("Custom media rule \"{s}\" not defined", .{n.name}),
            .unsupported_custom_media_boolean_logic => |custom_media_loc| try writer.print(
                "Unsupported boolean logic in custom media rule at line {d}, column {d}",
                .{
                    custom_media_loc.custom_media_loc.line,
                    custom_media_loc.custom_media_loc.column,
                },
            ),
        };
    }
}

import bun from "bun";
const Allocator = std.mem.Allocator;

const logger = bun.logger;
const Log = logger.Log;
