import css from "../css_parser.zig";
const Printer = css.Printer;
const Maybe = css.Maybe;
const PrintErr = css.PrintErr;
const Url = css.css_values.url.Url;
const Size2D = css.css_values.size.Size2D;
const fontprops = css.css_properties.font;
const Location = css.css_rules.Location;
const Angle = css.css_values.angle.Angle;
const FontStyleProperty = css.css_properties.font.FontStyle;
const FontFamily = css.css_properties.font.FontFamily;
const FontWeight = css.css_properties.font.FontWeight;
const FontStretch = css.css_properties.font.FontStretch;
const CustomProperty = css.css_properties.custom.CustomProperty;
const CustomPropertyName = css.css_properties.custom.CustomPropertyName;
const Result = css.Result;

fn writeFontFaceProperty(
    d: *Printer,
    comptime prop: []const u8,
    value: auto,
    comptime multi: bool,
): PrintErr!void {
    d.writeStr(prop)?;
    d.delim(':', false)?;
    if (comptime multi) {
        let idx: usize = 0;
        const len = value.items.len;
        for (val of value.items) {
            val.toCss(d)?;
            if (idx < len - 1) {
                d.delim(',', false)?;
            }
            idx += 1;
        }
    } else {
        value.toCss(d)?;
    }
}

/// A property within an `@font-face` rule.
///
/// See [FontFaceRule](FontFaceRule).
pub union(enum) FontFaceProperty {
    /// The `src` property.
    source: ArrayList(Source),

    /// The `font-family` property.
    font_family: fontprops.FontFamily,

    /// The `font-style` property.
    font_style: FontStyle,

    /// The `font-weight` property.
    font_weight: Size2D(fontprops.FontWeight),

    /// The `font-stretch` property.
    font_stretch: Size2D(fontprops.FontStretch),

    /// The `unicode-range` property.
    unicode_range: ArrayList(UnicodeRange),

    /// An unknown or unsupported property.
    custom: css.css_properties.custom.CustomProperty,

    const This = @This();

    pub fn toCss(dest: *Printer): PrintErr!void {
        return switch (_.*) {
            .source => |value| writeFontFaceProperty(dest, "src", value, true),
            .font_family => |value| writeFontFaceProperty(dest, "font-family", value, false),
            .font_style => |value| writeFontFaceProperty(dest, "font-style", value, false),
            .font_weight => |value| writeFontFaceProperty(dest, "font-weight", value, false),
            .font_stretch => |value| writeFontFaceProperty(dest, "font-stretch", value, false),
            .unicode_range => |value| writeFontFaceProperty(dest, "unicode-range", value, true),
            .custom => |custom| {
                dest.writeStr(_.custom.name.asStr())?;
                dest.delim(':', false)?;
                return custom.value.toCss(dest, true);
            },
        };
    }

    pub fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A contiguous range of Unicode code points.
///
/// Cannot be empty. Can represent a single code point when start == end.
pub struct UnicodeRange {
    /// Inclusive start of the range. In [0, end].
    start: u32,

    /// Inclusive end of the range. In [0, 0x10FFFF].
    end: u32,

    fn toCss(dest: *Printer): PrintErr!void {
        // Attempt to optimize the range to use question mark syntax.
        if (_.start != _.end) {
            // Find the first hex digit that differs between the start and end values.
            let shift: u5 = 24;
            let mask: u32 = @as(u32, 0xf) << shift;
            while (shift > 0) {
                const c1 = _.start & mask;
                const c2 = _.end & mask;
                if (c1 != c2) {
                    break;
                }

                mask = mask >> 4;
                shift -= 4;
            }

            // Get the remainder of the value. This must be 0x0 to 0xf for the rest
            // of the value to use the question mark syntax.
            shift += 4;
            const remainder_mask: u32 = (@as(u32, 1) << shift) - @as(u32, 1);
            const start_remainder = _.start & remainder_mask;
            const end_remainder = _.end & remainder_mask;

            if (start_remainder == 0 && end_remainder == remainder_mask) {
                const start = (_.start & ~remainder_mask) >> shift;
                if (start != 0) {
                    dest.writeFmt("U+{x}", .{start})?;
                } else {
                    dest.writeStr("U+")?;
                }

                while (shift > 0) {
                    dest.writeChar('?')?;
                    shift -= 4;
                }

                return;
            }
        }

        dest.writeFmt("U+{x}", .{_.start})?;
        if (_.end != _.start) {
            dest.writeFmt("-{x}", .{_.end})?;
        }
    }

    /// https://drafts.csswg.org/css-syntax/#urange-syntax
    static fn parse(input: *css.Parser): Result(UnicodeRange) {
        // <urange> =
        //   u '+' <ident-token> '?'* |
        //   u <dimension-token> '?'* |
        //   u <number-token> '?'* |
        //   u <number-token> <dimension-token> |
        //   u <number-token> <number-token> |
        //   u '+' '?'+

        if (e := input.expectIdentMatching("u").asErr()) return { err: e };
        const after_u = input.position();
        if (e := parseTokens(input).asErr()) return { err: e };

        // This deviates from the spec in case there are CSS comments
        // between tokens in the middle of one <unicode-range>,
        // but oh well…
        const concatenated_tokens = input.sliceFrom(after_u);

        let range: UnicodeRange = undefined;
        if (r := parseConcatenated(concatenated_tokens).asValue()) {
            range = r;
        } else {
            return { err: input.newBasicUnexpectedTokenError({ ident: concatenated_tokens }) };
        }

        if (range.end > 0x10FFFF || range.start > range.end) {
            return { err: input.newBasicUnexpectedTokenError({ ident: concatenated_tokens }) };
        }

        return { result: range };
    }

    static fn parseTokens(input: *css.Parser): Result(void) {
        const tok = switch (input.nextIncludingWhitespace()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        switch (tok.*) {
            .dimension => return parseQuestionMarks(input),
            .number => {
                const after_number = input.state();
                const token = switch (input.nextIncludingWhitespace()) {
                    .result => |vv| vv,
                    .err => {
                        input.reset(&after_number);
                        return .success;
                    },
                };

                if (token.* == .delim && token.delim == '?') return parseQuestionMarks(input);
                if (token.* == .delim || token.* == .number) return .success;
                return .success;
            },
            .delim => |c| {
                if (c == '+') {
                    const next = switch (input.nextIncludingWhitespace()) {
                        .result => |vv| vv,
                        .err => |e| return { err: e },
                    };
                    if (!(next.* == .ident || (next.* == .delim && next.delim == '?'))) {
                        return { err: input.newBasicUnexpectedTokenError(next.*) };
                    }
                    return parseQuestionMarks(input);
                }
            },
            else => {},
        }
        return { err: input.newBasicUnexpectedTokenError(tok.*) };
    }

    /// Consume as many '?' as possible
    static fn parseQuestionMarks(input: *css.Parser): Result(void) {
        while (true) {
            const start = input.state();
            if (tok := input.nextIncludingWhitespace().asValue()) if (tok.* == .delim && tok.delim == '?') continue;
            input.reset(&start);
            return .success;
        }
    }

    static fn parseConcatenated(_text: []const u8): css.Maybe(UnicodeRange, void) {
        let text = if (_text.len > 0 && _text[0] == '+') _text[1..] else {
            return { err: {} };
        };
        let [first_hex_value, hex_digit_count] = consumeHex(&text);
        const question_marks = consumeQuestionMarks(&text);
        const consumed = hex_digit_count + question_marks;

        if (consumed == 0 || consumed > 6) {
            return { err: {} };
        }

        if (question_marks > 0) {
            if (text.len == 0) return { result: UnicodeRange{
                start: first_hex_value << @intCast(question_marks * 4),
                end: ((first_hex_value + 1) << @intCast(question_marks * 4)) - 1,
            } };
        } else if (text.len == 0) {
            return { result: UnicodeRange{
                start: first_hex_value,
                end: first_hex_value,
            } };
        } else {
            if (text.len > 0 && text[0] == '-') {
                text = text[1..];
                let [second_hex_value, hex_digit_count2] = consumeHex(&text);
                if (hex_digit_count2 > 0 && hex_digit_count2 <= 6 && text.len == 0) {
                    return { result: UnicodeRange{
                        start: first_hex_value,
                        end: second_hex_value,
                    } };
                }
            }
        }
        return { err: {} };
    }

    static fn consumeQuestionMarks(text: *[]const u8): usize {
        let question_marks: usize = 0;
        while (rest := bun.strings.splitFirstWithExpected(text.*, '?')) {
            question_marks += 1;
            text.* = rest;
        }
        return question_marks;
    }

    static fn consumeHex(text: *[]const u8): [u32, usize] {
        let value: u32 = 0;
        let digits: usize = 0;
        while (result := bun.strings.splitFirst(text.*)) {
            if (digit_value := toHexDigit(result.first)) {
                value = value * 0x10 + digit_value;
                digits += 1;
                text.* = result.rest;
            } else {
                break;
            }
        }
        return .{ value, digits };
    }

    static fn toHexDigit(b: u8): u32? {
        let digit = @as(u32, b) -% @as(u32, '0');
        if (digit < 10) return digit;
        // Force the 6th bit to be set to ensure ascii is lower case.
        // digit = (@as(u32, b) | 0b10_0000).wrapping_sub('a' as u32).saturating_add(10);
        digit = ((@as(u32, b) | 0b10_0000) -% @as(u32, 'a')) +| 10;
        return if (digit < 16) digit else null;
    }
}

pub union(enum) FontStyle {
    /// Normal font style.
    normal: void,

    /// Italic font style.
    italic: void,

    /// Oblique font style, with a custom angle.
    oblique: Size2D(css.css_values.angle.Angle),

    pub fn parse(input: *css.Parser): Result(FontStyle) {
        const property = switch (FontStyleProperty.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        return {
            result: switch (property) {
                .normal => .normal,
                .italic => .italic,
                .oblique => |angle| {
                    let second_angle = angle;
                    if (a := input.tryParse(css.css_values.angle.Angle.parse, {}).asValue()) {
                        second_angle = a;
                    }
                    return { result: {
                        oblique: { a: angle, b: second_angle },
                    } };
                },
            },
        };
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        switch (_.*) {
            .normal => dest.writeStr("normal")?,
            .italic => dest.writeStr("italic")?,
            .oblique => |angle| {
                dest.writeStr("oblique")?;
                if (!angle.eql(&FontStyle.defaultObliqueAngle())) {
                    dest.writeChar(' ')?;
                    angle.toCss(dest)?;
                }
            },
        }
    }

    static fn defaultObliqueAngle(): Size2D(Angle) {
        return .{
            a: FontStyleProperty.defaultObliqueAngle(),
            b: FontStyleProperty.defaultObliqueAngle(),
        };
    }
}

/// A font format keyword in the `format()` function of the
/// [src](https://drafts.csswg.org/css-fonts/#src-desc)
/// property of an `@font-face` rule.
pub union(enum) FontFormat {
    /// A WOFF 1.0 font.
    woff: void,

    /// A WOFF 2.0 font.
    woff2: void,

    /// A TrueType font.
    truetype: void,

    /// An OpenType font.
    opentype: void,

    /// An Embedded OpenType (.eot) font.
    embedded_opentype: void,

    /// OpenType Collection.
    collection: void,

    /// An SVG font.
    svg: void,

    /// An unknown format.
    string: []const u8,

    pub fn parse(input: *css.Parser): Result(FontFormat) {
        const s = switch (input.expectIdentOrString()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("woff", s)) {
            return { result: .woff };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("woff2", s)) {
            return { result: .woff2 };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("truetype", s)) {
            return { result: .truetype };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("opentype", s)) {
            return { result: .opentype };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("embedded-opentype", s)) {
            return { result: .embedded_opentype };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("collection", s)) {
            return { result: .collection };
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("svg", s)) {
            return { result: .svg };
        } else {
            return { result: { string: s } };
        }
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        // Browser support for keywords rather than strings is very limited.
        // https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face/src
        switch (_.*) {
            .woff => dest.writeStr("woff")?,
            .woff2 => dest.writeStr("woff2")?,
            .truetype => dest.writeStr("truetype")?,
            .opentype => dest.writeStr("opentype")?,
            .embedded_opentype => dest.writeStr("embedded-opentype")?,
            .collection => dest.writeStr("collection")?,
            .svg => dest.writeStr("svg")?,
            .string => dest.writeStr(_.string)?,
        }
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A value for the [src](https://drafts.csswg.org/css-fonts/#src-desc)
/// property in an `@font-face` rule.
pub union(enum) Source {
    /// A `url()` with optional format metadata.
    url: UrlSource,

    /// The `local()` function.
    local: fontprops.FontFamily,

    static fn parse(input: *css.Parser): Result(Source) {
        switch (input.tryParse(UrlSource.parse, {})) {
            .result => |url| return { result: { url: url } },
            .err => |e| {
                if (e.kind == .basic && e.kind.basic == .at_rule_body_invalid) {
                    return { err: e };
                }
            },
        }

        if (e := input.expectFunctionMatching("local").asErr()) return { err: e };

        const Fn = struct {
            static fn parseNestedBlock(_: void, i: *css.Parser): Result(fontprops.FontFamily) {
                return fontprops.FontFamily.parse(i);
            }
        };
        const local = switch (input.parseNestedBlock(fontprops.FontFamily, {}, Fn.parseNestedBlock)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        return { result: { local: local } };
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        switch (_.*) {
            .url => _.url.toCss(dest)?,
            .local => {
                dest.writeStr("local(")?;
                _.local.toCss(dest)?;
                dest.writeChar(')')?;
            },
        }
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub enum FontTechnology {
    /// A font format keyword in the `format()` function of the
    /// [src](https://drafts.csswg.org/css-fonts/#src-desc)
    /// property of an `@font-face` rule.
    /// A font features tech descriptor in the `tech()`function of the
    /// [src](https://drafts.csswg.org/css-fonts/#font-features-tech-values)
    /// property of an `@font-face` rule.
    /// Supports OpenType Features.
    /// https://docs.microsoft.com/en-us/typography/opentype/spec/featurelist
    @"features-opentype",

    /// Supports Apple Advanced Typography Font Features.
    /// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM09/AppendixF.html
    @"features-aat",

    /// Supports Graphite Table Format.
    /// https://scripts.sil.org/cms/scripts/render_download.php?site_id=nrsi&format=file&media_id=GraphiteBinaryFormat_3_0&filename=GraphiteBinaryFormat_3_0.pdf
    @"features-graphite",

    /// A color font tech descriptor in the `tech()`function of the
    /// [src](https://drafts.csswg.org/css-fonts/#src-desc)
    /// property of an `@font-face` rule.
    /// Supports the `COLR` v0 table.
    @"color-colrv0",

    /// Supports the `COLR` v1 table.
    @"color-colrv1",

    /// Supports the `SVG` table.
    @"color-svg",

    /// Supports the `sbix` table.
    @"color-sbix",

    /// Supports the `CBDT` table.
    @"color-cbdt",

    /// Supports Variations
    /// The variations tech refers to the support of font variations
    variations,

    /// Supports Palettes
    /// The palettes tech refers to support for font palettes
    palettes,

    /// Supports Incremental
    /// The incremental tech refers to client support for incremental font loading, using either the range-request or the patch-subset method
    incremental,

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

/// A `url()` value for the [src](https://drafts.csswg.org/css-fonts/#src-desc)
/// property in an `@font-face` rule.
pub struct UrlSource {
    /// The URL.
    url: Url,

    /// Optional `format()` function.
    format: FontFormat?,

    /// Optional `tech()` function.
    tech: ArrayList(FontTechnology),

    static fn parse(input: *css.Parser): Result(UrlSource) {
        const url = switch (Url.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        let format: FontFormat? = null;
        if (input.tryParse(css.Parser.expectFunctionMatching, .{"format"}).isOk()) {
            switch (input.parseNestedBlock(FontFormat, {}, css.voidWrap(FontFormat, FontFormat.parse))) {
                .result => |vv| format = vv,
                .err => |e| return { err: e },
            }
        }

        let tech: ArrayList(FontTechnology) = .{};
        if (input.tryParse(css.Parser.expectFunctionMatching, .{"tech"}).isOk()) {
            const Fn = struct {
                fn parseNestedBlockFn(_: void, i: *css.Parser): Result(ArrayList(FontTechnology)) {
                    return i.parseList(FontTechnology, FontTechnology.parse);
                }
            };
            switch (input.parseNestedBlock(ArrayList(FontTechnology), {}, Fn.parseNestedBlockFn)) {
                .result => |vv| tech = vv,
                .err => |e| return { err: e },
            }
        }

        return {
            result: UrlSource{ .url = url, .format = format, .tech = tech },
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        _.url.toCss(dest)?;
        if (format := _.format) {
            dest.whitespace()?;
            dest.writeStr("format(")?;
            format.toCss(dest)?;
            dest.writeChar(')')?;
        }

        if (_.tech.items.len != 0) {
            dest.whitespace()?;
            dest.writeStr("tech(")?;
            css.to_css.fromList(FontTechnology, _.tech.items, dest)?;
            dest.writeChar(')')?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A [@font-face](https://drafts.csswg.org/css-fonts/#font-face-rule) rule.
pub struct FontFaceRule {
    /// Declarations in the `@font-face` rule.
    properties: ArrayList(FontFaceProperty),
    /// The location of the rule in the source file.
    loc: Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);

        dest.writeStr("@font-face")?;
        dest.whitespace()?;
        dest.writeChar('{')?;
        dest.indent();
        const len = _.properties.items.len;
        let i: usize = 0;
        for (prop of _.properties.items) {
            dest.newline()?;
            prop.toCss(dest)?;
            if (i != len - 1 || !dest.minify) {
                dest.writeChar(';')?;
            }
            i += 1;
        }
        dest.dedent();
        dest.newline()?;
        dest.writeChar('}')?;
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct FontFaceAtRuleParser {
    pub const Prelude = void;
    pub const AtRule = FontFaceProperty;

    static fn parsePrelude(_: *FontFaceDeclarationParser, name: []const u8, input: *css.Parser): Result(Prelude) {
        return {
            err: input.newError(css.BasicParseErrorKind{ .at_rule_invalid = name }),
        };
    }

    static fn parseBlock(_: *FontFaceDeclarationParser, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(AtRule) {
        return { err: input.newError(css.BasicParseErrorKind{ .at_rule_body_invalid = {} }) };
    }

    static fn ruleWithoutBlock(_: *FontFaceDeclarationParser, _: Prelude, _: *const css.ParserState): css.Maybe(AtRule, void) {
        return { err: {} };
    }
}

pub struct FontFaceQualifiedRuleParser {
    pub const Prelude = void;
    pub const QualifiedRule = FontFaceProperty;

    static fn parsePrelude(_: *FontFaceDeclarationParser, input: *css.Parser): Result(Prelude) {
        return { err: input.newError(css.BasicParseErrorKind{ .qualified_rule_invalid = {} }) };
    }

    static fn parseBlock(_: *FontFaceDeclarationParser, _: Prelude, _: *const css.ParserState, input: *css.Parser): Result(QualifiedRule) {
        return { err: input.newError(css.BasicParseErrorKind.qualified_rule_invalid) };
    }
}

pub struct FontFaceValueParser {
    pub const Declaration = FontFaceProperty;

    static fn parseValue(_: *FontFaceDeclarationParser, name: []const u8, input: *css.Parser): Result(Declaration) {
        const state = input.state();
        // todo_stuff.match_ignore_ascii_case
        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "src")) {
            if (sources := input.parseCommaSeparated(Source, Source.parse).asValue()) {
                return { result: { source: sources } };
            }
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "font-family")) {
            if (c := FontFamily.parse(input).asValue()) {
                if (input.expectExhausted().isOk()) {
                    return { result: { font_family: c } };
                }
            }
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "font-weight")) {
            if (c := Size2D(FontWeight).parse(input).asValue()) {
                if (input.expectExhausted().isOk()) {
                    return { result: { font_weight: c } };
                }
            }
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "font-style")) {
            if (c := FontStyle.parse(input).asValue()) {
                if (input.expectExhausted().isOk()) {
                    return { result: { font_style: c } };
                }
            }
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "font-stretch")) {
            if (c := Size2D(FontStretch).parse(input).asValue()) {
                if (input.expectExhausted().isOk()) {
                    return { result: { font_stretch: c } };
                }
            }
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(name, "unicode-range")) {
            if (c := input.parseList(UnicodeRange, UnicodeRange.parse).asValue()) {
                if (input.expectExhausted().isOk()) {
                    return { result: { unicode_range: c } };
                }
            }
        } else {
            //
        }

        input.reset(&state);
        const opts = css.ParserOptions.default(input.allocator(), null);
        return {
            result: {
                custom: switch (CustomProperty.parse(CustomPropertyName.fromStr(name), input, &opts)) {
                    .result => |v| v,
                    .err => |e| return { err: e },
                },
            },
        };
    }
}

pub struct FontFaceRuleBodyItemParser {
    static fn parseQualified(_: *FontFaceDeclarationParser): bool {
        return false;
    }

    static fn parseDeclarations(_: *FontFaceDeclarationParser): bool {
        return true;
    }
}

pub struct FontFaceDeclarationParser {
    pub const AtRuleParser = FontFaceAtRuleParser;
    pub const QualifiedRuleParser = FontFaceQualifiedRuleParser;
    pub const DeclarationParser = FontFaceValueParser;
    pub const RuleBodyItemParser = FontFaceRuleBodyItemParser;
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
