// Reference-hardened against rwf2/multer (MIT) & RFC 7578/6570
// Source: cp/lib/uritemplate/template.cp (RFC 6570 Level 1-4 URI Template Parser, Expander & Variable Extractor)
// Pure CP zero-C-dependency RFC 6570 URI Template engine for REST API clients,
// OpenAPI/MCP dynamic endpoints, and route parameter extraction, with 16-lane SIMD brace scanning
// (`simdUriFindBrace`) and output expansion limit protection (`URI_TEMPLATE_DEFAULT_MAX_OUTPUT`).

const std = @import("std");

/// Module-local 16-lane byte vector alias for URI Template lexical brace scanning.
type UriTplVec16u8 = @Vector(16, u8);

/// Default maximum expanded URI output length (64 KiB) to guard against expansion blowup.
pub const URI_TEMPLATE_DEFAULT_MAX_OUTPUT: usize = 65536;

/// Errors returned during URI template parsing, variable binding, or expansion.
pub err UriTemplateError {
    UnclosedExpression,
    UnexpectedCloseBrace,
    InvalidOperator,
    InvalidVarName,
    InvalidModifier,
    OutputTooLarge,
    OutOfMemory,
}

// ============================================================================
// 16-Lane SIMD Brace Scanner (0-Diff with Scalar Reference)
// ============================================================================

/// Scalar reference: finds the first index of `'{'` or `'}'` in `s`, or `null`.
pub fn scalarUriFindBrace(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '{' || c == '}') return i;
    }
    return null;
}

/// 16-lane SIMD accelerated scanner for `'{'` or `'}'` (wired into `UriTemplate.parse`).
pub fn simdUriFindBrace(s: []const u8): usize? {
    const v_open: UriTplVec16u8 = @splat('{');
    const v_close: UriTplVec16u8 = @splat('}');
    let i: usize = 0;
    while (i + 16 <= s.len) {
        const v: UriTplVec16u8 = s[i .. i + 16][0..16].*;
        const hit = (v == v_open) | (v == v_close);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '{' || c == '}') return i;
    }
    return null;
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

const UriVarKind = enum {
    Scalar,
    List,
    Assoc,
};

pub type OwnedBytes = []u8;

struct UriVarEntry {
    name: []u8;
    kind: UriVarKind;
    scalar: []u8 = [];
    list: [][]u8 = [];
    assoc: []UriAssocPairOwned = [];
}

struct UriAssocPairOwned {
    key: []u8;
    value: []u8;
}

/// Variable bindings container supporting scalar strings, string lists, and associative maps.
pub struct TemplateVars {
    alloc: std.mem.Allocator;
    entries: std.ArrayList(UriVarEntry) = .empty;

    /// Initialize an empty variable binding container.
    pub static fn init(alloc: std.mem.Allocator): TemplateVars {
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

    fn freeEntry(*_, entry: *UriVarEntry) {
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

    fn getEntry(name: []const u8): (&UriVarEntry)? {
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
            _.entries.items[idx] = UriVarEntry{
                name: name_copy,
                kind: .Scalar,
                scalar: val_copy,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, UriVarEntry{
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
            _.entries.items[idx] = UriVarEntry{
                name: name_copy,
                kind: .List,
                list: copied,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, UriVarEntry{
                name: name_copy,
                kind: .List,
                list: copied,
            }) catch return UriTemplateError.OutOfMemory;
        }
    }

    /// Bind an associative list of key-value pairs to a variable name (overwrites any previous binding).
    pub fn setAssoc(*_, name: []const u8, pairs: []const AssocPair): void ! UriTemplateError {
        let copied = _.alloc.alloc(UriAssocPairOwned, pairs.len) catch return UriTemplateError.OutOfMemory;
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
            copied[built] = UriAssocPairOwned{
                key: k_copy,
                value: v_copy,
            };
        }

        if (idx := _.findIndex(name)) {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            _.freeEntry(&_.entries.items[idx]);
            _.entries.items[idx] = UriVarEntry{
                name: name_copy,
                kind: .Assoc,
                assoc: copied,
            };
        } else {
            const name_copy = _.alloc.dupe(u8, name) catch return UriTemplateError.OutOfMemory;
            errdefer _.alloc.free(name_copy);

            _.entries.append(_.alloc, UriVarEntry{
                name: name_copy,
                kind: .Assoc,
                assoc: copied,
            }) catch return UriTemplateError.OutOfMemory;
        }
    }
}

struct UriVarSpec {
    name: []u8;
    explode: bool = false;
    max_len: usize? = null;
}

struct UriExprPart {
    op: TemplateOp;
    vars: []UriVarSpec;
}

enum UriTemplatePart {
    Literal: []u8,
    Expr: UriExprPart,
}

/// Parsed RFC 6570 Level 1-4 URI Template.
pub struct UriTemplate {
    alloc: std.mem.Allocator;
    parts: []UriTemplatePart;

    /// Parse and validate an RFC 6570 URI template pattern using 16-lane SIMD brace scanning.
    pub static fn parse(alloc: std.mem.Allocator, pattern: []const u8): UriTemplate ! UriTemplateError {
        let part_list: std.ArrayList(UriTemplatePart) = .empty;
        errdefer {
            for (part of part_list.items) {
                uriFreeTemplatePart(alloc, part);
            }
            part_list.deinit(alloc);
        }

        let i: usize = 0;
        let lit_start: usize = 0;

        while (i < pattern.len) {
            const rel_opt = simdUriFindBrace(pattern[i..]);
            if (rel_opt == null) {
                break;
            }
            const brace_pos = i + rel_opt.?;
            const ch = pattern[brace_pos];
            if (ch == '}') {
                return UriTemplateError.UnexpectedCloseBrace;
            }
            // ch == '{'
            if (brace_pos > lit_start) {
                const lit = alloc.dupe(u8, pattern[lit_start..brace_pos]) catch return UriTemplateError.OutOfMemory;
                errdefer alloc.free(lit);
                part_list.append(alloc, UriTemplatePart{ .Literal = lit }) catch return UriTemplateError.OutOfMemory;
            }

            const expr_start = brace_pos + 1;
            const inner_rel = simdUriFindBrace(pattern[expr_start..]) orelse return UriTemplateError.UnclosedExpression;
            const end_idx = expr_start + inner_rel;
            if (pattern[end_idx] == '{') {
                return UriTemplateError.UnclosedExpression;
            }

            const expr_body = pattern[expr_start..end_idx];
            const parsed_expr = uriParseExpression(alloc, expr_body)?;
            errdefer uriFreeExprPart(alloc, parsed_expr);
            part_list.append(alloc, UriTemplatePart{ .Expr = parsed_expr }) catch return UriTemplateError.OutOfMemory;

            i = end_idx + 1;
            lit_start = i;
        }

        if (lit_start < pattern.len) {
            const lit = alloc.dupe(u8, pattern[lit_start..pattern.len]) catch return UriTemplateError.OutOfMemory;
            errdefer alloc.free(lit);
            part_list.append(alloc, UriTemplatePart{ .Literal = lit }) catch return UriTemplateError.OutOfMemory;
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
            uriFreeTemplatePart(_.alloc, part);
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

    /// Expand the URI template with an explicit `max_output_len` cap to prevent expansion blowup.
    pub fn expandWithLimit(alloc: std.mem.Allocator, vars: *TemplateVars, max_output_len: usize): []u8 ! UriTemplateError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        for (part of _.parts) {
            switch (part) {
                .Literal => |lit| {
                    uriAppendBoundedSlice(alloc, &out, lit, max_output_len)?;
                },
                .Expr => |expr| {
                    uriExpandExpression(alloc, &out, expr, vars, max_output_len)?;
                },
            }
        }

        return out.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
    }

    /// Expand the URI template using the supplied variable bindings (RFC 6570 Levels 1-4).
    pub fn expand(alloc: std.mem.Allocator, vars: *TemplateVars): []u8 ! UriTemplateError {
        return _.expandWithLimit(alloc, vars, URI_TEMPLATE_DEFAULT_MAX_OUTPUT);
    }
}

fn uriAppendBoundedByte(alloc: std.mem.Allocator, out: *std.ArrayList(u8), b: u8, max_len: usize): void ! UriTemplateError {
    if (out.items.len >= max_len) return UriTemplateError.OutputTooLarge;
    out.append(alloc, b) catch return UriTemplateError.OutOfMemory;
}

fn uriAppendBoundedSlice(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8, max_len: usize): void ! UriTemplateError {
    if (s.len > max_len || out.items.len > max_len - s.len) return UriTemplateError.OutputTooLarge;
    out.appendSlice(alloc, s) catch return UriTemplateError.OutOfMemory;
}

fn uriFreeExprPart(alloc: std.mem.Allocator, expr: UriExprPart) {
    for (vs of expr.vars) {
        alloc.free(vs.name);
    }
    if (expr.vars.len > 0) {
        alloc.free(expr.vars);
    }
}

fn uriFreeTemplatePart(alloc: std.mem.Allocator, part: UriTemplatePart) {
    switch (part) {
        .Literal => |lit| alloc.free(lit),
        .Expr => |expr| uriFreeExprPart(alloc, expr),
    }
}

fn uriIsHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn uriIsVarChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_';
}

fn uriValidateVarName(name: []const u8): bool {
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
            if (!uriIsHexDigit(name[i + 1]) || !uriIsHexDigit(name[i + 2])) return false;
            i += 3;
            continue;
        }
        if (!uriIsVarChar(c)) return false;
        i += 1;
    }
    return true;
}

fn uriParseVarSpec(alloc: std.mem.Allocator, raw: []const u8): UriVarSpec ! UriTemplateError {
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
    if (!uriValidateVarName(name_slice)) {
        return UriTemplateError.InvalidVarName;
    }

    const owned_name = alloc.dupe(u8, name_slice) catch return UriTemplateError.OutOfMemory;
    return UriVarSpec{
        name: owned_name,
        explode: explode,
        max_len: max_len,
    };
}

fn uriParseExpression(alloc: std.mem.Allocator, body: []const u8): UriExprPart ! UriTemplateError {
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

    let specs: std.ArrayList(UriVarSpec) = .empty;
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
            const spec = uriParseVarSpec(alloc, rest[start..i])?;
            errdefer alloc.free(spec.name);
            specs.append(alloc, spec) catch return UriTemplateError.OutOfMemory;
            start = i + 1;
        }
    }

    const owned_specs = specs.toOwnedSlice(alloc) catch return UriTemplateError.OutOfMemory;
    return UriExprPart{
        op: op,
        vars: owned_specs,
    };
}

fn uriIsUnreserved(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '.' || c == '_' || c == '~';
}

fn uriIsReserved(c: u8): bool {
    return switch (c) {
        ':', '/', '?', '#', '[', ']', '@',
        '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=' => true,
        else => false,
    };
}

fn uriAppendPctEncoded(alloc: std.mem.Allocator, out: *std.ArrayList(u8), byte: u8, max_len: usize): void ! UriTemplateError {
    const hex = "0123456789ABCDEF";
    const buf: [3]u8 = ['%', hex[byte >> 4], hex[byte & 0x0F]];
    uriAppendBoundedSlice(alloc, out, buf[0..3], max_len)?;
}

fn uriAppendEncoded(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    value: []const u8,
    allow_reserved: bool,
    max_len: usize,
): void ! UriTemplateError {
    let i: usize = 0;
    while (i < value.len) {
        const c = value[i];
        if (uriIsUnreserved(c)) {
            uriAppendBoundedByte(alloc, out, c, max_len)?;
            i += 1;
            continue;
        }
        if (allow_reserved) {
            if (uriIsReserved(c)) {
                uriAppendBoundedByte(alloc, out, c, max_len)?;
                i += 1;
                continue;
            }
            if (c == '%' && i + 2 < value.len && uriIsHexDigit(value[i + 1]) && uriIsHexDigit(value[i + 2])) {
                uriAppendBoundedSlice(alloc, out, value[i .. i + 3], max_len)?;
                i += 3;
                continue;
            }
        }
        uriAppendPctEncoded(alloc, out, c, max_len)?;
        i += 1;
    }
}

/// Truncate UTF-8 string to at most `max_chars` Unicode codepoints (RFC 6570 Section 2.4.1).
fn uriTruncateUtf8Prefix(s: []const u8, max_chars: usize): []const u8 {
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

fn uriExpandExpression(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    expr: UriExprPart,
    vars: *TemplateVars,
    max_len: usize,
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
                    raw_val = uriTruncateUtf8Prefix(entry.scalar, ml);
                }
                if (is_first) {
                    uriAppendBoundedSlice(alloc, out, op.first(), max_len)?;
                    is_first = false;
                } else {
                    uriAppendBoundedSlice(alloc, out, op.sep(), max_len)?;
                }
                if (named) {
                    uriAppendBoundedSlice(alloc, out, vs.name, max_len)?;
                    if (raw_val.len == 0) {
                        uriAppendBoundedSlice(alloc, out, op.ifEmpty(), max_len)?;
                    } else {
                        uriAppendBoundedByte(alloc, out, '=', max_len)?;
                        uriAppendEncoded(alloc, out, raw_val, allow_res, max_len)?;
                    }
                } else {
                    uriAppendEncoded(alloc, out, raw_val, allow_res, max_len)?;
                }
            },
            .List => {
                if (entry.list.len == 0) continue;
                if (is_first) {
                    uriAppendBoundedSlice(alloc, out, op.first(), max_len)?;
                    is_first = false;
                } else {
                    uriAppendBoundedSlice(alloc, out, op.sep(), max_len)?;
                }

                if (!vs.explode) {
                    if (named) {
                        uriAppendBoundedSlice(alloc, out, vs.name, max_len)?;
                        uriAppendBoundedByte(alloc, out, '=', max_len)?;
                    }
                    let idx: usize = 0;
                    while (idx < entry.list.len) : (idx += 1) {
                        if (idx > 0) {
                            uriAppendBoundedByte(alloc, out, ',', max_len)?;
                        }
                        uriAppendEncoded(alloc, out, entry.list[idx], allow_res, max_len)?;
                    }
                } else {
                    let idx: usize = 0;
                    while (idx < entry.list.len) : (idx += 1) {
                        if (idx > 0) {
                            uriAppendBoundedSlice(alloc, out, op.sep(), max_len)?;
                        }
                        const item = entry.list[idx];
                        if (named) {
                            uriAppendBoundedSlice(alloc, out, vs.name, max_len)?;
                            if (item.len == 0) {
                                uriAppendBoundedSlice(alloc, out, op.ifEmpty(), max_len)?;
                            } else {
                                uriAppendBoundedByte(alloc, out, '=', max_len)?;
                                uriAppendEncoded(alloc, out, item, allow_res, max_len)?;
                            }
                        } else {
                            uriAppendEncoded(alloc, out, item, allow_res, max_len)?;
                        }
                    }
                }
            },
            .Assoc => {
                if (entry.assoc.len == 0) continue;
                if (is_first) {
                    uriAppendBoundedSlice(alloc, out, op.first(), max_len)?;
                    is_first = false;
                } else {
                    uriAppendBoundedSlice(alloc, out, op.sep(), max_len)?;
                }

                if (!vs.explode) {
                    if (named) {
                        uriAppendBoundedSlice(alloc, out, vs.name, max_len)?;
                        uriAppendBoundedByte(alloc, out, '=', max_len)?;
                    }
                    let idx: usize = 0;
                    while (idx < entry.assoc.len) : (idx += 1) {
                        if (idx > 0) {
                            uriAppendBoundedByte(alloc, out, ',', max_len)?;
                        }
                        uriAppendEncoded(alloc, out, entry.assoc[idx].key, allow_res, max_len)?;
                        uriAppendBoundedByte(alloc, out, ',', max_len)?;
                        uriAppendEncoded(alloc, out, entry.assoc[idx].value, allow_res, max_len)?;
                    }
                } else {
                    let idx: usize = 0;
                    while (idx < entry.assoc.len) : (idx += 1) {
                        if (idx > 0) {
                            uriAppendBoundedSlice(alloc, out, op.sep(), max_len)?;
                        }
                        const pair = entry.assoc[idx];
                        uriAppendEncoded(alloc, out, pair.key, allow_res, max_len)?;
                        if (named && pair.value.len == 0) {
                            uriAppendBoundedSlice(alloc, out, op.ifEmpty(), max_len)?;
                        } else {
                            uriAppendBoundedByte(alloc, out, '=', max_len)?;
                            uriAppendEncoded(alloc, out, pair.value, allow_res, max_len)?;
                        }
                    }
                }
            },
        }
    }
}
