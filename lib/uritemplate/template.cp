// Source: cp/lib/uritemplate/template.cp (RFC 6570 Level 1-4 URI Template Parser, Expander & Variable Extractor)
// Pure CP zero-C-dependency RFC 6570 URI Template engine for REST API clients,
// OpenAPI/MCP dynamic endpoints, and route parameter extraction.

const std = @import("std");

/// Errors returned during URI template parsing, variable binding, or expansion.
pub err UriTemplateError {
    UnclosedExpression,
    UnexpectedCloseBrace,
    InvalidOperator,
    InvalidVarName,
    InvalidModifier,
    OutOfMemory,
}

/// RFC 6570 expression operators (Levels 1 through 4).
pub enum TemplateOp {
    Simple,
    Reserved,
    Fragment,
    Label,
    Path,
    PathParam,
    Query,
    QueryCont,

    fn first(): []const u8 {
        return switch (_) {
            .Simple => "",
            .Reserved => "",
            .Fragment => "#",
            .Label => ".",
            .Path => "/",
            .PathParam => ";",
            .Query => "?",
            .QueryCont => "&",
        };
    }

    fn sep(): []const u8 {
        return switch (_) {
            .Simple => ",",
            .Reserved => ",",
            .Fragment => ",",
            .Label => ".",
            .Path => "/",
            .PathParam => ";",
            .Query => "&",
            .QueryCont => "&",
        };
    }

    fn isNamed(): bool {
        return switch (_) {
            .PathParam | .Query | .QueryCont => true,
            else => false,
        };
    }

    fn ifEmpty(): []const u8 {
        return switch (_) {
            .Query | .QueryCont => "=",
            else => "",
        };
    }

    fn allowReserved(): bool {
        return switch (_) {
            .Reserved | .Fragment => true,
            else => false,
        };
    }
}

/// Key-value pair for associative map template variables.
pub struct AssocPair {
    key: []const u8;
    value: []const u8;
}

const VarKind = enum {
    Scalar,
    List,
    Assoc,
};

pub type OwnedBytes = []u8;

struct VarEntry {
    name: []u8;
    kind: VarKind;
    scalar: []u8 = [];
    list: [][]u8 = [];
    assoc: []AssocPairOwned = [];
}

struct AssocPairOwned {
    key: []u8;
    value: []u8;
}

/// Variable bindings container supporting scalar strings, string lists, and associative maps.
pub struct TemplateVars {
    alloc: std.mem.Allocator;
    entries: std.ArrayList(VarEntry) = .empty;

    /// Initialize an empty variable binding container.
    pub fn init(alloc: std.mem.Allocator): TemplateVars {
        return TemplateVars{
            alloc: alloc,
            entries: .empty,
        };
    }

    /// Release all owned variable names and values.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            _.freeEntry(&_.entries.items[i]);
        }
        _.entries.deinit(_.alloc);
    }

    fn freeEntry(*_, entry: *VarEntry) {
        _.alloc.free(entry.name);
        switch (entry.kind) {
            .Scalar => {
                if (entry.scalar.len > 0) _.alloc.free(entry.scalar);
            },
            .List => {
                for (item of entry.list) {
                    _.alloc.free(item);
                }
                if (entry.list.len > 0) _.alloc.free(entry.list);
            },
            .Assoc => {
                for (pair of entry.assoc) {
                    _.alloc.free(pair.key);
                    _.alloc.free(pair.value);
                }
                if (entry.assoc.len > 0) _.alloc.free(entry.assoc);
            },
        }
    }

    fn findIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.entries.items[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    fn getEntry(name: []const u8): (&VarEntry)? {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.entries.items[i].name, name)) {
                return &_.entries.items[i];
            }
        }
        return null;
    }

    /// Bind a scalar string value to a variable name (overwrites any previous binding).
    pub fn set(*_, name: []const u8, value: []const u8): void ! UriTemplateError {
        const val_copy = _.alloc.dupe(u8, value) catch return UriTemplateError.OutOfMemory;
        errdefer _.alloc.free(val_copy);

        if (idx := _.findIndex(name)) {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            _.freeEntry(&_.entries.items[idx]);
            _.entries.items[idx] = VarEntry{
                name: name_copy,
                kind: .Scalar,
                scalar: val_copy,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, VarEntry{
                name: name_copy,
                kind: .Scalar,
                scalar: val_copy,
            }) catch return UriTemplateError.OutOfMemory;
        }
    }

    /// Bind a list of string values to a variable name (overwrites any previous binding).
    pub fn setList(*_, name: []const u8, items: []const []const u8): void ! UriTemplateError {
        let copied = _.alloc.alloc(OwnedBytes, items.len) catch return UriTemplateError.OutOfMemory;
        let built: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < built) : (k += 1) {
                _.alloc.free(copied[k]);
            }
            _.alloc.free(copied);
        }

        while (built < items.len) : (built += 1) {
            copied[built] = _.alloc.dupe(u8, items[built]) catch return UriTemplateError.OutOfMemory;
        }

        if (idx := _.findIndex(name)) {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            _.freeEntry(&_.entries.items[idx]);
            _.entries.items[idx] = VarEntry{
                name: name_copy,
                kind: .List,
                list: copied,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, VarEntry{
                name: name_copy,
                kind: .List,
                list: copied,
            }) catch return UriTemplateError.OutOfMemory;
        }
    }

    /// Bind an associative list of key-value pairs to a variable name (overwrites any previous binding).
    pub fn setAssoc(*_, name: []const u8, pairs: []const AssocPair): void ! UriTemplateError {
        let copied = _.alloc.alloc(AssocPairOwned, pairs.len) catch return UriTemplateError.OutOfMemory;
        let built: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < built) : (k += 1) {
                _.alloc.free(copied[k].key);
                _.alloc.free(copied[k].value);
            }
            _.alloc.free(copied);
        }

        while (built < pairs.len) : (built += 1) {
            const k_copy = _.alloc.dupe(u8, pairs[built].key) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(k_copy);
            const v_copy = _.alloc.dupe(u8, pairs[built].value) catch return UriTemplateError.OutOfMemory;
            copied[built] = AssocPairOwned{
                key: k_copy,
                value: v_copy,
            };
        }

        if (idx := _.findIndex(name)) {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            _.freeEntry(&_.entries.items[idx]);
            _.entries.items[idx] = VarEntry{
                name: name_copy,
                kind: .Assoc,
                assoc: copied,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, VarEntry{
                name: name_copy,
                kind: .Assoc,
                assoc: copied,
            }) catch return UriTemplateError.OutOfMemory;
        }
    }
}

struct VarSpec {
    name: []u8;
    explode: bool = false;
    max_len: usize? = null;
}

struct ExprPart {
    op: TemplateOp;
    vars: []VarSpec;
}

enum TemplatePart {
    Literal: []u8,
    Expr: ExprPart,
}

/// Parsed RFC 6570 Level 1-4 URI Template.
pub struct UriTemplate {
    alloc: std.mem.Allocator;
    parts: []TemplatePart;

    /// Parse and validate an RFC 6570 URI template pattern.
    pub fn parse(alloc: std.mem.Allocator, pattern: []const u8): UriTemplate ! UriTemplateError {
        let part_list: std.ArrayList(TemplatePart) = .empty;
        errdefer {
            for (part of part_list.items) {
                freeTemplatePart(alloc, part);
            }
            part_list.deinit(alloc);
        }

        let i: usize = 0;
        let lit_start: usize = 0;

        while (i < pattern.len) {
            const ch = pattern[i];
            if (ch == '}') {
                return UriTemplateError.UnexpectedCloseBrace;
            }
            if (ch == '{') {
                if (i > lit_start) {
                    const lit = alloc.dupe(u8, pattern[lit_start..i]) catch return UriTemplateError.OutOfMemory;
                    errdefer alloc.free(lit);
                    part_list.append(alloc, TemplatePart{ .Literal = lit }) catch return UriTemplateError.OutOfMemory;
                }

                const expr_start = i + 1;
                let close_idx: usize? = null;
                let j = expr_start;
                while (j < pattern.len) : (j += 1) {
                    if (pattern[j] == '}') {
                        close_idx = j;
                        break;
                    }
                    if (pattern[j] == '{') {
                        return UriTemplateError.UnclosedExpression;
                    }
                }

                if (close_idx == null) return UriTemplateError.UnclosedExpression;
                const end_idx = close_idx.?;
                const expr_body = pattern[expr_start..end_idx];
                const parsed_expr = parseExpression(alloc, expr_body)?;
                errdefer freeExprPart(alloc, parsed_expr);
                part_list.append(alloc, TemplatePart{ .Expr = parsed_expr }) catch return UriTemplateError.OutOfMemory;

                i = end_idx + 1;
                lit_start = i;
                continue;
            }
            i += 1;
        }

        if (lit_start < pattern.len) {
            const lit = alloc.dupe(u8, pattern[lit_start..pattern.len]) catch return UriTemplateError.OutOfMemory;
            errdefer alloc.free(lit);
            part_list.append(alloc, TemplatePart{ .Literal = lit }) catch return UriTemplateError.OutOfMemory;
        }

        const owned_parts = part_list.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
        return UriTemplate{
            alloc: alloc,
            parts: owned_parts,
        };
    }

    /// Release all memory owned by the parsed URI template.
    pub fn deinit(*_) {
        for (part of _.parts) {
            freeTemplatePart(_.alloc, part);
        }
        if (_.parts.len > 0) {
            _.alloc.free(_.parts);
        }
        _.parts = [];
    }

    /// Extract deduplicated variable names declared in the template in order of first appearance.
    pub fn variables(alloc: std.mem.Allocator): [][]u8 ! UriTemplateError {
        let names: std.ArrayList([]u8) = .empty;
        errdefer {
            for (n of names.items) {
                alloc.free(n);
            }
            names.deinit(alloc);
        }

        for (part of _.parts) {
            switch (part) {
                .Literal => {},
                .Expr => |expr| {
                    for (vs of expr.vars) {
                        let exists = false;
                        for (existing of names.items) {
                            if (std.mem.eql(u8, existing, vs.name)) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            const copy = alloc.dupe(u8, vs.name) catch return UriTemplateError.OutOfMemory;
                            errdefer alloc.free(copy);
                            names.append(alloc, copy) catch return UriTemplateError.OutOfMemory;
                        }
                    }
                },
            }
        }

        return names.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
    }

    /// Expand the URI template using the supplied variable bindings (RFC 6570 Levels 1-4).
    pub fn expand(alloc: std.mem.Allocator, vars: *TemplateVars): []u8 ! UriTemplateError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        for (part of _.parts) {
            switch (part) {
                .Literal => |lit| {
                    out.appendSlice(alloc, lit) catch return UriTemplateError.OutOfMemory;
                },
                .Expr => |expr| {
                    expandExpression(alloc, &out, expr, vars)?;
                },
            }
        }

        return out.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
    }
}

fn freeExprPart(alloc: std.mem.Allocator, expr: ExprPart) {
    for (vs of expr.vars) {
        alloc.free(vs.name);
    }
    if (expr.vars.len > 0) {
        alloc.free(expr.vars);
    }
}

fn freeTemplatePart(alloc: std.mem.Allocator, part: TemplatePart) {
    switch (part) {
        .Literal => |lit| alloc.free(lit),
        .Expr => |expr| freeExprPart(alloc, expr),
    }
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isVarChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_';
}

fn validateVarName(name: []const u8): bool {
    if (name.len == 0) return false;
    if (name[0] == '.' || name[name.len - 1] == '.') return false;

    let i: usize = 0;
    let prev_dot = false;
    while (i < name.len) {
        const c = name[i];
        if (c == '.') {
            if (prev_dot) return false;
            prev_dot = true;
            i += 1;
            continue;
        }
        prev_dot = false;
        if (c == '%') {
            if (i + 2 >= name.len) return false;
            if (!isHexDigit(name[i + 1]) || !isHexDigit(name[i + 2])) return false;
            i += 3;
            continue;
        }
        if (!isVarChar(c)) return false;
        i += 1;
    }
    return true;
}

fn parseVarSpec(alloc: std.mem.Allocator, raw: []const u8): VarSpec ! UriTemplateError {
    if (raw.len == 0) return UriTemplateError.InvalidVarName;

    let explode = false;
    let max_len: usize? = null;
    let name_end = raw.len;

    if (raw[raw.len - 1] == '*') {
        explode = true;
        name_end = raw.len - 1;
        // Reject combined prefix + explode such as {var:3*}
        let k: usize = 0;
        while (k < name_end) : (k += 1) {
            if (raw[k] == ':') return UriTemplateError.InvalidModifier;
        }
    } else {
        let colon_pos: usize? = null;
        let k: usize = 0;
        while (k < raw.len) : (k += 1) {
            if (raw[k] == ':') {
                colon_pos = k;
                break;
            }
        }
        if (cp := colon_pos) {
            const mod_str = raw[cp + 1 .. raw.len];
            if (mod_str.len == 0 || mod_str.len > 4) {
                return UriTemplateError.InvalidModifier;
            }
            if (mod_str[0] < '1' || mod_str[0] > '9') {
                return UriTemplateError.InvalidModifier;
            }
            let parsed_len: usize = 0;
            for (d of mod_str) {
                if (d < '0' || d > '9') return UriTemplateError.InvalidModifier;
                parsed_len = parsed_len * 10 + usize(d - '0');
            }
            max_len = parsed_len;
            name_end = cp;
        }
    }

    const name_slice = raw[0..name_end];
    if (!validateVarName(name_slice)) {
        return UriTemplateError.InvalidVarName;
    }

    const owned_name = alloc.dupe(u8, name_slice) catch return UriTemplateError.OutOfMemory;
    return VarSpec{
        name: owned_name,
        explode: explode,
        max_len: max_len,
    };
}

fn parseExpression(alloc: std.mem.Allocator, body: []const u8): ExprPart ! UriTemplateError {
    if (body.len == 0) return UriTemplateError.InvalidVarName;

    let op = TemplateOp.Simple;
    let rest = body;

    const first = body[0];
    if (first == '+') {
        op = .Reserved;
        rest = body[1..body.len];
    } else if (first == '#') {
        op = .Fragment;
        rest = body[1..body.len];
    } else if (first == '.') {
        op = .Label;
        rest = body[1..body.len];
    } else if (first == '/') {
        op = .Path;
        rest = body[1..body.len];
    } else if (first == ';') {
        op = .PathParam;
        rest = body[1..body.len];
    } else if (first == '?') {
        op = .Query;
        rest = body[1..body.len];
    } else if (first == '&') {
        op = .QueryCont;
        rest = body[1..body.len];
    } else if (first == '=' || first == ',' || first == '!' || first == '@' || first == '|') {
        return UriTemplateError.InvalidOperator;
    }

    if (rest.len == 0) return UriTemplateError.InvalidVarName;

    let specs: std.ArrayList(VarSpec) = .empty;
    errdefer {
        for (vs of specs.items) {
            alloc.free(vs.name);
        }
        specs.deinit(alloc);
    }

    let start: usize = 0;
    let i: usize = 0;
    while (i <= rest.len) : (i += 1) {
        if (i == rest.len || rest[i] == ',') {
            const spec = parseVarSpec(alloc, rest[start..i])?;
            errdefer alloc.free(spec.name);
            specs.append(alloc, spec) catch return UriTemplateError.OutOfMemory;
            start = i + 1;
        }
    }

    const owned_specs = specs.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
    return ExprPart{
        op: op,
        vars: owned_specs,
    };
}

fn isUnreserved(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '.' || c == '_' || c == '~';
}

fn isReserved(c: u8): bool {
    return switch (c) {
        ':', '/', '?', '#', '[', ']', '@',
        '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=' => true,
        else => false,
    };
}

fn appendPctEncoded(alloc: std.mem.Allocator, out: *std.ArrayList(u8), byte: u8): void ! UriTemplateError {
    const hex = "0123456789ABCDEF";
    const buf: [3]u8 = ['%', hex[byte >> 4], hex[byte & 0x0F]];
    out.appendSlice(alloc, buf[0..3]) catch return UriTemplateError.OutOfMemory;
}

fn appendEncoded(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    value: []const u8,
    allow_reserved: bool,
): void ! UriTemplateError {
    let i: usize = 0;
    while (i < value.len) {
        const c = value[i];
        if (isUnreserved(c)) {
            out.append(alloc, c) catch return UriTemplateError.OutOfMemory;
            i += 1;
            continue;
        }
        if (allow_reserved) {
            if (isReserved(c)) {
                out.append(alloc, c) catch return UriTemplateError.OutOfMemory;
                i += 1;
                continue;
            }
            if (c == '%' && i + 2 < value.len && isHexDigit(value[i + 1]) && isHexDigit(value[i + 2])) {
                out.appendSlice(alloc, value[i .. i + 3]) catch return UriTemplateError.OutOfMemory;
                i += 3;
                continue;
            }
        }
        appendPctEncoded(alloc, out, c)?;
        i += 1;
    }
}

/// Truncate UTF-8 string to at most `max_chars` Unicode codepoints (RFC 6570 Section 2.4.1).
fn truncateUtf8Prefix(s: []const u8, max_chars: usize): []const u8 {
    let byte_idx: usize = 0;
    let char_count: usize = 0;
    while (byte_idx < s.len && char_count < max_chars) {
        const b = s[byte_idx];
        let step: usize = 1;
        if (b < 0x80) {
            step = 1;
        } else if ((b & 0xE0) == 0xC0) {
            step = 2;
        } else if ((b & 0xF0) == 0xE0) {
            step = 3;
        } else if ((b & 0xF8) == 0xF0) {
            step = 4;
        }
        if (byte_idx + step > s.len) {
            byte_idx = s.len;
            break;
        }
        byte_idx += step;
        char_count += 1;
    }
    return s[0..byte_idx];
}

fn expandExpression(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    expr: ExprPart,
    vars: *TemplateVars,
): void ! UriTemplateError {
    const op = expr.op;
    const allow_res = op.allowReserved();
    const named = op.isNamed();
    let is_first = true;

    for (vs of expr.vars) {
        const entry = vars.getEntry(vs.name) ?? continue;

        switch (entry.kind) {
            .Scalar => {
                let raw_val: []const u8 = entry.scalar;
                if (ml := vs.max_len) {
                    raw_val = truncateUtf8Prefix(entry.scalar, ml);
                }
                if (is_first) {
                    out.appendSlice(alloc, op.first()) catch return UriTemplateError.OutOfMemory;
                    is_first = false;
                } else {
                    out.appendSlice(alloc, op.sep()) catch return UriTemplateError.OutOfMemory;
                }
                if (named) {
                    out.appendSlice(alloc, vs.name) catch return UriTemplateError.OutOfMemory;
                    if (raw_val.len == 0) {
                        out.appendSlice(alloc, op.ifEmpty()) catch return UriTemplateError.OutOfMemory;
                    } else {
                        out.append(alloc, '=') catch return UriTemplateError.OutOfMemory;
                        appendEncoded(alloc, out, raw_val, allow_res)?;
                    }
                } else {
                    appendEncoded(alloc, out, raw_val, allow_res)?;
                }
            },
            .List => {
                if (entry.list.len == 0) continue;
                if (is_first) {
                    out.appendSlice(alloc, op.first()) catch return UriTemplateError.OutOfMemory;
                    is_first = false;
                } else {
                    out.appendSlice(alloc, op.sep()) catch return UriTemplateError.OutOfMemory;
                }

                if (!vs.explode) {
                    if (named) {
                        out.appendSlice(alloc, vs.name) catch return UriTemplateError.OutOfMemory;
                        out.append(alloc, '=') catch return UriTemplateError.OutOfMemory;
                    }
                    let idx: usize = 0;
                    while (idx < entry.list.len) : (idx += 1) {
                        if (idx > 0) {
                            out.append(alloc, ',') catch return UriTemplateError.OutOfMemory;
                        }
                        appendEncoded(alloc, out, entry.list[idx], allow_res)?;
                    }
                } else {
                    let idx: usize = 0;
                    while (idx < entry.list.len) : (idx += 1) {
                        if (idx > 0) {
                            out.appendSlice(alloc, op.sep()) catch return UriTemplateError.OutOfMemory;
                        }
                        const item = entry.list[idx];
                        if (named) {
                            out.appendSlice(alloc, vs.name) catch return UriTemplateError.OutOfMemory;
                            if (item.len == 0) {
                                out.appendSlice(alloc, op.ifEmpty()) catch return UriTemplateError.OutOfMemory;
                            } else {
                                out.append(alloc, '=') catch return UriTemplateError.OutOfMemory;
                                appendEncoded(alloc, out, item, allow_res)?;
                            }
                        } else {
                            appendEncoded(alloc, out, item, allow_res)?;
                        }
                    }
                }
            },
            .Assoc => {
                if (entry.assoc.len == 0) continue;
                if (is_first) {
                    out.appendSlice(alloc, op.first()) catch return UriTemplateError.OutOfMemory;
                    is_first = false;
                } else {
                    out.appendSlice(alloc, op.sep()) catch return UriTemplateError.OutOfMemory;
                }

                if (!vs.explode) {
                    if (named) {
                        out.appendSlice(alloc, vs.name) catch return UriTemplateError.OutOfMemory;
                        out.append(alloc, '=') catch return UriTemplateError.OutOfMemory;
                    }
                    let idx: usize = 0;
                    while (idx < entry.assoc.len) : (idx += 1) {
                        if (idx > 0) {
                            out.append(alloc, ',') catch return UriTemplateError.OutOfMemory;
                        }
                        appendEncoded(alloc, out, entry.assoc[idx].key, allow_res)?;
                        out.append(alloc, ',') catch return UriTemplateError.OutOfMemory;
                        appendEncoded(alloc, out, entry.assoc[idx].value, allow_res)?;
                    }
                } else {
                    let idx: usize = 0;
                    while (idx < entry.assoc.len) : (idx += 1) {
                        if (idx > 0) {
                            out.appendSlice(alloc, op.sep()) catch return UriTemplateError.OutOfMemory;
                        }
                        const pair = entry.assoc[idx];
                        appendEncoded(alloc, out, pair.key, allow_res)?;
                        if (named && pair.value.len == 0) {
                            out.appendSlice(alloc, op.ifEmpty()) catch return UriTemplateError.OutOfMemory;
                        } else {
                            out.append(alloc, '=') catch return UriTemplateError.OutOfMemory;
                            appendEncoded(alloc, out, pair.value, allow_res)?;
                        }
                    }
                }
            },
        }
    }
}
