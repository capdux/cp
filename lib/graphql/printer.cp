// Source: cp/lib/graphql/printer.cp (GraphQL AST Pretty & Minify Printer)
// Pure CP GraphQL October 2021 AST canonical formatter (Pretty) and single-line minifier (Minify)
// supporting lossless parse -> print -> parse and parse -> minify -> parse round-trips.

const std = @import("std");
import { GqlError } from "./lexer.cp";
import {
    GqlValue,
    GqlArgument,
    GqlDirective,
    GqlType,
    GqlVarDef,
    GqlSelection,
    GqlOperation,
    GqlFragmentDef,
    GqlDocument,
} from "./parser.cp";

/// Formatting mode for `GqlPrinter`.
pub enum GqlPrintMode {
    Pretty,
    Minify,
}

/// Canonical GraphQL AST printer supporting Pretty (multi-line indented)
/// and Minify (single-line token-minimal) output with lossless round-trips.
pub struct GqlPrinter {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    mode: GqlPrintMode = .Pretty,
    last_was_word_or_num: bool = false,
    last_was_string: bool = false,

    /// Formats a `GqlDocument` into canonical multi-line GraphQL source text.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub static fn print(alloc: std.mem.Allocator, doc: GqlDocument): ([]u8) ! GqlError {
        return GqlPrinter.printWithMode(alloc, doc, .Pretty);
    }

    /// Formats a `GqlDocument` into canonical 2-space indented GraphQL source text.
    pub static fn printPretty(alloc: std.mem.Allocator, doc: GqlDocument): ([]u8) ! GqlError {
        return GqlPrinter.printWithMode(alloc, doc, .Pretty);
    }

    /// Formats a `GqlDocument` into a compact single-line GraphQL string without redundant whitespace.
    pub static fn minify(alloc: std.mem.Allocator, doc: GqlDocument): ([]u8) ! GqlError {
        return GqlPrinter.printWithMode(alloc, doc, .Minify);
    }

    /// Formats a `GqlDocument` using the specified `GqlPrintMode`.
    pub static fn printWithMode(alloc: std.mem.Allocator, doc: GqlDocument, mode: GqlPrintMode): ([]u8) ! GqlError {
        let p = GqlPrinter{
            allocator: alloc,
            buf: .empty,
            mode: mode,
            last_was_word_or_num: false,
            last_was_string: false,
        };
        errdefer p.buf.deinit(alloc);

        p.writeDocument(doc)?;
        return p.buf.toOwnedSlice(alloc) catch return GqlError.OutOfMemory;
    }

    fn writeRawStr(*_, s: string): void ! GqlError {
        _.buf.appendSlice(_.allocator, s) catch return GqlError.OutOfMemory;
    }

    fn writeRawByte(*_, b: u8): void ! GqlError {
        _.buf.append(_.allocator, b) catch return GqlError.OutOfMemory;
    }

    fn writePunct(*_, s: string): void ! GqlError {
        _.writeRawStr(s)?;
        _.last_was_word_or_num = false;
        _.last_was_string = false;
    }

    fn writePunctByte(*_, b: u8): void ! GqlError {
        _.writeRawByte(b)?;
        _.last_was_word_or_num = false;
        _.last_was_string = false;
    }

    fn writeSpace(*_): void ! GqlError {
        _.writeRawByte(' ')?;
        _.last_was_word_or_num = false;
        _.last_was_string = false;
    }

    fn writeWordOrNum(*_, s: string): void ! GqlError {
        if (_.mode == .Minify && _.last_was_word_or_num) {
            _.writeRawByte(' ')?;
        }
        _.writeRawStr(s)?;
        _.last_was_word_or_num = true;
        _.last_was_string = false;
    }

    fn writeIndent(*_, level: usize): void ! GqlError {
        let i: usize = 0;
        while (i < level * 2) : (i += 1) {
            _.writeRawByte(' ')?;
        }
        _.last_was_word_or_num = false;
        _.last_was_string = false;
    }

    fn writeDocument(*_, doc: GqlDocument): void ! GqlError {
        let first_def = true;

        for (op of doc.operations) {
            if (!first_def && _.mode == .Pretty) {
                _.writePunct("\n\n")?;
            }
            first_def = false;
            _.writeOperation(op)?;
        }

        for (frag of doc.fragments) {
            if (!first_def && _.mode == .Pretty) {
                _.writePunct("\n\n")?;
            }
            first_def = false;
            _.writeFragmentDef(frag)?;
        }
    }

    fn writeOperation(*_, op: GqlOperation): void ! GqlError {
        const is_shorthand = op.shorthand &&
            op.op == .Query &&
            op.name == null &&
            op.var_defs.len == 0 &&
            op.directives.len == 0;

        if (!is_shorthand) {
            _.writeWordOrNum(op.op.keyword())?;
            if (op_name := op.name) {
                if (_.mode == .Pretty) _.writeSpace()?;
                _.writeWordOrNum(op_name)?;
            }
            if (op.var_defs.len > 0) {
                _.writePunctByte('(')?;
                let i: usize = 0;
                while (i < op.var_defs.len) : (i += 1) {
                    if (i > 0 && _.mode == .Pretty) _.writePunct(", ")?;
                    _.writeVarDef(op.var_defs[i])?;
                }
                _.writePunctByte(')')?;
            }
            _.writeDirectives(op.directives)?;
            if (_.mode == .Pretty) _.writeSpace()?;
        }

        _.writeSelectionSet(op.selections, 0)?;
    }

    fn writeFragmentDef(*_, frag: GqlFragmentDef): void ! GqlError {
        _.writeWordOrNum("fragment")?;
        if (_.mode == .Pretty) _.writeSpace()?;
        _.writeWordOrNum(frag.name)?;
        if (_.mode == .Pretty) _.writeSpace()?;
        _.writeWordOrNum("on")?;
        if (_.mode == .Pretty) _.writeSpace()?;
        _.writeWordOrNum(frag.type_condition)?;
        _.writeDirectives(frag.directives)?;
        if (_.mode == .Pretty) _.writeSpace()?;
        _.writeSelectionSet(frag.selections, 0)?;
    }

    fn writeVarDef(*_, vd: GqlVarDef): void ! GqlError {
        _.writePunctByte('$')?;
        _.writeWordOrNum(vd.name)?;
        _.writePunctByte(':')?;
        if (_.mode == .Pretty) _.writeSpace()?;
        _.writeType(vd.var_type)?;
        if (dv := vd.default_value) {
            if (_.mode == .Pretty) {
                _.writePunct(" = ")?;
            } else {
                _.writePunctByte('=')?;
            }
            _.writeValue(dv)?;
        }
        _.writeDirectives(vd.directives)?;
    }

    fn writeType(*_, t: GqlType): void ! GqlError {
        switch (t) {
            .Named n => _.writeWordOrNum(n)?,
            .List inner => {
                _.writePunctByte('[')?;
                if (inner.len > 0) _.writeType(inner[0])?;
                _.writePunctByte(']')?;
            },
            .NonNull inner => {
                if (inner.len > 0) _.writeType(inner[0])?;
                _.writePunctByte('!')?;
            },
        }
    }

    fn writeDirectives(*_, dirs: []GqlDirective): void ! GqlError {
        for (d of dirs) {
            if (_.mode == .Pretty) _.writeSpace()?;
            _.writePunctByte('@')?;
            _.writeWordOrNum(d.name)?;
            if (d.args.len > 0) {
                _.writeArguments(d.args)?;
            }
        }
    }

    fn writeArguments(*_, args: []GqlArgument): void ! GqlError {
        _.writePunctByte('(')?;
        let i: usize = 0;
        while (i < args.len) : (i += 1) {
            if (i > 0 && _.mode == .Pretty) _.writePunct(", ")?;
            _.writeWordOrNum(args[i].name)?;
            _.writePunctByte(':')?;
            if (_.mode == .Pretty) _.writeSpace()?;
            _.writeValue(args[i].value)?;
        }
        _.writePunctByte(')')?;
    }

    fn writeSelectionSet(*_, sels: []GqlSelection, indent: usize): void ! GqlError {
        if (_.mode == .Pretty) {
            _.writePunct("{\n")?;
        } else {
            _.writePunctByte('{')?;
        }

        for (sel of sels) {
            if (_.mode == .Pretty) _.writeIndent(indent + 1)?;
            switch (sel) {
                .Field f => {
                    if (al := f.alias) {
                        _.writeWordOrNum(al)?;
                        _.writePunctByte(':')?;
                        if (_.mode == .Pretty) _.writeSpace()?;
                    }
                    _.writeWordOrNum(f.name)?;
                    if (f.args.len > 0) {
                        _.writeArguments(f.args)?;
                    }
                    _.writeDirectives(f.directives)?;
                    if (f.selections.len > 0) {
                        if (_.mode == .Pretty) _.writeSpace()?;
                        _.writeSelectionSet(f.selections, indent + 1)?;
                    }
                },
                .FragmentSpread fs => {
                    _.writePunct("...")?;
                    _.writeWordOrNum(fs.name)?;
                    _.writeDirectives(fs.directives)?;
                },
                .InlineFragment inf => {
                    _.writePunct("...")?;
                    if (tc := inf.type_condition) {
                        if (_.mode == .Pretty) _.writeSpace()?;
                        _.writeWordOrNum("on")?;
                        if (_.mode == .Pretty) _.writeSpace()?;
                        _.writeWordOrNum(tc)?;
                    }
                    _.writeDirectives(inf.directives)?;
                    if (_.mode == .Pretty) _.writeSpace()?;
                    _.writeSelectionSet(inf.selections, indent + 1)?;
                },
            }
            if (_.mode == .Pretty) _.writePunctByte('\n')?;
        }

        if (_.mode == .Pretty) _.writeIndent(indent)?;
        _.writePunctByte('}')?;
    }

    fn writeEscapedString(*_, s: string): void ! GqlError {
        if (_.mode == .Minify && _.last_was_string) {
            _.writeRawByte(' ')?;
        }
        _.writeRawByte('"')?;
        for (c of s) {
            switch (c) {
                '"' => _.writeRawStr("\\\"")?,
                '\\' => _.writeRawStr("\\\\")?,
                0x08 => _.writeRawStr("\\b")?,
                0x0C => _.writeRawStr("\\f")?,
                '\n' => _.writeRawStr("\\n")?,
                '\r' => _.writeRawStr("\\r")?,
                '\t' => _.writeRawStr("\\t")?,
                else => {
                    if (c < 0x20) {
                        const hex = "0123456789ABCDEF";
                        let esc: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
                        _.writeRawStr(&esc)?;
                    } else {
                        _.writeRawByte(c)?;
                    }
                },
            }
        }
        _.writeRawByte('"')?;
        _.last_was_word_or_num = false;
        _.last_was_string = true;
    }

    fn writeValue(*_, v: GqlValue): void ! GqlError {
        switch (v) {
            .Variable name => {
                _.writePunctByte('$')?;
                _.writeWordOrNum(name)?;
            },
            .Int n => {
                let tmp: [64]u8 = [0; 64];
                const rendered = std.fmt.bufPrint(&tmp, "{d}", .{n}) catch return GqlError.OutOfMemory;
                _.writeWordOrNum(rendered)?;
            },
            .Float f => {
                let tmp: [128]u8 = [0; 128];
                const rendered = std.fmt.bufPrint(&tmp, "{d}", .{f}) catch return GqlError.OutOfMemory;
                let has_dot_or_exp = false;
                for (c of rendered) {
                    if (c == '.' || c == 'e' || c == 'E') {
                        has_dot_or_exp = true;
                        break;
                    }
                }
                if (has_dot_or_exp) {
                    _.writeWordOrNum(rendered)?;
                } else {
                    let out_buf: [132]u8 = [0; 132];
                    const full = std.fmt.bufPrint(&out_buf, "{s}.0", .{rendered}) catch return GqlError.OutOfMemory;
                    _.writeWordOrNum(full)?;
                }
            },
            .String s => _.writeEscapedString(s)?,
            .Bool b => _.writeWordOrNum(if (b) "true" else "false")?,
            .Null => _.writeWordOrNum("null")?,
            .Enum e => _.writeWordOrNum(e)?,
            .List items => {
                _.writePunctByte('[')?;
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    if (i > 0 && _.mode == .Pretty) _.writePunct(", ")?;
                    _.writeValue(items[i])?;
                }
                _.writePunctByte(']')?;
            },
            .Object fields => {
                _.writePunctByte('{')?;
                let i: usize = 0;
                while (i < fields.len) : (i += 1) {
                    if (i > 0 && _.mode == .Pretty) _.writePunct(", ")?;
                    _.writeWordOrNum(fields[i].name)?;
                    _.writePunctByte(':')?;
                    if (_.mode == .Pretty) _.writeSpace()?;
                    _.writeValue(fields[i].value)?;
                }
                _.writePunctByte('}')?;
            },
        }
    }
}
