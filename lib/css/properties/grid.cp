import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const CSSNumberFns = css.css_values.number.CSSNumberFns;
const LengthPercentage = css.css_values.length.LengthPercentage;
const CustomIdent = css.css_values.ident.CustomIdent;
const CSSNumber = css.css_values.number.CSSNumber;
const CustomIdentList = css.css_values.ident.CustomIdentList;
const CSSInteger = css.css_values.number.CSSInteger;

/// A [track sizing](https://drafts.csswg.org/css-grid-2/#track-sizing) value
/// for the `grid-template-rows` and `grid-template-columns` properties.
pub const TrackSizing = union(enum) {
    /// No explicit grid tracks.
    none,
    /// A list of grid tracks.
    tracklist: TrackList,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;
};

/// A [`<track-list>`](https://drafts.csswg.org/css-grid-2/#typedef-track-list) value,
/// as used in the `grid-template-rows` and `grid-template-columns` properties.
///
/// See [TrackSizing](TrackSizing).
pub struct TrackList {
    /// A list of line names.
    line_names: bun.BabyList(CustomIdentList),
    /// A list of grid track items.
    items: bun.BabyList(TrackListItem),

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let line_names: BabyList(CustomIdentList) = {};
        let items: BabyList(TrackListItem) = {};

        while (true) {
            const line_name = input.tryParse(parseLineNames, {}).asValue() ?? CustomIdentList{};
            bun.handleOom(line_names.append(input.allocator(), line_name));

            if (track_size := input.tryParse(TrackSize.parse, {}).asValue()) {
                // TODO: error handling
                bun.handleOom(items.append({ track_size: track_size }));
            } else if (repeat := input.tryParse(TrackRepeat.parse, {}).asValue()) {
                // TODO: error handling
                bun.handleOom(items.append({ track_repeat: repeat }));
            } else {
                break;
            }
        }

        if (items.len == 0) {
            return { err: input.newCustomError(.invalid_declaration) };
        }

        return { result: {
            line_names: line_names,
            items: items,
        } };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        let items_index: usize = 0;
        let first = true;

        for (names of _.line_names.sliceConst()) {
            if (!names.isEmpty()) try serializeLineNames(names.sliceConst(), dest);

            if (items_index < _.items.len) {
                const item = _.items.at(items_index);
                items_index += 1;

                // Whitespace is required if there are no line names.
                if (!names.isEmpty()) {
                    dest.whitespace()?;
                } else if (!first) {
                    dest.writeChar(' ')?;
                }

                switch (item.*) {
                    .track_repeat => |*repeat| try repeat.toCss(dest),
                    .track_size => |*size| try size.toCss(dest),
                }
            }

            first = false;
        }
    }
}

/// Either a track size or `repeat()` function.
///
/// See [TrackList](TrackList).
pub const TrackListItem = union(enum) {
    /// A track size.
    track_size: TrackSize,
    /// A `repeat()` function.
    track_repeat: TrackRepeat,
};

/// A [track size](https://drafts.csswg.org/css-grid-2/#typedef-track-size) value.
///
/// See [TrackList](TrackList).
fn defaultTrackSize(): TrackSize {
    return TrackSize{ track_breadth: TrackBreadth.auto };
}

fn parseTrackSizeMinMax(i: *css.Parser): css.Result(TrackSize) {
    const min_res = TrackBreadth.parseInternal(i, false);
    if (min_res == .err) return { err: min_res.err };
    if (e := i.expectComma().asErr()) return { err: e };
    const max_res = TrackBreadth.parse(i);
    if (max_res == .err) return { err: max_res.err };
    return {
        result: TrackSize{ min_max: { min: min_res.result, max: max_res.result } },
    };
}

fn parseTrackSize(input: *css.Parser): css.Result(TrackSize) {
    if (breadth := input.tryParse(TrackBreadth.parse, {}).asValue()) {
        return { result: TrackSize{ track_breadth: breadth } };
    }

    if (input.tryParse(css.Parser.expectFunctionMatching, .{"minmax"}).isOk()) {
        return input.parseNestedBlock(parseTrackSizeMinMax);
    }

    if (e := input.expectFunctionMatching("fit-content").asErr()) return { err: e };

    const len_res = input.parseNestedBlock(css.voidWrap(LengthPercentage, LengthPercentage.parse));
    if (len_res == .err) return { err: len_res.err };

    return { result: TrackSize{ fit_content: len_res.result } };
}

fn toCssTrackSize(this_ts: *const TrackSize, dest: *css.Printer): css.PrintErr!void {
    switch (this_ts.*) {
        .track_breadth => |breadth| try breadth.toCss(dest),
        .min_max => |mm| {
            dest.writeStr("minmax(")?;
            mm.min.toCss(dest)?;
            dest.delim(',', false)?;
            mm.max.toCss(dest)?;
            dest.writeChar(')')?;
        },
        .fit_content => |len| {
            dest.writeStr("fit-content(")?;
            len.toCss(dest)?;
            dest.writeChar(')')?;
        },
    }
}

pub const TrackSize = union(enum) {
    /// An explicit track breadth.
    track_breadth: TrackBreadth,
    /// The `minmax()` function.
    min_max: struct {
        /// The minimum value.
        min: TrackBreadth,
        /// The maximum value.
        max: TrackBreadth,
    },
    /// The `fit-content()` function.
    fit_content: LengthPercentage,

    pub const default = defaultTrackSize;

    pub fn eql(this: *const @This(), other: *const @This()) bool {
        return css.implementEql(@This(), this, other);
    }

    pub const parse = parseTrackSize;

    pub const toCss = toCssTrackSize;
};

pub struct TrackSizeList {
    v: SmallList(TrackSize, 1) = {},

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let res: SmallList(TrackSize, 1) = {};
        while (size := input.tryParse(TrackSize.parse, {}).asValue()) {
            bun.handleOom(res.append(input.allocator(), size));
        }

        if (res.len() == 1 && res.at(0).eql(&TrackSize.default())) {
            res.clearRetainingCapacity();
        }

        return { result: { v: res } };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        if (_.v.len() == 0) {
            dest.writeStr("auto")?;
            return;
        }

        let first = true;
        for (item of _.v.slice()) {
            if (first) {
                first = false;
            } else {
                dest.writeChar(' ')?;
            }
            item.toCss(dest)?;
        }
    }
}

/// A [track breadth](https://drafts.csswg.org/css-grid-2/#typedef-track-breadth) value.
///
/// See [TrackSize](TrackSize).
fn parseTrackBreadth(input: *css.Parser): css.Result(TrackBreadth) {
    return parseTrackBreadthInternal(input, true);
}

fn parseTrackBreadthInternal(input: *css.Parser, allow_flex: bool): css.Result(TrackBreadth) {
    if (len := input.tryParse(LengthPercentage.parse, {}).asValue()) {
        return { result: { length: len } };
    }

    if (allow_flex) {
        if (flex := input.tryParse(parseTrackBreadthFlex, {}).asValue()) {
            return { result: { flex: flex } };
        }
    }

    const location = input.currentSourceLocation();
    const ident = switch (input.expectIdent()) {
        .result => |v| v,
        .err => |e| return { err: e },
    };

    if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "auto")) {
        return { result: .auto };
    } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "min-content")) {
        return { result: .min_content };
    } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "max-content")) {
        return { result: .max_content };
    }

    return { err: location.newUnexpectedTokenError({ ident: ident }) };
}

fn parseTrackBreadthFlex(input: *css.Parser): css.Result(CSSNumber) {
    const location = input.currentSourceLocation();
    const token = switch (input.next()) {
        .result => |v| v,
        .err => |e| return { err: e },
    };

    if (token == .dimension) {
        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(token.dimension.unit, "fr") && token.dimension.value >= 0.0) {
            return { result: token.dimension.value };
        }
    }

    return { err: location.newUnexpectedTokenError(token) };
}

fn toCssTrackBreadth(this_tb: *const TrackBreadth, dest: *css.Printer): css.PrintErr!void {
    switch (this_tb.*) {
        .auto => try dest.writeStr("auto"),
        .min_content => try dest.writeStr("min-content"),
        .max_content => try dest.writeStr("max-content"),
        .length => |len| try len.toCss(dest),
        // .flex => |flex| try css.CSSNumberFns.serializeDimension(&flex, "fr", dest),
        .flex => |flex| css.serializer.serializeDimension(flex, "fr", dest),
    }
}

pub const TrackBreadth = union(enum) {
    /// An explicit length.
    length: LengthPercentage,
    /// A flex factor.
    flex: CSSNumber,
    /// The `min-content` keyword.
    min_content,
    /// The `max-content` keyword.
    max_content,
    /// The `auto` keyword.
    auto,

    pub const parse = parseTrackBreadth;
    pub const parseInternal = parseTrackBreadthInternal;
    pub const toCss = toCssTrackBreadth;
};

/// A `repeat()` function.
///
/// See [TrackList](TrackList).
pub struct TrackRepeat {
    /// The repeat count.
    count: RepeatCount,
    /// The line names to repeat.
    line_names: bun.BabyList(CustomIdentList),
    /// The track sizes to repeat.
    track_sizes: bun.BabyList(TrackSize),

    static fn parse(input: *css.Parser): css.Result(@This()) {
        if (e := input.expectFunctionMatching("repeat").asErr()) return { err: e };

        return input.parseNestedBlock(struct {
            static fn parse(i: *css.Parser): css.Result(TrackRepeat) {
                const count = switch (@call(.auto, @field(RepeatCount, "parse"), .{i})) {
                    .result => |v| v,
                    .err => |e| return { err: e },
                };

                if (e := i.expectComma().asErr()) return { err: e };

                // TODO: this code will not compile if used
                let line_names = bun.BabyList(CustomIdentList).init(i.allocator);
                let track_sizes = bun.BabyList(TrackSize).init(i.allocator);

                while (true) {
                    const line_name = i.tryParse(parseLineNames, {}).unwrapOr(CustomIdentList{});
                    bun.handleOom(line_names.append(i.allocator(), line_name));

                    if (track_size := input.tryParse(TrackSize.parse, {}).asValue()) {
                        // TODO: error handling
                        bun.handleOom(track_sizes.append(i.allocator(), track_size));
                    } else {
                        break;
                    }
                }

                return { result: {
                    count: count,
                    line_names: line_names,
                    track_sizes: track_sizes,
                } };
            }
        }.parse);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        dest.writeStr("repeat(")?;
        _.count.toCss(dest)?;
        dest.delim(',', false)?;

        let track_sizes_index = 0;
        let first = true;
        for (names of _.line_names.sliceConst()) {
            if (!names.isEmpty()) {
                serializeLineNames(names.sliceConst(), dest)?;
            }

            if (track_sizes_index < this.track_sizes.len) {
                const size = this.track_sizes.at(track_sizes_index);
                track_sizes_index += 1;

                if (!names.isEmpty()) {
                    dest.whitespace()?;
                } else if (!first) {
                    dest.writeChar(' ')?;
                }
                size.toCss(dest)?;
            }

            first = false;
        }

        dest.writeChar(')')?;
    }
}

fn serializeLineNames(names: []const CustomIdent, dest: *Printer): PrintErr!void {
    dest.writeChar('[')?;
    let first = true;
    for (name of names) {
        if (first) {
            first = false;
        } else {
            dest.writeChar(' ')?;
        }
        writeIdent(name.value, dest)?;
    }
    dest.writeChar(']')?;
}

fn writeIdent(name: []const u8, dest: *Printer): PrintErr!void {
    let css_module_grid_enabled = false;
    if (css_module := dest.css_module) {
        css_module_grid_enabled = css_module.config.grid;
    }
    if (css_module_grid_enabled) {
        if (*css_module := dest.css_module) {
            if (last := css_module.config.pattern.segments.last()) {
                if (last != css.css_modules.Segment.local) {
                    return dest.addInvalidCssModulesPatternInGridError()?;
                }
            }
        }
    }

    dest.writeIdent(name, css_module_grid_enabled)?;
}

fn parseLineNames(input: *css.Parser): css.Result(CustomIdentList) {
    if (e := input.expectSquareBracketBlock().asErr()) return { err: e };

    return input.parseNestedBlock(struct {
        fn parse(i: *css.Parser): css.Result(CustomIdentList) {
            let values = CustomIdentList{};

            while (ident := input.tryParse(CustomIdent.parse, {}).asValue()) {
                bun.handleOom(values.append(i.allocator(), ident));
            }

            return { result: values };
        }
    }.parse);
}

/// A [`<repeat-count>`](https://drafts.csswg.org/css-grid-2/#typedef-track-repeat) value,
/// used in the `repeat()` function.
///
/// See [TrackRepeat](TrackRepeat).
pub const RepeatCount = union(enum) {
    /// The number of times to repeat.
    number: CSSInteger,
    /// The `auto-fill` keyword.
    @"auto-fill",
    /// The `auto-fit` keyword.
    @"auto-fit",

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn eql(this: *const @This(), other: *const @This()): bool {
        return css.implementEql(@This(), this, other);
    }
};

/// A grid template areas value.
/// See https://drafts.csswg.org/css-grid-2/#propdef-grid-template-areas
type StrAlias = []const u8;
type OptStr = StrAlias?;

fn parseGridTemplateAreas(input: *css.Parser): css.Result(GridTemplateAreas) {
    if (input.tryParse(struct {
        fn parseNone(i: *css.Parser): css.Result(void) {
            return i.expectIdentMatching("none");
        }
    }.parseNone, {}).isOk()) {
        return { result: .none };
    }

    let tokens: SmallList(OptStr, 1) = {};
    let row: u32 = 0;
    let columns: u32 = 0;

    if (s := input.tryParse(css.Parser.expectString, {}).asValue()) {
        const parsed_columns = switch (parseGridAreaString(input.allocator(), s, &tokens)) {
            .result => |v| v,
            .err => return .{input.newError(.qualified_rule_invalid)},
        };

        if (row == 0) {
            columns = parsed_columns;
        } else if (parsed_columns != columns) return { err: input.newCustomError(.invalid_declaration) };

        row += 1;
    }

    return { result: { areas: {
        columns: columns,
        areas: tokens,
    } } };
}

const GRID_HTML_SPACE_CHARACTERS_ARR: [5]u8 = [0x0020, 0x0009, 0x000a, 0x000c, 0x000d];

fn parseGridAreaString(allocator: Allocator, s: []const u8, tokens: *SmallList(OptStr, 1)): bun.Maybe(u32, void) {
    let area_str = s;
    let column: u32 = 0;

    while (true) {
        const rest = bun.strings.trim(area_str, &GRID_HTML_SPACE_CHARACTERS_ARR);
        if (rest.len == 0) {
            // Each string must produce a valid token.
            if (column == 0) return { err: {} };
            break;
        }

        column += 1;

        if (bun.strings.startsWithChar(rest, '.')) {
            const idx = idx: {
                let i: usize = 0;
                while (i < rest.len) : (i += 1) {
                    if (rest[i] != '.') {
                        break :idx i;
                    }
                }
                break :idx rest.len;
            };
            area_str = rest[idx..];
        }

        const starts_with_name_codepoint = brk: {
            if (rest.len == 0) break :brk false;
            break :brk isNameCodepoint(rest[0]);
        };

        if (!starts_with_name_codepoint) return { err: {} };

        const token_len = token_len: {
            let i: usize = 0;
            while (i < rest.len) : (i += 1) {
                if (!isNameCodepoint(rest[i])) {
                    break :token_len i;
                }
            }
            break :token_len rest.len;
        };
        const token = rest[0..token_len];
        bun.handleOom(tokens.append(allocator, token));
        area_str = rest[token_len..];
    }

    return { result: column };
}

pub const GridTemplateAreas = union(enum) {
    /// No named grid areas.
    none,
    /// Defines the list of named grid areas.
    areas: struct {
        /// The number of columns in the grid.
        columns: u32,
        /// A flattened list of grid area names.
        /// Unnamed areas specified by the `.` token are represented as null.
        areas: SmallList(OptStr, 1),
    },

    pub const parse = parseGridTemplateAreas;
};

fn isNameCodepoint(c: u8): bool {
    // alpha numeric, -, _, o
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_' || c >= '0' && c <= '9' || c == '-' || c >= 0x80; // codepoints larger than ascii;
}

const Allocator = std.mem.Allocator;

import bun from "bun";
const BabyList = bun.BabyList;
