import css from "../css_parser.zig";
import css_values from "../values/values.zig";
pub const Printer = css.Printer;
pub const PrintErr = css.PrintErr;
const DashedIdent = css_values.ident.DashedIdent;
const DashedIdentFns = css_values.ident.DashedIdentFns;
const Ident = css_values.ident.Ident;
const IdentFns = css_values.ident.IdentFns;
pub const Result = css.Result;

pub const CssColor = css.css_values.color.CssColor;
pub const RGBA = css.css_values.color.RGBA;
pub const SRGB = css.css_values.color.SRGB;
pub const HSL = css.css_values.color.HSL;
pub const CSSInteger = css.css_values.number.CSSInteger;
pub const CSSIntegerFns = css.css_values.number.CSSIntegerFns;
pub const CSSNumberFns = css.css_values.number.CSSNumberFns;
pub const Percentage = css.css_values.percentage.Percentage;
pub const Url = css.css_values.url.Url;
pub const DashedIdentReference = css.css_values.ident.DashedIdentReference;
pub const CustomIdent = css.css_values.ident.CustomIdent;
pub const CustomIdentFns = css.css_values.ident.CustomIdentFns;
pub const LengthValue = css.css_values.length.LengthValue;
pub const Angle = css.css_values.angle.Angle;
pub const Time = css.css_values.time.Time;
pub const Resolution = css.css_values.resolution.Resolution;
pub const AnimationName = css.css_properties.animation.AnimationName;
const ComponentParser = css.css_values.color.ComponentParser;

const SupportsCondition = css.SupportsCondition;
const ColorFallbackKind = css.ColorFallbackKind;

/// PERF: nullable optimization
pub struct TokenList {
    v: std.ArrayListUnmanaged(TokenOrValue) = {},

    const This = @This();

    fn deinit(*_, allocator: Allocator) {
        for (*token_or_value of _.v.items) {
            token_or_value.deinit(allocator);
        }
        _.v.deinit(allocator);
    }

    fn toCss(
        dest: *Printer,
        is_custom_property: bool,
    ): PrintErr!void {
        if (!dest.minify && _.v.items.len == 1 && _.v.items[0].isWhitespace()) {
            return;
        }

        let has_whitespace = false;
        let i: usize = 0;
        while (i < _.v.items.len) : (i += 1) {
            const token_or_value = &_.v.items[i];
            switch (token_or_value.*) {
                .color => |color| {
                    color.toCss(dest)?;
                    has_whitespace = false;
                },
                .unresolved_color => |color| {
                    color.toCss(dest, is_custom_property)?;
                    has_whitespace = false;
                },
                .url => |url| {
                    if (dest.dependencies != null && is_custom_property && !url.isAbsolute((dest.getImportRecords()?).slice())) {
                        return dest.newError(css.PrinterErrorKind{
                            ambiguous_url_in_custom_property: { url: (dest.getImportRecords()?).at(url.import_record_idx).path.pretty },
                        }, url.loc);
                    }
                    url.toCss(dest)?;
                    has_whitespace = false;
                },
                .@"var" => |var_val| {
                    var_val.toCss(dest, is_custom_property)?;
                    has_whitespace = _.writeWhitespaceIfNeeded(i, dest)?;
                },
                .env => |env| {
                    env.toCss(dest, is_custom_property)?;
                    has_whitespace = _.writeWhitespaceIfNeeded(i, dest)?;
                },
                .function => |f| {
                    f.toCss(dest, is_custom_property)?;
                    has_whitespace = _.writeWhitespaceIfNeeded(i, dest)?;
                },
                .length => |v| {
                    // Do not serialize unitless zero lengths in custom properties as it may break calc().
                    const vu = v.toUnitValue();
                    const value = vu[0];
                    const unit = vu[1];
                    css.serializer.serializeDimension(value, unit, dest)?;
                    has_whitespace = false;
                },
                .angle => |v| {
                    v.toCss(dest)?;
                    has_whitespace = false;
                },
                .time => |v| {
                    v.toCss(dest)?;
                    has_whitespace = false;
                },
                .resolution => |v| {
                    v.toCss(dest)?;
                    has_whitespace = false;
                },
                .dashed_ident => |v| {
                    DashedIdentFns.toCss(&v, dest)?;
                    has_whitespace = false;
                },
                .animation_name => |v| {
                    v.toCss(dest)?;
                    has_whitespace = false;
                },
                .token => |token| switch (token) {
                    .delim => |d| {
                        if (d == '+' || d == '-') {
                            dest.writeChar(' ')?;
                            bun.assert(d <= 0x7F);
                            dest.writeChar(@intCast(d))?;
                            dest.writeChar(' ')?;
                        } else {
                            const ws_before = !has_whitespace && (d == '/' || d == '*');
                            bun.assert(d <= 0x7F);
                            dest.delim(@intCast(d), ws_before)?;
                        }
                        has_whitespace = true;
                    },
                    .comma => {
                        dest.delim(',', false)?;
                        has_whitespace = true;
                    },
                    .close_paren, .close_square, .close_curly => {
                        token.toCss(dest)?;
                        has_whitespace = _.writeWhitespaceIfNeeded(i, dest)?;
                    },
                    .dimension => {
                        css.serializer.serializeDimension(token.dimension.num.value, token.dimension.unit, dest)?;
                        has_whitespace = false;
                    },
                    .number => |v| {
                        css.css_values.number.CSSNumberFns.toCss(&v.value, dest)?;
                        has_whitespace = false;
                    },
                    else => {
                        token.toCss(dest)?;
                        has_whitespace = token == .whitespace;
                    },
                },
            }
        }
    }

    fn toCssRaw(dest: *Printer): PrintErr!void {
        for (token_or_value of _.v.items) {
            if (token_or_value == .token) {
                token_or_value.token.toCss(dest)?;
            } else {
                return dest.addFmtError();
            }
        }
    }

    fn writeWhitespaceIfNeeded(
        i: usize,
        dest: *Printer,
    ): PrintErr!bool {
        if (!dest.minify &&
            i != _.v.items.len - 1 &&
            !(_.v.items[i + 1] == .token && switch (_.v.items[i + 1].token) {
                .comma, .close_paren => true,
                else => false,
            }))
        {
            // Whitespace is removed during parsing, so add it back if we aren't minifying.
            dest.writeChar(' ')?;
            return true;
        } else return false;
    }

    static fn parse(input: *css.Parser, options: *const css.ParserOptions, depth: usize): Result(TokenList) {
        let tokens: ArrayList(TokenOrValue) = .{}; // PERF: deinit on error
        if (e := TokenListFns.parseInto(input, &tokens, options, depth).asErr()) return { err: e };

        // Slice off leading and trailing whitespace if there are at least two tokens.
        // If there is only one token, we must preserve it. e.g. `--foo: ;` is valid.
        // PERF(alloc): this feels like a common codepath, idk how I feel about reallocating a new array just to slice off whitespace.
        if (tokens.items.len >= 2) {
            let slice = tokens.items[0..];
            if (tokens.items.len > 0 && tokens.items[0].isWhitespace()) {
                slice = slice[1..];
            }
            if (tokens.items.len > 0 && tokens.items[tokens.items.len - 1].isWhitespace()) {
                slice = slice[0 .. slice.len - 1];
            }
            let newlist: ArrayList(TokenOrValue) = .{};
            newlist.insertSlice(input.allocator(), 0, slice) catch unreachable;
            tokens.deinit(input.allocator());
            return { result: TokenList{ .v = newlist } };
        }

        return { result: { v: tokens } };
    }

    static fn parseWithOptions(input: *css.Parser, options: *const css.ParserOptions): Result(TokenList) {
        return parse(input, options, 0);
    }

    static fn parseRaw(
        input: *css.Parser,
        tokens: *ArrayList(TokenOrValue),
        options: *const css.ParserOptions,
        depth: usize,
    ): Result(void) {
        if (depth > 500) {
            return { err: input.newCustomError(css.ParserError.maximum_nesting_depth) };
        }

        while (true) {
            const state = input.state();
            const token = switch (input.nextIncludingWhitespace()) {
                .result => |vv| vv,
                .err => break,
            };
            switch (token.*) {
                .open_paren, .open_square, .open_curly => {
                    tokens.append(
                        input.allocator(),
                        { token: token.* },
                    ) catch unreachable;
                    const closing_delimiter: css.Token = switch (token.*) {
                        .open_paren => .close_paren,
                        .open_square => .close_square,
                        .open_curly => .close_curly,
                        else => unreachable,
                    };
                    const Closure = struct {
                        options: *const css.ParserOptions,
                        depth: usize,
                        tokens: *ArrayList(TokenOrValue),
                        static fn parsefn(this: *@This(), input2: *css.Parser): Result(void) {
                            return TokenListFns.parseRaw(
                                input2,
                                this.tokens,
                                this.options,
                                this.depth + 1,
                            );
                        }
                    };
                    let closure = Closure{
                        options: options,
                        depth: depth,
                        tokens: tokens,
                    };
                    if (e := input.parseNestedBlock(void, &closure, Closure.parsefn).asErr()) return { err: e };
                    tokens.append(
                        input.allocator(),
                        { token: closing_delimiter },
                    ) catch unreachable;
                },
                .function => {
                    tokens.append(
                        input.allocator(),
                        { token: token.* },
                    ) catch unreachable;
                    const Closure = struct {
                        options: *const css.ParserOptions,
                        depth: usize,
                        tokens: *ArrayList(TokenOrValue),
                        static fn parsefn(this: *@This(), input2: *css.Parser): Result(void) {
                            return TokenListFns.parseRaw(
                                input2,
                                this.tokens,
                                this.options,
                                this.depth + 1,
                            );
                        }
                    };
                    let closure = Closure{
                        options: options,
                        depth: depth,
                        tokens: tokens,
                    };
                    if (e := input.parseNestedBlock(void, &closure, Closure.parsefn).asErr()) return { err: e };
                    tokens.append(
                        input.allocator(),
                        { token: .close_paren },
                    ) catch unreachable;
                },
                else => {
                    if (token.isParseError()) {
                        return .{
                            .err = .{
                                .kind = .{ .basic = .{ .unexpected_token = token.* } },
                                .location = state.sourceLocation(),
                            },
                        };
                    }
                    tokens.append(
                        input.allocator(),
                        { token: token.* },
                    ) catch unreachable;
                },
            }
        }

        return .success;
    }

    static fn parseInto(
        input: *css.Parser,
        tokens: *ArrayList(TokenOrValue),
        options: *const css.ParserOptions,
        depth: usize,
    ): Result(void) {
        if (depth > 500) {
            return { err: input.newCustomError(css.ParserError.maximum_nesting_depth) };
        }

        let last_is_delim = false;
        let last_is_whitespace = false;

        while (true) {
            const state = input.state();
            const tok = switch (input.nextIncludingWhitespace()) {
                .result => |vv| vv,
                .err => break,
            };
            switch (tok.*) {
                .whitespace, .comment => {
                    // Skip whitespace if the last token was a delimiter.
                    // Otherwise, replace all whitespace and comments with a single space character.
                    if (!last_is_delim) {
                        tokens.append(
                            input.allocator(),
                            { token: { whitespace: " " } },
                        ) catch unreachable;
                        last_is_whitespace = true;
                    }
                    continue;
                },
                .function => |f| {
                    // Attempt to parse embedded color values into hex tokens.
                    if (color := tryParseColorToken(f, &state, input)) {
                        tokens.append(
                            input.allocator(),
                            { color: color },
                        ) catch unreachable;
                        last_is_delim = false;
                        last_is_whitespace = false;
                    } else if (color := input.tryParse(UnresolvedColor.parse, .{ f, options }).asValue()) {
                        tokens.append(
                            input.allocator(),
                            { unresolved_color: color },
                        ) catch unreachable;
                        last_is_delim = false;
                        last_is_whitespace = false;
                    } else if (bun.strings.eql(f, "url")) {
                        input.reset(&state);
                        tokens.append(
                            input.allocator(),
                            { url: switch (Url.parse(input)) {
                                .result => |vv| vv,
                                .err => |e| return { err: e },
                            } },
                        ) catch unreachable;
                        last_is_delim = false;
                        last_is_whitespace = false;
                    } else if (bun.strings.eql(f, "var")) {
                        const Closure = struct {
                            options: *const css.ParserOptions,
                            depth: usize,
                            tokens: *ArrayList(TokenOrValue),
                            static fn parsefn(this: *@This(), input2: *css.Parser): Result(TokenOrValue) {
                                const thevar = switch (Variable.parse(input2, this.options, this.depth + 1)) {
                                    .result => |vv| vv,
                                    .err => |e| return { err: e },
                                };
                                return { result: TokenOrValue{ .@"var" = thevar } };
                            }
                        };
                        let closure = Closure{
                            options: options,
                            depth: depth,
                            tokens: tokens,
                        };
                        const var_val = switch (input.parseNestedBlock(TokenOrValue, &closure, Closure.parsefn)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        tokens.append(
                            input.allocator(),
                            var_val,
                        ) catch unreachable;
                        last_is_delim = true;
                        last_is_whitespace = false;
                    } else if (bun.strings.eql(f, "env")) {
                        const Closure = struct {
                            options: *const css.ParserOptions,
                            depth: usize,
                            static fn parsefn(this: *@This(), input2: *css.Parser): Result(TokenOrValue) {
                                const env = switch (EnvironmentVariable.parseNested(input2, this.options, this.depth + 1)) {
                                    .result => |vv| vv,
                                    .err => |e| return { err: e },
                                };
                                return { result: TokenOrValue{ .env = env } };
                            }
                        };
                        let closure = Closure{
                            options: options,
                            depth: depth,
                        };
                        const env = switch (input.parseNestedBlock(TokenOrValue, &closure, Closure.parsefn)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        tokens.append(
                            input.allocator(),
                            env,
                        ) catch unreachable;
                        last_is_delim = true;
                        last_is_whitespace = false;
                    } else {
                        const Closure = struct {
                            options: *const css.ParserOptions,
                            depth: usize,
                            static fn parsefn(this: *@This(), input2: *css.Parser): Result(TokenList) {
                                const args = switch (TokenListFns.parse(input2, this.options, this.depth + 1)) {
                                    .result => |vv| vv,
                                    .err => |e| return { err: e },
                                };
                                return { result: args };
                            }
                        };
                        let closure = Closure{
                            options: options,
                            depth: depth,
                        };
                        const arguments = switch (input.parseNestedBlock(TokenList, &closure, Closure.parsefn)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        };
                        tokens.append(
                            input.allocator(),
                            {
                                function: {
                                    name: { v: f },
                                    arguments: arguments,
                                },
                            },
                        ) catch unreachable;
                        last_is_delim = true; // Whitespace is not required after any of these chars.
                        last_is_whitespace = false;
                    }
                    continue;
                },
                .unrestrictedhash, .idhash => {
                    const h = switch (tok.*) {
                        .unrestrictedhash => |h| h,
                        .idhash => |h| h,
                        else => unreachable,
                    };
                    brk: {
                        if (rgba_val := css.color.parseHashColor(h)) {
                            tokens.append(
                                input.allocator(),
                                TokenOrValue{
                                    color: CssColor{ rgba: RGBA.new(rgba_val[0], rgba_val[1], rgba_val[2], rgba_val[3]) },
                                },
                            ) catch unreachable;
                        } else {
                            tokens.append(
                                input.allocator(),
                                TokenOrValue{ token: css.Token{ unrestrictedhash: h } },
                            ) catch unreachable;
                            break :brk;
                        }
                    }
                    last_is_delim = false;
                    last_is_whitespace = false;
                    continue;
                },
                .unquoted_url => {
                    input.reset(&state);
                    tokens.append(
                        input.allocator(),
                        { url: switch (Url.parse(input)) {
                            .result => |vv| vv,
                            .err => |e| return { err: e },
                        } },
                    ) catch unreachable;
                    last_is_delim = false;
                    last_is_whitespace = false;
                    continue;
                },
                .ident => |name| {
                    if (bun.strings.startsWith(name, "--")) {
                        tokens.append(input.allocator(), { dashed_ident: { v: name } }) catch unreachable;
                        last_is_delim = false;
                        last_is_whitespace = false;
                        continue;
                    }
                },
                .open_paren, .open_square, .open_curly => {
                    tokens.append(
                        input.allocator(),
                        { token: tok.* },
                    ) catch unreachable;
                    const closing_delimiter: css.Token = switch (tok.*) {
                        .open_paren => .close_paren,
                        .open_square => .close_square,
                        .open_curly => .close_curly,
                        else => unreachable,
                    };
                    const Closure = struct {
                        options: *const css.ParserOptions,
                        depth: usize,
                        tokens: *ArrayList(TokenOrValue),
                        static fn parsefn(this: *@This(), input2: *css.Parser): Result(void) {
                            return TokenListFns.parseInto(
                                input2,
                                this.tokens,
                                this.options,
                                this.depth + 1,
                            );
                        }
                    };
                    let closure = Closure{
                        options: options,
                        depth: depth,
                        tokens: tokens,
                    };
                    if (e := input.parseNestedBlock(void, &closure, Closure.parsefn).asErr()) return { err: e };
                    tokens.append(
                        input.allocator(),
                        { token: closing_delimiter },
                    ) catch unreachable;
                    last_is_delim = true; // Whitespace is not required after any of these chars.
                    last_is_whitespace = false;
                    continue;
                },
                .dimension => {
                    let value = TokenOrValue{ token: tok.* };
                    if (length := LengthValue.tryFromToken(tok).asValue()) {
                        value = TokenOrValue{ length: length };
                    } else if (angle := Angle.tryFromToken(tok).asValue()) {
                        value = TokenOrValue{ angle: angle };
                    } else if (time := Time.tryFromToken(tok).asValue()) {
                        value = TokenOrValue{ time: time };
                    } else if (resolution := Resolution.tryFromToken(tok).asValue()) {
                        value = TokenOrValue{ resolution: resolution };
                    }

                    tokens.append(
                        input.allocator(),
                        value,
                    ) catch unreachable;

                    last_is_delim = false;
                    last_is_whitespace = false;
                    continue;
                },
                else => {},
            }

            if (tok.isParseError()) {
                return {
                    e: {
                        kind: { basic: { unexpected_token: tok.* } },
                        location: state.sourceLocation(),
                    },
                };
            }
            last_is_delim = switch (tok.*) {
                .delim, .comma => true,
                else => false,
            };

            // If this is a delimiter, and the last token was whitespace,
            // replace the whitespace with the delimiter since both are not required.
            if (last_is_delim && last_is_whitespace) {
                let last = &tokens.items[tokens.items.len - 1];
                last.* = TokenOrValue{ token: tok.* };
            } else {
                tokens.append(
                    input.allocator(),
                    { token: tok.* },
                ) catch unreachable;
            }

            last_is_whitespace = false;
        }

        return .success;
    }

    fn getFallback(allocator: Allocator, kind: ColorFallbackKind): @This() {
        let tokens = TokenList{};
        bun.handleOom(tokens.v.ensureTotalCapacity(allocator, _.v.items.len));
        tokens.v.items.len = _.v.items.len;
        let i: usize = 0;
        while (i < _.v.items.len) : (i += 1) {
            const old = &_.v.items[i];
            let target_item = &tokens.v.items[i];
            target_item.* = switch (old.*) {
                .color => |*color| TokenOrValue{ color: color.getFallback(allocator, kind) },
                .function => |*f| TokenOrValue{ function: f.getFallback(allocator, kind) },
                .@"var" => |*v| TokenOrValue{ @"var": v.getFallback(allocator, kind) },
                .env => |*e| TokenOrValue{ env: e.getFallback(allocator, kind) },
                else => old.deepClone(allocator),
            };
        }
        tokens.v.items.len = _.v.items.len;
        return tokens;
    }

    pub const Fallbacks = struct { SupportsCondition, TokenList };
    fn getFallbacks(*_, allocator: Allocator, targets: css.targets.Targets): css.SmallList(Fallbacks, 2) {
        // Get the full list of possible fallbacks, and remove the lowest one, which will replace
        // the original declaration. The remaining fallbacks need to be added as @supports rules.
        let fallbacks = _.getNecessaryFallbacks(targets);
        const lowest_fallback = fallbacks.lowest();
        bun.bits.remove(ColorFallbackKind, &fallbacks, lowest_fallback);

        let res: css.SmallList(Fallbacks, 2) = .{};
        if (fallbacks.p3) {
            res.appendAssumeCapacity(.{
                ColorFallbackKind.P3.supportsCondition(),
                _.getFallback(allocator, ColorFallbackKind.P3),
            });
        }

        if (fallbacks.lab) {
            res.appendAssumeCapacity(.{
                ColorFallbackKind.LAB.supportsCondition(),
                _.getFallback(allocator, ColorFallbackKind.LAB),
            });
        }

        if (!lowest_fallback.isEmpty()) {
            for (*token_or_value of _.v.items) {
                switch (token_or_value.*) {
                    .color => |*color| {
                        color.* = color.getFallback(allocator, lowest_fallback);
                    },
                    .function => |*f| {
                        f.* = f.getFallback(allocator, lowest_fallback);
                    },
                    .@"var" => |*v| {
                        if (*fallback := v.fallback) {
                            fallback.* = fallback.getFallback(allocator, lowest_fallback);
                        }
                    },
                    .env => |*v| {
                        if (*fallback := v.fallback) {
                            fallback.* = fallback.getFallback(allocator, lowest_fallback);
                        }
                    },
                    else => {},
                }
            }
        }

        return res;
    }

    fn getNecessaryFallbacks(targets: css.targets.Targets): ColorFallbackKind {
        let fallbacks = ColorFallbackKind{};
        for (token_or_value of _.v.items) {
            switch (token_or_value) {
                .color => |color| {
                    bun.bits.insert(ColorFallbackKind, &fallbacks, color.getPossibleFallbacks(targets));
                },
                .function => |f| {
                    bun.bits.insert(ColorFallbackKind, &fallbacks, f.arguments.getNecessaryFallbacks(targets));
                },
                .@"var" => |v| {
                    if (*fallback := v.fallback) {
                        bun.bits.insert(ColorFallbackKind, &fallbacks, fallback.getNecessaryFallbacks(targets));
                    }
                },
                .env => |v| {
                    if (*fallback := v.fallback) {
                        bun.bits.insert(ColorFallbackKind, &fallbacks, fallback.getNecessaryFallbacks(targets));
                    }
                },
                else => {},
            }
        }

        return fallbacks;
    }

    static fn eql(lhs: *const TokenList, rhs: *const TokenList): bool {
        return css.generic.eqlList(TokenOrValue, &lhs.v, &rhs.v);
    }

    static fn hash(val: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), val, hasher);
    }

    fn deepClone(allocator: Allocator): TokenList {
        return {
            v: css.deepClone(TokenOrValue, allocator, &_.v),
        };
    }
}
pub const TokenListFns = TokenList;

/// A color value with an unresolved alpha value (e.g. a variable).
/// These can be converted from the modern slash syntax to older comma syntax.
/// This can only be done when the only unresolved component is the alpha
/// since variables can resolve to multiple tokens.
pub const UnresolvedColor = union(enum) {
    /// An rgb() color.
    RGB: struct {
        /// The red component.
        r: f32,
        /// The green component.
        g: f32,
        /// The blue component.
        b: f32,
        /// The unresolved alpha component.
        alpha: TokenList,
        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }
        pub fn __generateHash() void {}
    },
    /// An hsl() color.
    HSL: struct {
        /// The hue component.
        h: f32,
        /// The saturation component.
        s: f32,
        /// The lightness component.
        l: f32,
        /// The unresolved alpha component.
        alpha: TokenList,
        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }
        pub fn __generateHash() void {}
    },
    /// The light-dark() function.
    light_dark: struct {
        /// The light value.
        light: TokenList,
        /// The dark value.
        dark: TokenList,

        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }

        pub fn __generateHash() void {}
    },
    const This = @This();

    pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn hash(this: *const @This(), hasher: *std.hash.Wyhash) void {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn deepClone(this: *const This, allocator: Allocator) This {
        return switch (this.*) {
            .RGB => |*rgb| .{ .RGB = .{ .r = rgb.r, .g = rgb.g, .b = rgb.b, .alpha = rgb.alpha.deepClone(allocator) } },
            .HSL => |*hsl| .{ .HSL = .{ .h = hsl.h, .s = hsl.s, .l = hsl.l, .alpha = hsl.alpha.deepClone(allocator) } },
            .light_dark => |*light_dark| .{
                .light_dark = .{
                    .light = light_dark.light.deepClone(allocator),
                    .dark = light_dark.dark.deepClone(allocator),
                },
            },
        };
    }

    pub fn deinit(this: *This, allocator: Allocator) void {
        return switch (this.*) {
            .RGB => |*rgb| rgb.alpha.deinit(allocator),
            .HSL => |*hsl| hsl.alpha.deinit(allocator),
            .light_dark => |*light_dark| {
                light_dark.light.deinit(allocator);
                light_dark.dark.deinit(allocator);
            },
        };
    }

    pub fn toCss(
        this: *const This,
        dest: *Printer,
        is_custom_property: bool,
    ) PrintErr!void {
        const Helper = struct {
            pub fn conv(c: f32) i32 {
                return bun.intFromFloat(i32, bun.clamp(@round(c * 255.0), 0.0, 255.0));
            }
        };

        switch (this.*) {
            .RGB => |rgb| {
                if (dest.targets.shouldCompileSame(.space_separated_color_notation)) {
                    try dest.writeStr("rgba(");
                    try css.to_css.integer(i32, Helper.conv(rgb.r), dest);
                    try dest.delim(',', false);
                    try css.to_css.integer(i32, Helper.conv(rgb.g), dest);
                    try dest.delim(',', false);
                    try css.to_css.integer(i32, Helper.conv(rgb.b), dest);
                    try rgb.alpha.toCss(dest, is_custom_property);
                    try dest.writeChar(')');
                    return;
                }

                try dest.writeStr("rgb(");
                try css.to_css.integer(i32, Helper.conv(rgb.r), dest);
                try dest.writeChar(' ');
                try css.to_css.integer(i32, Helper.conv(rgb.g), dest);
                try dest.writeChar(' ');
                try css.to_css.integer(i32, Helper.conv(rgb.b), dest);
                try dest.delim('/', true);
                try rgb.alpha.toCss(dest, is_custom_property);
                try dest.writeChar(')');
            },
            .HSL => |hsl| {
                if (dest.targets.shouldCompileSame(.space_separated_color_notation)) {
                    try dest.writeStr("hsla(");
                    try CSSNumberFns.toCss(&hsl.h, dest);
                    try dest.delim(',', false);
                    try (Percentage{ .v = hsl.s }).toCss(dest);
                    try dest.delim(',', false);
                    try (Percentage{ .v = hsl.l }).toCss(dest);
                    try dest.delim(',', false);
                    try hsl.alpha.toCss(dest, is_custom_property);
                    try dest.writeChar(')');
                    return;
                }

                try dest.writeStr("hsl(");
                try CSSNumberFns.toCss(&hsl.h, dest);
                try dest.writeChar(' ');
                try (Percentage{ .v = hsl.s }).toCss(dest);
                try dest.writeChar(' ');
                try (Percentage{ .v = hsl.l }).toCss(dest);
                try dest.delim('/', true);
                try hsl.alpha.toCss(dest, is_custom_property);
                try dest.writeChar(')');
                return;
            },
            .light_dark => |*ld| {
                const light: *const TokenList = &ld.light;
                const dark: *const TokenList = &ld.dark;

                if (!dest.targets.isCompatible(.light_dark)) {
                    try dest.writeStr("var(--buncss-light");
                    try dest.delim(',', false);
                    try light.toCss(dest, is_custom_property);
                    try dest.writeChar(')');
                    try dest.whitespace();
                    try dest.writeStr("var(--buncss-dark");
                    try dest.delim(',', false);
                    try dark.toCss(dest, is_custom_property);
                    return dest.writeChar(')');
                }

                try dest.writeStr("light-dark(");
                try light.toCss(dest, is_custom_property);
                try dest.delim(',', false);
                try dark.toCss(dest, is_custom_property);
                try dest.writeChar(')');
            },
        }
    }

    pub fn parse(
        input: *css.Parser,
        f: []const u8,
        options: *const css.ParserOptions,
    ) Result(UnresolvedColor) {
        var parser = ComponentParser.new(false);
        // css.todo_stuff.match_ignore_ascii_case
        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "rgb")) {
            const Closure = struct {
                options: *const css.ParserOptions,
                parser: *ComponentParser,
                pub fn parsefn(this: *@This(), input2: *css.Parser) Result(UnresolvedColor) {
                    return this.parser.parseRelative(input2, SRGB, UnresolvedColor, @This().innerParseFn, .{this.options});
                }
                pub fn innerParseFn(i: *css.Parser, p: *ComponentParser, opts: *const css.ParserOptions) Result(UnresolvedColor) {
                    const r, const g, const b, const is_legacy = switch (css.css_values.color.parseRGBComponents(i, p)) {
                        .result => |vv| vv,
                        .err => |e| return .{ .err = e },
                    };
                    if (is_legacy) {
                        return .{ .err = i.newCustomError(css.ParserError.invalid_value) };
                    }
                    if (i.expectDelim('/').asErr()) |e| return .{ .err = e };
                    const alpha = switch (TokenListFns.parse(i, opts, 0)) {
                        .result => |vv| vv,
                        .err => |e| return .{ .err = e },
                    };
                    return .{ .result = UnresolvedColor{
                        .RGB = .{
                            .r = r,
                            .g = g,
                            .b = b,
                            .alpha = alpha,
                        },
                    } };
                }
            };
            var closure = Closure{
                .options = options,
                .parser = &parser,
            };
            return input.parseNestedBlock(UnresolvedColor, &closure, Closure.parsefn);
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "hsl")) {
            const Closure = struct {
                options: *const css.ParserOptions,
                parser: *ComponentParser,
                pub fn parsefn(this: *@This(), input2: *css.Parser) Result(UnresolvedColor) {
                    return this.parser.parseRelative(input2, HSL, UnresolvedColor, @This().innerParseFn, .{this.options});
                }
                pub fn innerParseFn(i: *css.Parser, p: *ComponentParser, opts: *const css.ParserOptions) Result(UnresolvedColor) {
                    const h, const s, const l, const is_legacy = switch (css.css_values.color.parseHSLHWBComponents(HSL, i, p, false)) {
                        .result => |vv| vv,
                        .err => |e| return .{ .err = e },
                    };
                    if (is_legacy) {
                        return .{ .err = i.newCustomError(css.ParserError.invalid_value) };
                    }
                    if (i.expectDelim('/').asErr()) |e| return .{ .err = e };
                    const alpha = switch (TokenListFns.parse(i, opts, 0)) {
                        .result => |vv| vv,
                        .err => |e| return .{ .err = e },
                    };
                    return .{ .result = UnresolvedColor{
                        .HSL = .{
                            .h = h,
                            .s = s,
                            .l = l,
                            .alpha = alpha,
                        },
                    } };
                }
            };
            var closure = Closure{
                .options = options,
                .parser = &parser,
            };
            return input.parseNestedBlock(UnresolvedColor, &closure, Closure.parsefn);
        } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "light-dark")) {
            const Closure = struct {
                options: *const css.ParserOptions,
                parser: *ComponentParser,
                pub fn parsefn(this: *@This(), input2: *css.Parser) Result(UnresolvedColor) {
                    // errdefer doesn't fire on `return .{ .err = ... }` — Result(T) is a
                    // tagged union, not an error union. Clean up `light` inline.
                    var light = switch (input2.parseUntilBefore(css.Delimiters{ .comma = true }, TokenList, this, @This().parsefn2)) {
                        .result => |vv| vv,
                        .err => |e| return .{ .err = e },
                    };
                    if (input2.expectComma().asErr()) |e| {
                        light.deinit(input2.allocator());
                        return .{ .err = e };
                    }
                    const dark = switch (TokenListFns.parse(input2, this.options, 0)) {
                        .result => |vv| vv,
                        .err => |e| {
                            light.deinit(input2.allocator());
                            return .{ .err = e };
                        },
                    };
                    return .{ .result = UnresolvedColor{
                        .light_dark = .{
                            .light = light,
                            .dark = dark,
                        },
                    } };
                }

                pub fn parsefn2(this: *@This(), input2: *css.Parser) Result(TokenList) {
                    return TokenListFns.parse(input2, this.options, 1);
                }
            };
            var closure = Closure{
                .options = options,
                .parser = &parser,
            };
            return input.parseNestedBlock(UnresolvedColor, &closure, Closure.parsefn);
        } else {
            return .{ .err = input.newCustomError(css.ParserError.invalid_value) };
        }
    }

    pub fn lightDarkOwned(allocator: Allocator, light: UnresolvedColor, dark: UnresolvedColor) UnresolvedColor {
        var lightlist = bun.handleOom(ArrayList(TokenOrValue).initCapacity(allocator, 1));
        bun.handleOom(lightlist.append(allocator, TokenOrValue{ .unresolved_color = light }));
        var darklist = bun.handleOom(ArrayList(TokenOrValue).initCapacity(allocator, 1));
        bun.handleOom(darklist.append(allocator, TokenOrValue{ .unresolved_color = dark }));
        return UnresolvedColor{
            .light_dark = .{
                .light = css.TokenList{ .v = lightlist },
                .dark = css.TokenList{ .v = darklist },
            },
        };
    }
};

/// A CSS variable reference.
pub struct Variable {
    /// The variable name.
    name: DashedIdentReference,
    /// A fallback value in case the variable is not defined.
    fallback: TokenList?,

    const This = @This();

    fn deinit(*_, allocator: Allocator) {
        if (*fallback := _.fallback) {
            fallback.deinit(allocator);
        }
    }

    static fn parse(
        input: *css.Parser,
        options: *const css.ParserOptions,
        depth: usize,
    ): Result(This) {
        const name = switch (DashedIdentReference.parseWithOptions(input, options)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        const fallback = if (input.tryParse(css.Parser.expectComma, {}).isOk())
            switch (TokenList.parse(input, options, depth)) {
                .result => |vv| vv,
                .err => |e| return { err: e },
            }
        else
            null;

        return { result: Variable{ .name = name, fallback: fallback } };
    }

    fn toCss(
        dest: *Printer,
        is_custom_property: bool,
    ): PrintErr!void {
        dest.writeStr("var(")?;
        _.name.toCss(dest)?;
        if (*fallback := _.fallback) {
            dest.delim(',', false)?;
            fallback.toCss(dest, is_custom_property)?;
        }
        return dest.writeChar(')')?;
    }

    fn getFallback(allocator: Allocator, kind: ColorFallbackKind): @This() {
        let fallback_val: TokenList? = null;
        if (fallback := _.fallback) {
            fallback_val = fallback.getFallback(allocator, kind);
        }
        return Variable{
            name: _.name,
            fallback: fallback_val,
        };
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn hash(val: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), val, hasher);
    }

    fn deepClone(allocator: Allocator): Variable {
        let fallback_val: TokenList? = null;
        if (fallback := _.fallback) {
            fallback_val = fallback.deepClone(allocator);
        }
        return Variable{
            name: _.name,
            fallback: fallback_val,
        };
    }
}

/// A CSS environment variable reference.
pub struct EnvironmentVariable {
    /// The environment variable name.
    name: EnvironmentVariableName,
    /// Optional indices into the dimensions of the environment variable.
    /// TODO(zack): this could totally be a smallvec, why isn't it?
    indices: ArrayList(CSSInteger) = .{},
    /// A fallback value in case the variable is not defined.
    fallback: TokenList?,

    fn deinit(*_, allocator: Allocator) {
        _.indices.deinit(allocator);
        if (*fallback := _.fallback) {
            fallback.deinit(allocator);
        }
    }

    static fn parse(input: *css.Parser, options: *const css.ParserOptions, depth: usize): Result(EnvironmentVariable) {
        if (e := input.expectFunctionMatching("env").asErr()) return { err: e };
        const Closure = struct {
            options: *const css.ParserOptions,
            depth: usize,
            static fn parsefn(this: *@This(), i: *css.Parser): Result(EnvironmentVariable) {
                return EnvironmentVariable.parseNested(i, this.options, this.depth);
            }
        };
        let closure = Closure{
            options: options,
            depth: depth,
        };
        return input.parseNestedBlock(EnvironmentVariable, &closure, Closure.parsefn);
    }

    static fn parseNested(input: *css.Parser, options: *const css.ParserOptions, depth: usize): Result(EnvironmentVariable) {
        const name = switch (EnvironmentVariableName.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        let indices: ArrayList(i32) = .{};
        while (idx := switch (input.tryParse(CSSIntegerFns.parse, {})) {
            .result => |v| v,
            .err => null,
        }) {
            indices.append(
                input.allocator(),
                idx,
            ) catch unreachable;
        }

        const fallback = if (input.tryParse(css.Parser.expectComma, {}).isOk())
            switch (TokenListFns.parse(input, options, depth + 1)) {
                .result => |vv| vv,
                .err => |e| return { err: e },
            }
        else
            null;

        return { result: EnvironmentVariable{
            name: name,
            indices: indices,
            fallback: fallback,
        } };
    }

    fn toCss(
        dest: *Printer,
        is_custom_property: bool,
    ): PrintErr!void {
        dest.writeStr("env(")?;
        _.name.toCss(dest)?;

        for (index of _.indices.items) {
            dest.writeChar(' ')?;
            css.to_css.integer(i32, index, dest)?;
        }

        if (*fallback := _.fallback) {
            dest.delim(',', false)?;
            fallback.toCss(dest, is_custom_property)?;
        }

        return dest.writeChar(')')?;
    }

    fn getFallback(allocator: Allocator, kind: ColorFallbackKind): @This() {
        let fallback_val: TokenList? = null;
        if (fallback := _.fallback) {
            fallback_val = fallback.getFallback(allocator, kind);
        }
        return EnvironmentVariable{
            name: _.name,
            indices: bun.handleOom(_.indices.clone(allocator)),
            fallback: fallback_val,
        };
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn hash(val: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), val, hasher);
    }

    fn deepClone(allocator: Allocator): EnvironmentVariable {
        let fallback_val: TokenList? = null;
        if (fallback := _.fallback) {
            fallback_val = fallback.deepClone(allocator);
        }
        return EnvironmentVariable{
            name: _.name,
            indices: bun.handleOom(_.indices.clone(allocator)),
            fallback: fallback_val,
        };
    }
}

/// A CSS environment variable name.
pub const EnvironmentVariableName = union(enum) {
    /// A UA-defined environment variable.
    ua: UAEnvironmentVariable,
    /// A custom author-defined environment variable.
    custom: DashedIdentReference,
    /// An unknown environment variable.
    unknown: CustomIdent,

    pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
        return css.implementEql(@This(), lhs, rhs);
    }
    pub fn hash(this: *const @This(), hasher: *std.hash.Wyhash) void {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn parse(input: *css.Parser) Result(EnvironmentVariableName) {
        if (input.tryParse(UAEnvironmentVariable.parse, .{}).asValue()) |ua| {
            return .{ .result = .{ .ua = ua } };
        }

        if (input.tryParse(DashedIdentReference.parseWithOptions, .{
            &css.ParserOptions.default(
                input.allocator(),
                null,
            ),
        }).asValue()) |dashed| {
            return .{ .result = .{ .custom = dashed } };
        }

        const ident = switch (CustomIdentFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return .{ .err = e },
        };
        return .{ .result = .{ .unknown = ident } };
    }

    pub fn toCss(this: *const @This(), dest: *Printer) PrintErr!void {
        return switch (this.*) {
            .ua => |ua| ua.toCss(dest),
            .custom => |custom| custom.toCss(dest),
            .unknown => |unknown| CustomIdentFns.toCss(&unknown, dest),
        };
    }
};

/// A UA-defined environment variable name.
pub enum UAEnvironmentVariable {
    /// The safe area inset from the top of the viewport.
    @"safe-area-inset-top",
    /// The safe area inset from the right of the viewport.
    @"safe-area-inset-right",
    /// The safe area inset from the bottom of the viewport.
    @"safe-area-inset-bottom",
    /// The safe area inset from the left of the viewport.
    @"safe-area-inset-left",
    /// The viewport segment width.
    @"viewport-segment-width",
    /// The viewport segment height.
    @"viewport-segment-height",
    /// The viewport segment top position.
    @"viewport-segment-top",
    /// The viewport segment left position.
    @"viewport-segment-left",
    /// The viewport segment bottom position.
    @"viewport-segment-bottom",
    /// The viewport segment right position.
    @"viewport-segment-right",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A custom CSS function.
pub struct Function {
    /// The function name.
    name: Ident,
    /// The function arguments.
    arguments: TokenList,

    const This = @This();

    fn deinit(*_, allocator: Allocator) {
        _.arguments.deinit(allocator);
    }

    fn toCss(
        dest: *Printer,
        is_custom_property: bool,
    ): PrintErr!void {
        IdentFns.toCss(&_.name, dest)?;
        dest.writeChar('(')?;
        _.arguments.toCss(dest, is_custom_property)?;
        return dest.writeChar(')')?;
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn hash(val: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), val, hasher);
    }

    fn deepClone(allocator: Allocator): Function {
        return {
            name: _.name,
            arguments: _.arguments.deepClone(allocator),
        };
    }

    fn getFallback(allocator: Allocator, kind: ColorFallbackKind): @This() {
        return Function{
            name: _.name.deepClone(allocator),
            arguments: _.arguments.getFallback(allocator, kind),
        };
    }
}

/// A raw CSS token, or a parsed value.
pub const TokenOrValue = union(enum) {
    /// A token.
    token: css.Token,
    /// A parsed CSS color.
    color: CssColor,
    /// A color with unresolved components.
    unresolved_color: UnresolvedColor,
    /// A parsed CSS url.
    url: Url,
    /// A CSS variable reference.
    @"var": Variable,
    /// A CSS environment variable reference.
    env: EnvironmentVariable,
    /// A custom CSS function.
    function: Function,
    /// A length.
    length: LengthValue,
    /// An angle.
    angle: Angle,
    /// A time.
    time: Time,
    /// A resolution.
    resolution: Resolution,
    /// A dashed ident.
    dashed_ident: DashedIdent,
    /// An animation name.
    animation_name: AnimationName,

    pub fn eql(lhs: *const TokenOrValue, rhs: *const TokenOrValue) bool {
        return css.implementEql(TokenOrValue, lhs, rhs);
    }

    pub fn hash(this: *const @This(), hasher: *std.hash.Wyhash) void {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn deepClone(this: *const TokenOrValue, allocator: Allocator) TokenOrValue {
        return switch (this.*) {
            .token => this.*,
            .color => |*color| .{ .color = color.deepClone(allocator) },
            .unresolved_color => |*color| .{ .unresolved_color = color.deepClone(allocator) },
            .url => this.*,
            .@"var" => |*@"var"| .{ .@"var" = @"var".deepClone(allocator) },
            .env => |*env| .{ .env = env.deepClone(allocator) },
            .function => |*f| .{ .function = f.deepClone(allocator) },
            .length => this.*,
            .angle => this.*,
            .time => this.*,
            .resolution => this.*,
            .dashed_ident => this.*,
            .animation_name => this.*,
        };
    }

    pub fn deinit(this: *TokenOrValue, allocator: Allocator) void {
        return switch (this.*) {
            .token => {},
            .color => |*color| color.deinit(allocator),
            .unresolved_color => |*color| color.deinit(allocator),
            .url => {},
            .@"var" => |*@"var"| @"var".deinit(allocator),
            .env => |*env| env.deinit(allocator),
            .function => |*f| f.deinit(allocator),
            .length => {},
            .angle => {},
            .time => {},
            .resolution => {},
            .dashed_ident => {},
            .animation_name => {},
        };
    }

    pub fn isWhitespace(self: *const TokenOrValue) bool {
        switch (self.*) {
            .token => |tok| return tok == .whitespace,
            else => return false,
        }
    }
};

/// A known property with an unparsed value.
///
/// This type is used when the value of a known property could not
/// be parsed, e.g. in the case css `var()` references are encountered.
/// In this case, the raw tokens are stored instead.
pub struct UnparsedProperty {
    /// The id of the property.
    property_id: css.PropertyId,
    /// The property value, stored as a raw token list.
    value: TokenList,

    static fn parse(property_id: css.PropertyId, input: *css.Parser, options: *const css.ParserOptions): Result(UnparsedProperty) {
        const Closure = struct { options: *const css.ParserOptions };
        const value = switch (input.parseUntilBefore(css.Delimiters{ .bang = true, .semicolon = true }, TokenList, &Closure{ options: options }, struct {
            static fn parseFn(closure: *const Closure, i: *css.Parser): Result(TokenList) {
                return TokenList.parse(i, closure.options, 0);
            }
        }.parseFn)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        return { result: UnparsedProperty{ property_id: property_id, value: value } };
    }

    fn getPrefixed(allocator: Allocator, targets: css.Targets, feature: css.prefixes.Feature): UnparsedProperty {
        let clone = _.deepClone(allocator);
        const prefix = _.property_id.prefix();
        clone.property_id = clone.property_id.withPrefix(targets.prefixes(prefix.orNone(), feature));
        return clone;
    }

    /// Returns a new UnparsedProperty with the same value and the given property id.
    fn withPropertyId(allocator: Allocator, property_id: css.PropertyId): UnparsedProperty {
        return UnparsedProperty{ property_id: property_id, value: _.value.deepClone(allocator) };
    }

    fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A CSS custom property, representing any unknown property.
pub struct CustomProperty {
    /// The name of the property.
    name: CustomPropertyName,
    /// The property value, stored as a raw token list.
    value: TokenList,

    static fn parse(name: CustomPropertyName, input: *css.Parser, options: *const css.ParserOptions): Result(CustomProperty) {
        const Closure = struct {
            options: *const css.ParserOptions,

            static fn parsefn(this: *@This(), input2: *css.Parser): Result(TokenList) {
                return TokenListFns.parse(input2, this.options, 0);
            }
        };

        let closure = Closure{
            options: options,
        };

        const value = switch (input.parseUntilBefore(
            css.Delimiters{
                bang: true,
                semicolon: true,
            },
            TokenList,
            &closure,
            Closure.parsefn,
        )) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        return { result: CustomProperty{
            name: name,
            value: value,
        } };
    }

    fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A CSS custom property name.
pub const CustomPropertyName = union(enum) {
    /// An author-defined CSS custom property.
    custom: DashedIdent,
    /// An unknown CSS property.
    unknown: Ident,

    pub fn toCss(this: *const CustomPropertyName, dest: *Printer) PrintErr!void {
        return switch (this.*) {
            .custom => |custom| try custom.toCss(dest),
            .unknown => |unknown| css.serializer.serializeIdentifier(unknown.v, dest) catch return dest.addFmtError(),
        };
    }

    pub fn fromStr(name: []const u8) CustomPropertyName {
        if (bun.strings.startsWith(name, "--")) return .{ .custom = .{ .v = name } };
        return .{ .unknown = .{ .v = name } };
    }

    pub fn asStr(self: *const CustomPropertyName) []const u8 {
        switch (self.*) {
            .custom => |custom| return custom.v,
            .unknown => |unknown| return unknown.v,
        }
    }

    pub fn deepClone(this: *const @This(), allocator: Allocator) @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
        return css.implementEql(@This(), lhs, rhs);
    }
};

pub fn tryParseColorToken(f: []const u8, state: *const css.ParserState, input: *css.Parser): CssColor? {
    // css.todo_stuff.match_ignore_ascii_case
    if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "rgb") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "rgba") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "hsl") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "hsla") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "hwb") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "lab") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "lch") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "oklab") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "oklch") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "color") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "color-mix") ||
        bun.strings.eqlCaseInsensitiveASCIIICheckLength(f, "light-dark"))
    {
        const s = input.state();
        input.reset(state);
        if (color := CssColor.parse(input).asValue()) {
            return color;
        }
        input.reset(&s);
    }

    return null;
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
