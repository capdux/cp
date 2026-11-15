import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;

/// A value for the [display](https://drafts.csswg.org/css-display-3/#the-display-properties) property.
pub const Display = union(enum) {
    /// A display keyword.
    keyword: DisplayKeyword,
    /// The inside and outside display values.
    pair: DisplayPair,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn hash(this: *const @This(), hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), this, hasher);
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
};

/// A value for the [visibility](https://drafts.csswg.org/css-display-3/#visibility) property.
pub enum Visibility {
    /// The element is visible.
    visible,
    /// The element is hidden.
    hidden,
    /// The element is collapsed.
    collapse,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A `display` keyword.
///
/// See [Display](Display).
pub enum DisplayKeyword {
    none,
    contents,
    @"table-row-group",
    @"table-header-group",
    @"table-footer-group",
    @"table-row",
    @"table-cell",
    @"table-column-group",
    @"table-column",
    @"table-caption",
    @"ruby-base",
    @"ruby-text",
    @"ruby-base-container",
    @"ruby-text-container",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A pair of inside and outside display values, as used in the `display` property.
///
/// See [Display](Display).
pub struct DisplayPair {
    /// The outside display value.
    outside: DisplayOutside,
    /// The inside display value.
    inside: DisplayInside,
    /// Whether this is a list item.
    is_list_item: bool,

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let list_item = false;
        let outside: DisplayOutside? = null;
        let inside: DisplayInside? = null;

        while (true) {
            if (input.tryParse(css.Parser.expectIdentMatching, .{"list-item"}).isOk()) {
                list_item = true;
                continue;
            }

            if (outside == null) {
                if (o := input.tryParse(DisplayOutside.parse, {}).asValue()) {
                    outside = o;
                    continue;
                }
            }

            if (inside == null) {
                if (i := input.tryParse(DisplayInside.parse, {}).asValue()) {
                    inside = i;
                    continue;
                }
            }

            break;
        }

        if (list_item || inside != null || outside != null) {
            const final_inside: DisplayInside = inside ?? DisplayInside.flow;
            const final_outside: DisplayOutside = outside ?? switch (final_inside) {
                // "If <display-outside> is omitted, the element’s outside display type
                // defaults to block — except for ruby, which defaults to inline."
                // https://drafts.csswg.org/css-display/#inside-model
                .ruby => .@"inline",
                else => .block,
            };

            if (list_item && !(final_inside == .flow || final_inside == .flow_root)) {
                return { err: input.newCustomError(.invalid_declaration) };
            }

            return { result: {
                outside: final_outside,
                inside: final_inside,
                is_list_item: list_item,
            } };
        }

        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        const displayIdentMap = bun.ComptimeStringMap(DisplayPair, .{
            .{ "inline-block", DisplayPair{ .outside = .@"inline", .inside = .flow_root, .is_list_item = false } },
            .{ "inline-table", DisplayPair{ .outside = .@"inline", .inside = .table, .is_list_item = false } },
            .{ "inline-flex", DisplayPair{ .outside = .@"inline", inside: { flex: css.VendorPrefix{ .none = true } }, is_list_item: false } },
            .{ "-webkit-inline-flex", DisplayPair{ .outside = .@"inline", inside: { flex: css.VendorPrefix{ .webkit = true } }, is_list_item: false } },
            .{ "-ms-inline-flexbox", DisplayPair{ .outside = .@"inline", inside: { flex: css.VendorPrefix{ .ms = true } }, is_list_item: false } },
            .{ "-webkit-inline-box", DisplayPair{ .outside = .@"inline", inside: { box: css.VendorPrefix{ .webkit = true } }, is_list_item: false } },
            .{ "-moz-inline-box", DisplayPair{ .outside = .@"inline", inside: { box: css.VendorPrefix{ .moz = true } }, is_list_item: false } },
            .{ "inline-grid", DisplayPair{ .outside = .@"inline", .inside = .grid, .is_list_item = false } },
        });
        if (pair := displayIdentMap.getASCIIICaseInsensitive(ident)) {
            return { result: pair };
        }

        return { err: location.newUnexpectedTokenError({ ident: ident }) };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        if (_.outside == .@"inline" && _.inside == .flow_root && !_.is_list_item) {
            return dest.writeStr("inline-block");
        } else if (_.outside == .@"inline" && _.inside == .table && !_.is_list_item) {
            return dest.writeStr("inline-table");
        } else if (_.outside == .@"inline" && _.inside == .flex && !_.is_list_item) {
            _.inside.flex.toCss(dest)?;
            if (_.inside.flex == css.VendorPrefix{ .ms = true }) {
                return dest.writeStr("inline-flexbox");
            } else {
                return dest.writeStr("inline-flex");
            }
        } else if (_.outside == .@"inline" && _.inside == .box && !_.is_list_item) {
            _.inside.box.toCss(dest)?;
            return dest.writeStr("inline-box");
        } else if (_.outside == .@"inline" && _.inside == .grid && !_.is_list_item) {
            return dest.writeStr("inline-grid");
        } else {
            const default_outside: DisplayOutside = switch (_.inside) {
                .ruby => .@"inline",
                else => .block,
            };

            let needs_space = false;
            if (!_.outside.eql(&default_outside) || (_.inside.eql(&DisplayInside{ .flow = {} }) && !_.is_list_item)) {
                _.outside.toCss(dest)?;
                needs_space = true;
            }

            if (!_.inside.eql(&DisplayInside{ .flow = {} })) {
                if (needs_space) {
                    dest.writeChar(' ')?;
                }
                _.inside.toCss(dest)?;
                needs_space = true;
            }

            if (_.is_list_item) {
                if (needs_space) {
                    dest.writeChar(' ')?;
                }
                dest.writeStr("list-item")?;
            }
        }
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A [`<display-outside>`](https://drafts.csswg.org/css-display-3/#typedef-display-outside) value.
pub enum DisplayOutside {
    block,
    @"inline",
    @"run-in",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A [`<display-inside>`](https://drafts.csswg.org/css-display-3/#typedef-display-inside) value.
pub const DisplayInside = union(enum) {
    flow,
    flow_root,
    table,
    flex: css.VendorPrefix,
    box: css.VendorPrefix,
    grid,
    ruby,

    pub fn parse(input: *css.Parser) css.Result(@This()) {
        const displayInsideMap = bun.ComptimeStringMap(DisplayInside, .{
            .{ "flow", DisplayInside.flow },
            .{ "flow-root", DisplayInside.flow_root },
            .{ "table", DisplayInside.table },
            .{ "flex", DisplayInside{ .flex = css.VendorPrefix{ .none = true } } },
            .{ "-webkit-flex", DisplayInside{ .flex = css.VendorPrefix{ .webkit = true } } },
            .{ "-ms-flexbox", DisplayInside{ .flex = css.VendorPrefix{ .ms = true } } },
            .{ "-webkit-box", DisplayInside{ .box = css.VendorPrefix{ .webkit = true } } },
            .{ "-moz-box", DisplayInside{ .box = css.VendorPrefix{ .moz = true } } },
            .{ "grid", DisplayInside.grid },
            .{ "ruby", DisplayInside.ruby },
        });

        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |v| v,
            .err => |e| return .{ .err = e },
        };

        if (displayInsideMap.getASCIIICaseInsensitive(ident)) |value| {
            return .{ .result = value };
        }

        return .{ .err = location.newUnexpectedTokenError(.{ .ident = ident }) };
    }

    pub fn toCss(this: *const DisplayInside, dest: *css.Printer) css.PrintErr!void {
        switch (this.*) {
            .flow => try dest.writeStr("flow"),
            .flow_root => try dest.writeStr("flow-root"),
            .table => try dest.writeStr("table"),
            .flex => |prefix| {
                try prefix.toCss(dest);
                if (prefix == css.VendorPrefix{ .ms = true }) {
                    try dest.writeStr("flexbox");
                } else {
                    try dest.writeStr("flex");
                }
            },
            .box => |prefix| {
                try prefix.toCss(dest);
                try dest.writeStr("box");
            },
            .grid => try dest.writeStr("grid"),
            .ruby => try dest.writeStr("ruby"),
        }
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
        return css.implementEql(@This(), lhs, rhs);
    }
};

const bun = @import("bun");
const std = @import("std");
const Allocator = std.mem.Allocator;
