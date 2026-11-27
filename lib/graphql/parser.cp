// Source: cp/lib/graphql/parser.cp (GraphQL Recursive Descent AST Parser)
const std = @import("std");
import {
    GqlError,
    GqlTokenKind,
    GqlToken,
    GqlObjectField,
    GqlValue,
    GqlTypeRef,
    GqlArgument,
    GqlDirective,
    GqlVariableDef,
    GqlField,
    GqlFragmentSpread,
    GqlInlineFragment,
    GqlSelection,
    GqlOperationType,
    GqlOperation,
    GqlFragment,
    GqlDocument,
    freeValue,
    freeTypeRef,
    freeArguments,
    freeDirectives,
    freeSelectionSet,
    freeVariableDefs,
    freeOperation,
    freeFragment,
} from "./types.cp";
import { tokenize } from "./lexer.cp";

pub const DEFAULT_MAX_DEPTH: usize = 32;

struct Parser {
    alloc: std.mem.Allocator,
    tokens: []const GqlToken,
    pos: usize,
    depth: usize,
    max_depth: usize,
    owned_strings: std.ArrayList([]u8),

    fn peek(): GqlToken {
        if (_.pos < _.tokens.len) {
            return _.tokens[_.pos];
        }
        return _.tokens[_.tokens.len - 1];
    }

    fn peekKind(): GqlTokenKind {
        return _.peek().kind;
    }

    fn advance(*_): GqlToken {
        const tok = _.peek();
        if (tok.kind != .Eof) {
            _.pos += 1;
        }
        return tok;
    }

    fn expect(*_, kind: GqlTokenKind): GqlError!GqlToken {
        const tok = _.peek();
        if (tok.kind != kind) {
            return error.UnexpectedToken;
        }
        _.pos += 1;
        return tok;
    }

    fn expectKeyword(*_, kw: []const u8): GqlError!GqlToken {
        const tok = _.peek();
        if (tok.kind != .Name or !std.mem.eql(u8, tok.lexeme, kw)) {
            return error.UnexpectedToken;
        }
        _.pos += 1;
        return tok;
    }

    fn skip(*_, kind: GqlTokenKind): bool {
        if (_.peekKind() == kind) {
            _.pos += 1;
            return true;
        }
        return false;
    }

    fn addOwnedString(*_, s: []u8): GqlError![]const u8 {
        try _.owned_strings.append(_.alloc, s);
        return s;
    }

    fn decodeString(*_, tok: GqlToken): GqlError![]const u8 {
        if (tok.kind == .StringValue) {
            if (tok.lexeme.len < 2) return error.UnexpectedToken;
            const inner = tok.lexeme[1 .. tok.lexeme.len - 1];
            if (std.mem.indexOfScalar(u8, inner, '\\') == null) {
                return inner;
            }
            let buf = std.ArrayList(u8).empty;
            errdefer buf.deinit(_.alloc);
            let i: usize = 0;
            while (i < inner.len) {
                if (inner[i] == '\\' and i + 1 < inner.len) {
                    const esc = inner[i + 1];
                    switch (esc) {
                        '"' => try buf.append(_.alloc, '"'),
                        '\\' => try buf.append(_.alloc, '\\'),
                        '/' => try buf.append(_.alloc, '/'),
                        'b' => try buf.append(_.alloc, 0x08),
                        'f' => try buf.append(_.alloc, 0x0C),
                        'n' => try buf.append(_.alloc, '\n'),
                        'r' => try buf.append(_.alloc, '\r'),
                        't' => try buf.append(_.alloc, '\t'),
                        else => try buf.append(_.alloc, esc),
                    }
                    i += 2;
                } else {
                    try buf.append(_.alloc, inner[i]);
                    i += 1;
                }
            }
            const owned = try buf.toOwnedSlice(_.alloc);
            return _.addOwnedString(owned);
        }
        // BlockStringValue: """..."""
        if (tok.lexeme.len < 6) return error.UnexpectedToken;
        const raw = tok.lexeme[3 .. tok.lexeme.len - 3];
        const owned = try dedentBlockString(_.alloc, raw);
        return _.addOwnedString(owned);
    }

    fn parseValue(*_, is_const: bool): GqlError!GqlValue {
        const tok = _.peek();
        switch (tok.kind) {
            .Dollar => {
                if (is_const) return error.UnexpectedToken;
                * = _.advance();
                const name_tok = try _.expect(.Name);
                return GqlValue{ .Variable = name_tok.lexeme };
            },
            .IntValue => {
                * = _.advance();
                const val = std.fmt.parseInt(i64, tok.lexeme, 10) catch return error.InvalidNumber;
                return GqlValue{ .Int = val };
            },
            .FloatValue => {
                * = _.advance();
                const val = std.fmt.parseFloat(f64, tok.lexeme) catch return error.InvalidNumber;
                return GqlValue{ .Float = val };
            },
            .StringValue, .BlockStringValue => {
                * = _.advance();
                const s = try _.decodeString(tok);
                return GqlValue{ .String = s };
            },
            .Name => {
                * = _.advance();
                if (std.mem.eql(u8, tok.lexeme, "true")) {
                    return GqlValue{ .Boolean = true };
                }
                if (std.mem.eql(u8, tok.lexeme, "false")) {
                    return GqlValue{ .Boolean = false };
                }
                if (std.mem.eql(u8, tok.lexeme, "null")) {
                    return GqlValue{ .Null = {} };
                }
                return GqlValue{ .Enum = tok.lexeme };
            },
            .BracketL => {
                if (_.depth >= _.max_depth) return error.MaxDepthExceeded;
                _.depth += 1;
                defer _.depth -= 1;

                * = _.advance();
                let list = std.ArrayList(GqlValue).empty;
                errdefer {
                    for (*item of list.items) freeValue(item, _.alloc);
                    list.deinit(_.alloc);
                }
                while (_.peekKind() != .BracketR) {
                    if (_.peekKind() == .Eof) return error.UnexpectedToken;
                    const item = try _.parseValue(is_const);
                    try list.append(_.alloc, item);
                }
                * = try _.expect(.BracketR);
                return GqlValue{ .List = try list.toOwnedSlice(_.alloc) };
            },
            .BraceL => {
                if (_.depth >= _.max_depth) return error.MaxDepthExceeded;
                _.depth += 1;
                defer _.depth -= 1;

                * = _.advance();
                let fields = std.ArrayList(GqlObjectField).empty;
                errdefer {
                    for (*f of fields.items) freeValue(&f.value, _.alloc);
                    fields.deinit(_.alloc);
                }
                while (_.peekKind() != .BraceR) {
                    if (_.peekKind() == .Eof) return error.UnexpectedToken;
                    const fname = try _.expect(.Name);
                    * = try _.expect(.Colon);
                    const fval = try _.parseValue(is_const);
                    try fields.append(_.alloc, .{ .name = fname.lexeme, .value = fval });
                }
                * = try _.expect(.BraceR);
                return GqlValue{ .Object = try fields.toOwnedSlice(_.alloc) };
            },
            else => return error.UnexpectedToken,
        }
    }

    fn parseTypeRef(*_): GqlError!GqlTypeRef {
        let base: GqlTypeRef = undefined;
        if (_.skip(.BracketL)) {
            let inner = try _.alloc.create(GqlTypeRef);
            errdefer _.alloc.destroy(inner);
            inner.* = try _.parseTypeRef();
            * = try _.expect(.BracketR);
            base = GqlTypeRef{ .List = inner };
        } else {
            const name_tok = try _.expect(.Name);
            base = GqlTypeRef{ .Named = name_tok.lexeme };
        }
        if (_.skip(.Bang)) {
            let inner = try _.alloc.create(GqlTypeRef);
            errdefer {
                freeTypeRef(&base, _.alloc);
                _.alloc.destroy(inner);
            }
            inner.* = base;
            return GqlTypeRef{ .NonNull = inner };
        }
        return base;
    }

    fn parseArguments(*_): GqlError![]GqlArgument {
        if (!_.skip(.ParenL)) {
            return _.alloc.alloc(GqlArgument, 0);
        }
        let args = std.ArrayList(GqlArgument).empty;
        errdefer {
            for (*a of args.items) freeValue(&a.value, _.alloc);
            args.deinit(_.alloc);
        }
        while (_.peekKind() != .ParenR) {
            if (_.peekKind() == .Eof) return error.UnexpectedToken;
            const name_tok = try _.expect(.Name);
            * = try _.expect(.Colon);
            const val = try _.parseValue(false);
            try args.append(_.alloc, .{ .name = name_tok.lexeme, .value = val });
        }
        * = try _.expect(.ParenR);
        return args.toOwnedSlice(_.alloc);
    }

    fn parseDirectives(*_): GqlError![]GqlDirective {
        let dirs = std.ArrayList(GqlDirective).empty;
        errdefer {
            for (*d of dirs.items) freeArguments(d.arguments, _.alloc);
            dirs.deinit(_.alloc);
        }
        while (_.skip(.At)) {
            const name_tok = try _.expect(.Name);
            const args = try _.parseArguments();
            try dirs.append(_.alloc, .{ .name = name_tok.lexeme, .arguments = args });
        }
        return dirs.toOwnedSlice(_.alloc);
    }

    fn parseVariableDefs(*_): GqlError![]GqlVariableDef {
        if (!_.skip(.ParenL)) {
            return _.alloc.alloc(GqlVariableDef, 0);
        }
        let defs = std.ArrayList(GqlVariableDef).empty;
        errdefer {
            for (*vd of defs.items) {
                freeTypeRef(&vd.type_ref, _.alloc);
                if (*dv := vd.default_value) freeValue(dv, _.alloc);
            }
            defs.deinit(_.alloc);
        }
        while (_.peekKind() != .ParenR) {
            if (_.peekKind() == .Eof) return error.UnexpectedToken;
            * = try _.expect(.Dollar);
            const name_tok = try _.expect(.Name);
            * = try _.expect(.Colon);
            let tref = try _.parseTypeRef();
            errdefer freeTypeRef(&tref, _.alloc);
            let def_val: GqlValue? = null;
            if (_.skip(.Equals)) {
                def_val = try _.parseValue(true);
            }
            try defs.append(_.alloc, .{
                .name = name_tok.lexeme,
                .type_ref = tref,
                .default_value = def_val,
            });
        }
        * = try _.expect(.ParenR);
        return defs.toOwnedSlice(_.alloc);
    }

    fn parseSelectionSet(*_): GqlError![]GqlSelection {
        if (_.depth >= _.max_depth) {
            return error.MaxDepthExceeded;
        }
        _.depth += 1;
        defer _.depth -= 1;

        * = try _.expect(.BraceL);
        let sels = std.ArrayList(GqlSelection).empty;
        while (_.peekKind() != .BraceR) {
            if (_.peekKind() == .Eof) {
                freeSelectionSlice(sels.items, _.alloc);
                sels.deinit(_.alloc);
                return error.UnexpectedToken;
            }
            const sel = _.parseSelection() catch |e| {
                freeSelectionSlice(sels.items, _.alloc);
                sels.deinit(_.alloc);
                return e;
            };
            sels.append(_.alloc, sel) catch |e| {
                let tmp = sel;
                freeOneSelection(&tmp, _.alloc);
                freeSelectionSlice(sels.items, _.alloc);
                sels.deinit(_.alloc);
                return e;
            };
        }
        * = try _.expect(.BraceR);
        return sels.toOwnedSlice(_.alloc);
    }

    fn parseSelection(*_): GqlError!GqlSelection {
        if (_.skip(.Spread)) {
            const next = _.peek();
            if (next.kind == .Name and !std.mem.eql(u8, next.lexeme, "on")) {
                * = _.advance();
                const dirs = try _.parseDirectives();
                return GqlSelection{
                    .FragmentSpread = .{
                        .name = next.lexeme,
                        .directives = dirs,
                    },
                };
            }
            let type_cond: ([]const u8)? = null;
            if (next.kind == .Name and std.mem.eql(u8, next.lexeme, "on")) {
                * = _.advance();
                const tc_tok = try _.expect(.Name);
                type_cond = tc_tok.lexeme;
            }
            const dirs = try _.parseDirectives();
            errdefer freeDirectives(dirs, _.alloc);
            const sel_set = try _.parseSelectionSet();
            return GqlSelection{
                .InlineFragment = .{
                    .type_condition = type_cond,
                    .directives = dirs,
                    .selection_set = sel_set,
                },
            };
        }

        const first = try _.expect(.Name);
        let alias: ([]const u8)? = null;
        let name: []const u8 = first.lexeme;
        if (_.skip(.Colon)) {
            alias = first.lexeme;
            const name_tok = try _.expect(.Name);
            name = name_tok.lexeme;
        }
        const args = try _.parseArguments();
        errdefer freeArguments(args, _.alloc);
        const dirs = try _.parseDirectives();
        errdefer freeDirectives(dirs, _.alloc);

        let sel_set: []GqlSelection = undefined;
        if (_.peekKind() == .BraceL) {
            sel_set = try _.parseSelectionSet();
        } else {
            sel_set = try _.alloc.alloc(GqlSelection, 0);
        }
        return GqlSelection{
            .Field = .{
                .alias = alias,
                .name = name,
                .arguments = args,
                .directives = dirs,
                .selection_set = sel_set,
            },
        };
    }

    fn parseOperation(*_): GqlError!GqlOperation {
        if (_.peekKind() == .BraceL) {
            const sel_set = try _.parseSelectionSet();
            const empty_vdefs = try _.alloc.alloc(GqlVariableDef, 0);
            errdefer freeVariableDefs(empty_vdefs, _.alloc);
            const empty_dirs = try _.alloc.alloc(GqlDirective, 0);
            return GqlOperation{
                .op_type = .Query,
                .name = null,
                .variable_defs = empty_vdefs,
                .directives = empty_dirs,
                .selection_set = sel_set,
            };
        }

        const op_tok = try _.expect(.Name);
        let op_type: GqlOperationType = .Query;
        if (std.mem.eql(u8, op_tok.lexeme, "query")) {
            op_type = .Query;
        } else if (std.mem.eql(u8, op_tok.lexeme, "mutation")) {
            op_type = .Mutation;
        } else if (std.mem.eql(u8, op_tok.lexeme, "subscription")) {
            op_type = .Subscription;
        } else {
            return error.UnexpectedToken;
        }

        let name: ([]const u8)? = null;
        if (_.peekKind() == .Name) {
            name = _.advance().lexeme;
        }
        const vdefs = try _.parseVariableDefs();
        errdefer freeVariableDefs(vdefs, _.alloc);
        const dirs = try _.parseDirectives();
        errdefer freeDirectives(dirs, _.alloc);
        const sel_set = try _.parseSelectionSet();
        return GqlOperation{
            .op_type = op_type,
            .name = name,
            .variable_defs = vdefs,
            .directives = dirs,
            .selection_set = sel_set,
        };
    }

    fn parseFragment(*_): GqlError!GqlFragment {
        * = try _.expectKeyword("fragment");
        const name_tok = try _.expect(.Name);
        if (std.mem.eql(u8, name_tok.lexeme, "on")) {
            return error.UnexpectedToken;
        }
        * = try _.expectKeyword("on");
        const tc_tok = try _.expect(.Name);
        const dirs = try _.parseDirectives();
        errdefer freeDirectives(dirs, _.alloc);
        const sel_set = try _.parseSelectionSet();
        return GqlFragment{
            .name = name_tok.lexeme,
            .type_condition = tc_tok.lexeme,
            .directives = dirs,
            .selection_set = sel_set,
        };
    }
}

fn freeOneSelection(sel: *GqlSelection, alloc: std.mem.Allocator) {
    switch (sel.*) {
        .Field => |*f| {
            freeArguments(f.arguments, alloc);
            freeDirectives(f.directives, alloc);
            freeSelectionSet(f.selection_set, alloc);
        },
        .FragmentSpread => |*fs| freeDirectives(fs.directives, alloc),
        .InlineFragment => |*inf| {
            freeDirectives(inf.directives, alloc);
            freeSelectionSet(inf.selection_set, alloc);
        },
    }
}

fn freeSelectionSlice(sels: []GqlSelection, alloc: std.mem.Allocator) {
    for (*sel of sels) {
        freeOneSelection(sel, alloc);
    }
}

fn dedentBlockString(alloc: std.mem.Allocator, raw: []const u8): GqlError![]u8 {
    let lines = std.ArrayList(string).empty;
    defer lines.deinit(alloc);

    let i: usize = 0;
    let line_start: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\r' or raw[i] == '\n') {
            try lines.append(alloc, raw[line_start..i]);
            if (raw[i] == '\r' and i + 1 < raw.len and raw[i + 1] == '\n') {
                i += 2;
            } else {
                i += 1;
            }
            line_start = i;
        } else {
            i += 1;
        }
    }
    try lines.append(alloc, raw[line_start..raw.len]);

    let common_indent: usize? = null;
    if (lines.items.len > 1) {
        let idx: usize = 1;
        while (idx < lines.items.len) {
            const l = lines.items[idx];
            let indent: usize = 0;
            while (indent < l.len and (l[indent] == ' ' or l[indent] == '\t')) {
                indent += 1;
            }
            if (indent < l.len) {
                if (ci := common_indent) {
                    if (indent < ci) common_indent = indent;
                } else {
                    common_indent = indent;
                }
            }
            idx += 1;
        }
    }

    const ci = common_indent orelse 0;

    let first_non_empty: usize = 0;
    while (first_non_empty < lines.items.len and isBlankLine(lines.items[first_non_empty])) {
        first_non_empty += 1;
    }
    let last_non_empty: usize = lines.items.len;
    while (last_non_empty > first_non_empty and isBlankLine(lines.items[last_non_empty - 1])) {
        last_non_empty -= 1;
    }

    let out = std.ArrayList(u8).empty;
    errdefer bufFree(&out, alloc);

    let j: usize = first_non_empty;
    while (j < last_non_empty) {
        if (j > first_non_empty) {
            try out.append(alloc, '\n');
        }
        const l = lines.items[j];
        if (j == 0) {
            try out.appendSlice(alloc, l);
        } else if (l.len > ci) {
            try out.appendSlice(alloc, l[ci..]);
        }
        j += 1;
    }
    return out.toOwnedSlice(alloc);
}

fn bufFree(buf: *std.ArrayList(u8), alloc: std.mem.Allocator) {
    buf.deinit(alloc);
}

fn isBlankLine(l: []const u8): bool {
    for (c of l) {
        if (c != ' ' and c != '\t') return false;
    }
    return true;
}

pub fn parseDocument(alloc: std.mem.Allocator, source: []const u8, max_depth: usize): GqlError!GqlDocument {
    const tokens = try tokenize(alloc, source);
    defer alloc.free(tokens);

    const effective_max = if (max_depth == 0) DEFAULT_MAX_DEPTH else max_depth;

    let parser = Parser{
        .alloc = alloc,
        .tokens = tokens,
        .pos = 0,
        .depth = 0,
        .max_depth = effective_max,
        .owned_strings = std.ArrayList([]u8).empty,
    };
    errdefer {
        for (s of parser.owned_strings.items) alloc.free(s);
        parser.owned_strings.deinit(alloc);
    }

    let ops = std.ArrayList(GqlOperation).empty;
    errdefer {
        for (*op of ops.items) freeOperation(op, alloc);
        ops.deinit(alloc);
    }

    let frags = std.ArrayList(GqlFragment).empty;
    errdefer {
        for (*fr of frags.items) freeFragment(fr, alloc);
        frags.deinit(alloc);
    }

    while (parser.peekKind() != .Eof) {
        const tok = parser.peek();
        if (tok.kind == .BraceL) {
            const op = try parser.parseOperation();
            try ops.append(alloc, op);
        } else if (tok.kind == .Name) {
            if (std.mem.eql(u8, tok.lexeme, "fragment")) {
                const frag = try parser.parseFragment();
                try frags.append(alloc, frag);
            } else {
                const op = try parser.parseOperation();
                try ops.append(alloc, op);
            }
        } else {
            return error.UnexpectedToken;
        }
    }

    return GqlDocument{
        .operations = try ops.toOwnedSlice(alloc),
        .fragments = try frags.toOwnedSlice(alloc),
        .owned_strings = try parser.owned_strings.toOwnedSlice(alloc),
    };
}
