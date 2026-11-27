// Source: Absorbed from /data/dev/bun/src/interchange/json5.cp | Synced: 2026-11-22T07:47:14Z
// lib/conf/json5.cp
// Full JSON5 Parser & Comment-Stripping Preprocessor
// Supports comments, trailing commas, unquoted keys, single quotes, hex numbers, Infinity/NaN

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "../json/value";

type Json5Vec16 = @Vector(16, u8);

const J5_WS: u8 = 1;
const J5_ID_START: u8 = 2;
const J5_ID_CONT: u8 = 4;
const J5_HEX: u8 = 8;
const J5_NUM: u8 = 16;

fn makeJson5CharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[usize(' ')] |= J5_WS;
    lut[usize('\t')] |= J5_WS;
    lut[usize('\r')] |= J5_WS;
    lut[usize('\n')] |= J5_WS;
    lut[0x0B] |= J5_WS;
    lut[0x0C] |= J5_WS;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_' || i == '$') {
            lut[i] |= J5_ID_START | J5_ID_CONT;
        }
        if (i >= '0' && i <= '9') {
            lut[i] |= J5_ID_CONT | J5_HEX | J5_NUM;
        }
        if ((i >= 'a' && i <= 'f') || (i >= 'A' && i <= 'F')) {
            lut[i] |= J5_HEX;
        }
    }
    lut[usize('.')] |= J5_NUM;
    lut[usize('e')] |= J5_NUM;
    lut[usize('E')] |= J5_NUM;
    lut[usize('+')] |= J5_NUM;
    lut[usize('-')] |= J5_NUM;
    return lut;
}

const JSON5_CHAR_LUT: [u8; 256] = makeJson5CharLut();

fn j5FindByteSimd(s: []const u8, start: usize, target: u8): usize {
    let i = start;
    const t_v: Json5Vec16 = @splat(target);
    while (i + 16 <= s.len) : (i += 16) {
        const v: Json5Vec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == t_v);
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == target) return i;
    }
    return s.len;
}

fn j5FindQuoteOrBsSimd(s: []const u8, start: usize, quote: u8): usize {
    let i = start;
    const q_v: Json5Vec16 = @splat(quote);
    const bs_v: Json5Vec16 = @splat('\\' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: Json5Vec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == q_v) | (v == bs_v));
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == quote || s[i] == '\\') return i;
    }
    return s.len;
}

fn j5FindStripSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const dq_v: Json5Vec16 = @splat('"' as u8);
    const sq_v: Json5Vec16 = @splat('\'' as u8);
    const sl_v: Json5Vec16 = @splat('/' as u8);
    const cm_v: Json5Vec16 = @splat(',' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: Json5Vec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == dq_v) | (v == sq_v) | (v == sl_v) | (v == cm_v));
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\'' || c == '/' || c == ',') return i;
    }
    return s.len;
}

export error Json5Error {
    UnexpectedEof,
    UnexpectedCharacter,
    UnexpectedToken,
    UnterminatedString,
    UnterminatedComment,
    InvalidNumber,
    InvalidHexNumber,
    ExpectedColon,
    ExpectedComma,
    ExpectedClosingBrace,
    ExpectedClosingBracket,
}

pub enum Json5TokenKind: u8 {
    Eof = 0,
    LeftBrace = 1,
    RightBrace = 2,
    LeftBracket = 3,
    RightBracket = 4,
    Colon = 5,
    Comma = 6,
    String = 7,
    Number = 8,
    Boolean = 9,
    Null = 10,
    Identifier = 11,
}

pub struct JSON5Parser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): JSON5Parser {
        return JSON5Parser{
            .src = src,
            .pos = 0,
            .allocator = allocator,
        };
    }

    fn peekByte(*_): u8? {
        if (pos >= src.len) return null;
        return src[pos];
    }

    fn skipWhitespaceAndComments(*_): !void {
        const sp_v: Json5Vec16 = @splat(' ' as u8);
        const tb_v: Json5Vec16 = @splat('\t' as u8);
        const cr_v: Json5Vec16 = @splat('\r' as u8);
        const lf_v: Json5Vec16 = @splat('\n' as u8);
        const vt_v: Json5Vec16 = @splat(0x0B as u8);
        const ff_v: Json5Vec16 = @splat(0x0C as u8);
        while (pos < src.len) {
            while (pos + 16 <= src.len) {
                const v: Json5Vec16 = src[pos .. pos + 16][0..16].*;
                const is_ws = (v == sp_v) | (v == tb_v) | (v == cr_v) | (v == lf_v) | (v == vt_v) | (v == ff_v);
                if (@reduce(.And, is_ws)) {
                    pos += 16;
                } else {
                    const non_ws_mask: u16 = ~@as(u16, @bitCast(is_ws));
                    pos += (@ctz(non_ws_mask) as usize);
                    break;
                }
            }
            while (pos < src.len && (JSON5_CHAR_LUT[usize(src[pos])] & J5_WS) != 0) {
                pos += 1;
            }
            if (pos >= src.len) break;
            const c = src[pos];
            // Single-line comment //
            if (c == '/' and pos + 1 < src.len and src[pos + 1] == '/') {
                pos = j5FindByteSimd(src, pos + 2, '\n');
                continue;
            }
            // Multi-line comment /* ... */
            if (c == '/' and pos + 1 < src.len and src[pos + 1] == '*') {
                pos += 2;
                let closed = false;
                while (pos + 1 < src.len) {
                    const star = j5FindByteSimd(src, pos, '*');
                    if (star + 1 >= src.len) {
                        pos = src.len;
                        break;
                    }
                    if (src[star + 1] == '/') {
                        pos = star + 2;
                        closed = true;
                        break;
                    }
                    pos = star + 1;
                }
                if (!closed) {
                    return error.UnterminatedComment;
                }
                continue;
            }
            break;
        }
    }

    fn isIdStart(c: u8): bool {
        return (JSON5_CHAR_LUT[usize(c)] & J5_ID_START) != 0;
    }

    fn isIdContinue(c: u8): bool {
        return (JSON5_CHAR_LUT[usize(c)] & J5_ID_CONT) != 0;
    }

    fn parseIdentifier(*_): []const u8 {
        const start = pos;
        while (pos < src.len and isIdContinue(src[pos])) {
            pos += 1;
        }
        return src[start..pos];
    }

    fn parseKey(*_): ![]const u8 {
        try skipWhitespaceAndComments();
        if (pos >= src.len) return error.UnexpectedEof;
        const c = src[pos];
        if (c == '"' or c == '\'') {
            return try parseString();
        }
        if (isIdStart(c)) {
            return parseIdentifier();
        }
        return error.UnexpectedCharacter;
    }

    fn parseString(*_): ![]const u8 {
        try skipWhitespaceAndComments();
        if (pos >= src.len) return error.UnexpectedEof;
        const quote = src[pos];
        if (quote != '"' and quote != '\'') {
            return error.UnexpectedCharacter;
        }
        pos += 1; // skip opening quote

        const start = pos;
        let has_escape = false;
        while (pos < src.len) {
            pos = j5FindQuoteOrBsSimd(src, pos, quote);
            if (pos >= src.len) break;
            const b = src[pos];
            if (b == '\\') {
                has_escape = true;
                pos += 2;
            } else {
                const s = src[start..pos];
                pos += 1; // skip closing quote
                if (!has_escape) {
                    return s;
                }
                // Decode escape sequences with SIMD backslash jump + @memcpy
                let buf = try allocator.alloc(u8, s.len);
                let out_idx: usize = 0;
                let i: usize = 0;
                while (i < s.len) {
                    const next_bs = j5FindByteSimd(s, i, '\\');
                    const plain_len = next_bs - i;
                    if (plain_len > 0) {
                        @memcpy(buf[out_idx .. out_idx + plain_len], s[i..next_bs]);
                        out_idx += plain_len;
                        i = next_bs;
                    }
                    if (i >= s.len) break;
                    if (i + 1 < s.len) {
                        const esc = s[i + 1];
                        if (esc == 'n') { buf[out_idx] = '\n'; out_idx += 1; i += 2; }
                        else if (esc == 'r') { buf[out_idx] = '\r'; out_idx += 1; i += 2; }
                        else if (esc == 't') { buf[out_idx] = '\t'; out_idx += 1; i += 2; }
                        else if (esc == 'b') { buf[out_idx] = 0x08; out_idx += 1; i += 2; }
                        else if (esc == 'f') { buf[out_idx] = 0x0C; out_idx += 1; i += 2; }
                        else if (esc == 'v') { buf[out_idx] = 0x0B; out_idx += 1; i += 2; }
                        else if (esc == '0') { buf[out_idx] = 0; out_idx += 1; i += 2; }
                        else if (esc == '\n') { i += 2; } // Fold escaped newline
                        else if (esc == '\r' and i + 2 < s.len and s[i + 2] == '\n') { i += 3; }
                        else {
                            buf[out_idx] = esc;
                            out_idx += 1;
                            i += 2;
                        }
                    } else {
                        buf[out_idx] = s[i];
                        out_idx += 1;
                        i += 1;
                    }
                }
                return buf[0..out_idx];
            }
        }
        return error.UnterminatedString;
    }

    fn parseNumber(*_): !f64 {
        try skipWhitespaceAndComments();
        const start = pos;

        if (pos < src.len and (src[pos] == '+' or src[pos] == '-')) {
            pos += 1;
        }

        // Hexadecimal support: 0x... / 0X...
        if (pos + 2 <= src.len and src[pos] == '0' and (src[pos + 1] == 'x' or src[pos + 1] == 'X')) {
            pos += 2;
            while (pos < src.len && (JSON5_CHAR_LUT[usize(src[pos])] & J5_HEX) != 0) {
                pos += 1;
            }
            const hex_slice = src[start..pos];
            let clean_hex = hex_slice;
            let negative = false;
            if (clean_hex.len > 0 and clean_hex[0] == '-') {
                negative = true;
                clean_hex = clean_hex[1..];
            } else if (clean_hex.len > 0 and clean_hex[0] == '+') {
                clean_hex = clean_hex[1..];
            }
            if (clean_hex.len > 2 and clean_hex[0] == '0' and (clean_hex[1] == 'x' or clean_hex[1] == 'X')) {
                clean_hex = clean_hex[2..];
            }
            const val = std.fmt.parseInt(i64, clean_hex, 16) catch return error.InvalidHexNumber;
            const res: f64 = @floatFromInt(val);
            return if (negative) -res else res;
        }

        // Decimal / floating point
        while (pos < src.len && (JSON5_CHAR_LUT[usize(src[pos])] & J5_NUM) != 0) {
            pos += 1;
        }
        const num_str = src[start..pos];
        let parse_slice = num_str;
        if (parse_slice.len > 0 and parse_slice[0] == '+') {
            parse_slice = parse_slice[1..];
        }
        return std.fmt.parseFloat(f64, parse_slice) catch error.InvalidNumber;
    }

    pub fn parseValue(*_): !JsonValue {
        try skipWhitespaceAndComments();
        if (pos >= src.len) return error.UnexpectedEof;

        const ch = src[pos];

        if (ch == '{') {
            return try parseObject();
        }

        if (ch == '[') {
            return try parseArray();
        }

        if (ch == '"' or ch == '\'') {
            const s = try parseString();
            return JsonValue{ .tag = .String, .str_val = s };
        }

        if (ch == '-' or ch == '+' or ch == '.' or (ch >= '0' and ch <= '9')) {
            const n = try parseNumber();
            return JsonValue{ .tag = .Number, .num_val = n };
        }

        // Keywords & Identifiers (null, true, false, NaN, Infinity)
        if (isIdStart(ch)) {
            const ident = parseIdentifier();
            if (std.mem.eql(u8, ident, "null")) {
                return JsonValue{ .tag = .Null };
            }
            if (std.mem.eql(u8, ident, "true")) {
                return JsonValue{ .tag = .Bool, .bool_val = true };
            }
            if (std.mem.eql(u8, ident, "false")) {
                return JsonValue{ .tag = .Bool, .bool_val = false };
            }
            if (std.mem.eql(u8, ident, "NaN")) {
                return JsonValue{ .tag = .Number, .num_val = std.math.nan(f64) };
            }
            if (std.mem.eql(u8, ident, "Infinity")) {
                return JsonValue{ .tag = .Number, .num_val = std.math.inf(f64) };
            }
            return error.UnexpectedToken;
        }

        return error.UnexpectedCharacter;
    }

    fn parseArray(*_): !JsonValue {
        pos += 1; // skip '['
        try skipWhitespaceAndComments();

        let list: std.ArrayList(JsonValue) = .empty;
        errdefer list.deinit(allocator);

        if (pos < src.len and src[pos] == ']') {
            pos += 1;
            return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(allocator) };
        }

        while (true) {
            const val = try parseValue();
            try list.append(allocator, val);

            try skipWhitespaceAndComments();
            if (pos >= src.len) return error.UnexpectedEof;

            const sep = src[pos];
            if (sep == ']') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
                try skipWhitespaceAndComments();
                // Trailing comma support: [ 1, 2, ]
                if (pos < src.len and src[pos] == ']') {
                    pos += 1;
                    break;
                }
            } else {
                return error.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(allocator) };
    }

    fn parseObject(*_): !JsonValue {
        pos += 1; // skip '{'
        try skipWhitespaceAndComments();

        let list: std.ArrayList(JsonField) = .empty;
        errdefer list.deinit(allocator);

        if (pos < src.len and src[pos] == '}') {
            pos += 1;
            return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(allocator) };
        }

        while (true) {
            const key = try parseKey();
            try skipWhitespaceAndComments();
            if (pos >= src.len or src[pos] != ':') {
                return error.ExpectedColon;
            }
            pos += 1; // skip ':'

            const val = try parseValue();
            try list.append(allocator, JsonField{ .key = key, .value = val });

            try skipWhitespaceAndComments();
            if (pos >= src.len) return error.UnexpectedEof;

            const sep = src[pos];
            if (sep == '}') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
                try skipWhitespaceAndComments();
                // Trailing comma support: { a: 1, }
                if (pos < src.len and src[pos] == '}') {
                    pos += 1;
                    break;
                }
            } else {
                return error.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(allocator) };
    }
}

pub struct JSON5 {
    /// Parse JSON5 string into CP standard JsonValue AST.
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): !JsonValue {
        let parser = JSON5Parser.init(allocator, src);
        return try parser.parseValue();
    }

    /// Strips JSON5 single-line (//) and multi-line (/* */) comments and trailing commas into `out_buf`.
    /// Returns the valid slice of `out_buf` containing standard JSON-compatible bytes.
    pub static fn stripCommentsAndTrailingCommas(src: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_len: usize = 0;
        let in_string: bool = false;
        let quote_char: u8 = 0;

        while (i < src.len and out_len < out_buf.len) {
            if (in_string) {
                const hit = j5FindQuoteOrBsSimd(src, i, quote_char);
                const plain = hit - i;
                if (plain > 0) {
                    const avail = out_buf.len - out_len;
                    const take = if (plain < avail) plain else avail;
                    @memcpy(out_buf[out_len .. out_len + take], src[i .. i + take]);
                    out_len += take;
                    i += take;
                    if (out_len >= out_buf.len or i >= src.len) break;
                }
                const c = src[i];
                if (c == '\\' and i + 1 < src.len) {
                    out_buf[out_len] = c;
                    out_len += 1;
                    i += 1;
                    if (out_len < out_buf.len) {
                        out_buf[out_len] = src[i];
                        out_len += 1;
                    }
                    i += 1;
                    continue;
                }
                if (c == quote_char) {
                    in_string = false;
                    out_buf[out_len] = '"';
                    out_len += 1;
                    i += 1;
                    continue;
                }
                out_buf[out_len] = c;
                out_len += 1;
                i += 1;
                continue;
            }

            const hit = j5FindStripSpecialSimd(src, i);
            const plain = hit - i;
            if (plain > 0) {
                const avail = out_buf.len - out_len;
                const take = if (plain < avail) plain else avail;
                @memcpy(out_buf[out_len .. out_len + take], src[i .. i + take]);
                out_len += take;
                i += take;
                if (out_len >= out_buf.len or i >= src.len) break;
            }

            const c = src[i];
            if (c == '"' or c == '\'') {
                in_string = true;
                quote_char = c;
                out_buf[out_len] = '"';
                out_len += 1;
                i += 1;
                continue;
            }

            // Single-line comment //
            if (c == '/' and i + 1 < src.len and src[i + 1] == '/') {
                i = j5FindByteSimd(src, i + 2, '\n');
                continue;
            }

            // Multi-line comment /* ... */
            if (c == '/' and i + 1 < src.len and src[i + 1] == '*') {
                i += 2;
                while (i + 1 < src.len) {
                    const star = j5FindByteSimd(src, i, '*');
                    if (star + 1 >= src.len) {
                        i = src.len;
                        break;
                    }
                    if (src[star + 1] == '/') {
                        i = star + 2;
                        break;
                    }
                    i = star + 1;
                }
                continue;
            }

            // Skip trailing comma before } or ]
            if (c == ',') {
                let lookahead = i + 1;
                while (lookahead < src.len and (src[lookahead] == ' ' or src[lookahead] == '\t' or src[lookahead] == '\r' or src[lookahead] == '\n')) {
                    lookahead += 1;
                }
                if (lookahead < src.len and (src[lookahead] == '}' or src[lookahead] == ']')) {
                    i += 1;
                    continue;
                }
            }

            out_buf[out_len] = c;
            out_len += 1;
            i += 1;
        }

        return out_buf[0..out_len];
    }
}

pub const Json5Scanner = JSON5;
