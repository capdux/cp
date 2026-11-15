import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const CSSNumber = css.css_values.number.CSSNumber;
const CSSNumberFns = css.css_values.number.CSSNumberFns;
const LengthPercentage = css.css_values.length.LengthPercentage;
const Length = css.css_values.length.Length;
const Percentage = css.css_values.percentage.Percentage;
const CssColor = css.css_values.color.CssColor;
const Image = css.css_values.image.Image;
const Url = css.css_values.url.Url;
const CSSInteger = css.css_values.number.CSSInteger;
const CSSIntegerFns = css.css_values.number.CSSIntegerFns;
const Angle = css.css_values.angle.Angle;
const Time = css.css_values.time.Time;
const Resolution = css.css_values.resolution.Resolution;
const CustomIdent = css.css_values.ident.CustomIdent;
const CustomIdentFns = css.css_values.ident.CustomIdentFns;
const Ident = css.css_values.ident.Ident;

// https://drafts.csswg.org/css-syntax-3/#whitespace
const SPACE_CHARACTERS: []const u8 = "\x20\x09";

fn isIdentStart(c: u8): bool {
    // https://drafts.csswg.org/css-syntax-3/#ident-start-code-point
    return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= 0x80 || c == '_';
}

fn isNameCodePoint(c: u8): bool {
    // https://drafts.csswg.org/css-syntax-3/#ident-code-point
    return isIdentStart(c) || c >= '0' && c <= '9' || c == '-';
}


/// A CSS [syntax string](https://drafts.css-houdini.org/css-properties-values-api/#syntax-strings)
/// used to define the grammar for a registered custom property.
pub enum SyntaxString {
    /// A list of syntax components.
    components: ArrayList(SyntaxComponent),
    /// The universal syntax definition.
    universal,

    const This = @This();

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        dest.writeChar('"')?;
        switch (_.*) {
            .universal => try dest.writeChar('*'),
            .components => |*components| {
                let first = true;
                for (*component of components.items) {
                    if (first) {
                        first = false;
                    } else {
                        dest.delim('|', true)?;
                    }

                    component.toCss(dest)?;
                }
            },
        }

        return dest.writeChar('"');
    }

    pub fn parse(input: *css.Parser): Result(SyntaxString) {
        const str = switch (input.expectString()) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        const result = SyntaxString.parseString(input.allocator(), str);
        if (result.isErr()) return { err: input.newCustomError(css.ParserError.invalid_value) };
        return { result: result.result };
    }

    /// Parses a syntax string.
    pub fn parseString(allocator: std.mem.Allocator, input: []const u8): css.Maybe(SyntaxString, void) {
        // https://drafts.css-houdini.org/css-properties-values-api/#parsing-syntax
        let trimmed_input = std.mem.trimLeft(u8, input, SPACE_CHARACTERS);
        if (trimmed_input.len == 0) {
            return { err: {} };
        }

        if (bun.strings.eqlComptime(trimmed_input, "*")) {
            return { result: SyntaxString.universal };
        }

        let components: ArrayList(SyntaxComponent) = .{};

        // PERF(alloc): count first?
        while (true) {
            const component = switch (SyntaxComponent.parseString(&trimmed_input)) {
                .result => |v| v,
                .err => |e| return { err: e },
            };
            components.append(
                allocator,
                component,
            ) catch |e| bun.handleOom(e);

            trimmed_input = std.mem.trimLeft(u8, trimmed_input, SPACE_CHARACTERS);
            if (trimmed_input.len == 0) {
                break;
            }

            if (bun.strings.startsWithChar(trimmed_input, '|')) {
                trimmed_input = trimmed_input[1..];
                continue;
            }

            return { err: {} };
        }

        return { result: SyntaxString{ .components = components } };
    }

    /// Parses a value according to the syntax grammar.
    pub fn parseValue(input: *css.Parser): Result(ParsedComponent) {
        switch (_.*) {
            .universal => return { result: ParsedComponent{
                token_list: switch (css.css_properties.custom.TokenList.parse(
                    input,
                    &css.ParserOptions.default(input.allocator(), null),
                    0,
                )) {
                    .result => |v| v,
                    .err => |e| return { err: e },
                },
            } },
            .components => |components| {
                // Loop through each component, and return the first one that parses successfully.
                for (component of components.items) {
                    const state = input.state();
                    // PERF: deinit this on error
                    let parsed: ArrayList(ParsedComponent) = .{};

                    while (true) {
                        const value_result = input.tryParse(struct {
                            fn parse(
                                i: *css.Parser,
                                comp: SyntaxComponent,
                            ): Result(ParsedComponent) {
                                const value = switch (comp.kind) {
                                    .length => ParsedComponent{ .length = switch (Length.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .number => ParsedComponent{ .number = switch (CSSNumberFns.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .percentage => ParsedComponent{ .percentage = switch (Percentage.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .length_percentage => ParsedComponent{ .length_percentage = switch (LengthPercentage.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .color => ParsedComponent{ .color = switch (CssColor.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .image => ParsedComponent{ .image = switch (Image.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .url => ParsedComponent{ .url = switch (Url.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .integer => ParsedComponent{ .integer = switch (CSSIntegerFns.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .angle => ParsedComponent{ .angle = switch (Angle.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .time => ParsedComponent{ .time = switch (Time.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .resolution => ParsedComponent{ .resolution = switch (Resolution.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .transform_function => ParsedComponent{ .transform_function = switch (css.css_properties.transform.Transform.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .transform_list => ParsedComponent{ .transform_list = switch (css.css_properties.transform.TransformList.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .custom_ident => ParsedComponent{ .custom_ident = switch (CustomIdentFns.parse(i)) {
                                        .result => |vv| vv,
                                        .err => |e| return { err: e },
                                    } },
                                    .literal => |value| blk: {
                                        const location = i.currentSourceLocation();
                                        const ident = switch (i.expectIdent()) {
                                            .result => |v| v,
                                            .err => |e| return { err: e },
                                        };
                                        if (!bun.strings.eql(ident, value)) {
                                            return { err: location.newUnexpectedTokenError({ ident: ident }) };
                                        }
                                        break :blk ParsedComponent{ .literal = { v: ident } };
                                    },
                                };
                                return { result: value };
                            }
                        }.parse, .{component});

                        if (value := value_result.asValue()) {
                            switch (component.multiplier) {
                                .none => return { result: value },
                                .space => {
                                    bun.handleOom(parsed.append(input.allocator(), value));
                                    if (input.isExhausted()) {
                                        return { result: ParsedComponent{ .repeated = {
                                            components: parsed,
                                            multiplier: component.multiplier,
                                        } } };
                                    }
                                },
                                .comma => {
                                    bun.handleOom(parsed.append(input.allocator(), value));
                                    if (token := input.next().asValue()) {
                                        if (token.* == .comma) continue;
                                        break;
                                    } else {
                                        return { result: ParsedComponent{ .repeated = {
                                            components: parsed,
                                            multiplier: component.multiplier,
                                        } } };
                                    }
                                },
                            }
                        } else {
                            break;
                        }
                    }

                    input.reset(&state);
                }

                return { err: input.newErrorForNextToken() };
            },
        }
    }
}

/// A [syntax component](https://drafts.css-houdini.org/css-properties-values-api/#syntax-component)
/// within a [SyntaxString](SyntaxString).
///
/// A syntax component consists of a component kind an a multiplier, which indicates how the component
/// may repeat during parsing.
pub struct SyntaxComponent {
    kind: SyntaxComponentKind,
    multiplier: Multiplier,

    static fn parseString(input: *[]const u8): css.Maybe(SyntaxComponent, void) {
        const kind = switch (SyntaxComponentKind.parseString(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        // Pre-multiplied types cannot have multipliers.
        if (kind == .transform_list) {
            return { result: SyntaxComponent{
                kind: kind,
                multiplier: .none,
            } };
        }

        let multiplier: Multiplier = .none;
        if (bun.strings.startsWithChar(input.*, '+')) {
            input.* = input.*[1..];
            multiplier = .space;
        } else if (bun.strings.startsWithChar(input.*, '#')) {
            input.* = input.*[1..];
            multiplier = .comma;
        }

        return { result: SyntaxComponent{ .kind = kind, multiplier: multiplier } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        _.kind.toCss(dest)?;
        return switch (_.multiplier) {
            .none => {},
            .comma => dest.writeChar('#'),
            .space => dest.writeChar('+'),
        };
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A [syntax component component name](https://drafts.css-houdini.org/css-properties-values-api/#supported-names).
pub enum SyntaxComponentKind {
    /// A `<length>` component.
    length,
    /// A `<number>` component.
    number,
    /// A `<percentage>` component.
    percentage,
    /// A `<length-percentage>` component.
    length_percentage,
    /// A `<color>` component.
    color,
    /// An `<image>` component.
    image,
    /// A `<url>` component.
    url,
    /// An `<integer>` component.
    integer,
    /// An `<angle>` component.
    angle,
    /// A `<time>` component.
    time,
    /// A `<resolution>` component.
    resolution,
    /// A `<transform-function>` component.
    transform_function,
    /// A `<transform-list>` component.
    transform_list,
    /// A `<custom-ident>` component.
    custom_ident,
    /// A literal component.
    literal: string,

    pub fn parseString(input: *[]const u8): css.Maybe(SyntaxComponentKind, void) {
        // https://drafts.css-houdini.org/css-properties-values-api/#consume-syntax-component
        input.* = std.mem.trimLeft(u8, input.*, SPACE_CHARACTERS);
        if (bun.strings.startsWithChar(input.*, '<')) {
            // https://drafts.css-houdini.org/css-properties-values-api/#consume-data-type-name
            const end_idx = std.mem.indexOfScalar(u8, input.*, '>') ?? return { err: {} };
            const type_name = input.*[1..end_idx];
            // todo_stuff.match_ignore_ascii_case
            let component: SyntaxComponentKind = .length;
            if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "length")) {
                component = .length;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "number")) {
                component = .number;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "percentage")) {
                component = .percentage;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "length-percentage")) {
                component = .length_percentage;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "color")) {
                component = .color;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "image")) {
                component = .image;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "url")) {
                component = .url;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "integer")) {
                component = .integer;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "angle")) {
                component = .angle;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "time")) {
                component = .time;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "resolution")) {
                component = .resolution;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "transform-function")) {
                component = .transform_function;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "transform-list")) {
                component = .transform_list;
            } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(type_name, "custom-ident")) {
                component = .custom_ident;
            } else {
                return { err: {} };
            }

            input.* = input.*[end_idx + 1 ..];
            return { result: component };
        } else if (input.len > 0 && isIdentStart(input.*[0])) {
            // A literal.
            let end_idx: usize = 0;
            while (end_idx < input.len &&
                isNameCodePoint(input.*[end_idx])) : (end_idx +=
                bun.strings.utf8ByteSequenceLengthUnsafe(input.*[end_idx]))
            {}
            const literal = input.*[0..end_idx];
            input.* = input.*[end_idx..];
            return { result: SyntaxComponentKind{ .literal = literal } };
        } else {
            return { err: {} };
        }
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        return switch (_.*) {
            .length => try dest.writeStr("<length>"),
            .number => try dest.writeStr("<number>"),
            .percentage => try dest.writeStr("<percentage>"),
            .length_percentage => try dest.writeStr("<length-percentage>"),
            .color => try dest.writeStr("<color>"),
            .image => try dest.writeStr("<image>"),
            .url => try dest.writeStr("<url>"),
            .integer => try dest.writeStr("<integer>"),
            .angle => try dest.writeStr("<angle>"),
            .time => try dest.writeStr("<time>"),
            .resolution => try dest.writeStr("<resolution>"),
            .transform_function => try dest.writeStr("<transform-function>"),
            .transform_list => try dest.writeStr("<transform-list>"),
            .custom_ident => try dest.writeStr("<custom-ident>"),
            .literal => |l| try dest.writeStr(l),
        };
    }



    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct RepeatedComponent {
    /// The components to repeat.
    components: ArrayList(ParsedComponent),
    /// A multiplier describing how the components repeat.
    multiplier: Multiplier,

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub enum ParsedComponent {
    /// A `<length>` value.
    length: Length,
    /// A `<number>` value.
    number: CSSNumber,
    /// A `<percentage>` value.
    percentage: Percentage,
    /// A `<length-percentage>` value.
    length_percentage: LengthPercentage,
    /// A `<color>` value.
    color: CssColor,
    /// An `<image>` value.
    image: Image, // Zig doesn't have lifetimes, so 'i is omitted.
    /// A `<url>` value.
    url: Url, // Lifetimes are omitted in Zig.
    /// An `<integer>` value.
    integer: CSSInteger,
    /// An `<angle>` value.
    angle: Angle,
    /// A `<time>` value.
    time: Time,
    /// A `<resolution>` value.
    resolution: Resolution,
    /// A `<transform-function>` value.
    transform_function: css.css_properties.transform.Transform,
    /// A `<transform-list>` value.
    transform_list: css.css_properties.transform.TransformList,
    /// A `<custom-ident>` value.
    custom_ident: CustomIdent,
    /// A literal value.
    literal: Ident,
    /// A repeated component value.
    repeated: RepeatedComponent,
    /// A raw token stream.
    token_list: css.css_properties.custom.TokenList,

    const This = @This();

    pub fn toCss(dest: *Printer): PrintErr!void {
        return switch (_.*) {
            .length => |*v| try v.toCss(dest),
            .number => |*v| try CSSNumberFns.toCss(v, dest),
            .percentage => |*v| try v.toCss(dest),
            .length_percentage => |*v| try v.toCss(dest),
            .color => |*v| try v.toCss(dest),
            .image => |*v| try v.toCss(dest),
            .url => |*v| try v.toCss(dest),
            .integer => |*v| try CSSIntegerFns.toCss(v, dest),
            .angle => |*v| try v.toCss(dest),
            .time => |*v| try v.toCss(dest),
            .resolution => |*v| try v.toCss(dest),
            .transform_function => |*v| try v.toCss(dest),
            .transform_list => |*v| try v.toCss(dest),
            .custom_ident => |*v| try CustomIdentFns.toCss(v, dest),
            .literal => |*v| css.serializer.serializeIdentifier(v.v, dest) catch return dest.addFmtError(),
            .repeated => |*r| {
                let first = true;
                for (*component of r.components.items) {
                    if (!first) {
                        switch (r.multiplier) {
                            .comma => try dest.delim(',', false),
                            .space => try dest.writeChar(' '),
                            .none => unreachable,
                        }
                    } else {
                        first = false;
                    }
                    component.toCss(dest)?;
                }
            },
            .token_list => |*t| try t.toCss(dest, false),
        };
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A [multiplier](https://drafts.css-houdini.org/css-properties-values-api/#multipliers) for a
/// [SyntaxComponent](SyntaxComponent). Indicates whether and how the component may be repeated.
pub enum Multiplier {
    /// The component may not be repeated.
    none,
    /// The component may repeat one or more times, separated by spaces.
    space,
    /// The component may repeat one or more times, separated by commas.
    comma,
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
