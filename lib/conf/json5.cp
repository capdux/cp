// Source: Absorbed from /data/dev/bun/src/interchange/json5.cp | Synced: 2026-11-22T07:47:14Z
// lib/conf/json5.cp
// Full JSON5 Parser & Comment-Stripping Preprocessor
// Supports comments, trailing commas, unquoted keys, single quotes, hex numbers, Infinity/NaN

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "../json/value";

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
        while (pos < src.len) {
            const c = src[pos];
            if (c == ' ' or c == '\t' or c == '\r' or c == '\n' or c == 0x0B or c == 0x0C) {
                pos += 1;
                continue;
            }
            // Single-line comment //
            if (c == '/' and pos + 1 < src.len and src[pos + 1] == '/') {
                pos += 2;
                while (pos < src.len and src[pos] != '\n') {
                    pos += 1;
                }
                continue;
            }
            // Multi-line comment /* ... */
            if (c == '/' and pos + 1 < src.len and src[pos + 1] == '*') {
                pos += 2;
                let closed = false;
                while (pos + 1 < src.len) {
                    if (src[pos] == '*' and src[pos + 1] == '/') {
                        pos += 2;
                        closed = true;
                        break;
                    }
                    pos += 1;
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
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_' or c == '$';
    }

    fn isIdContinue(c: u8): bool {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_' or c == '$';
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
            const b = src[pos];
            if (b == '\\') {
                has_escape = true;
                pos += 2;
            } else if (b == quote) {
                const s = src[start..pos];
                pos += 1; // skip closing quote
                if (!has_escape) {
                    return s;
                }
                // Decode escape sequences
                let buf = try allocator.alloc(u8, s.len);
                let out_idx: usize = 0;
                let i: usize = 0;
                while (i < s.len) {
                    if (s[i] == '\\' and i + 1 < s.len) {
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
            } else {
                pos += 1;
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
            while (pos < src.len) {
                const c = src[pos];
                if ((c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F')) {
                    pos += 1;
                } else {
                    break;
                }
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
        while (pos < src.len) {
            const c = src[pos];
            if ((c >= '0' and c <= '9') or c == '.' or c == 'e' or c == 'E' or c == '+' or c == '-') {
                pos += 1;
            } else {
                break;
            }
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
            const c = src[i];

            if (in_string) {
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
                i += 2;
                while (i < src.len and src[i] != '\n') {
                    i += 1;
                }
                continue;
            }

            // Multi-line comment /* ... */
            if (c == '/' and i + 1 < src.len and src[i + 1] == '*') {
                i += 2;
                while (i + 1 < src.len and !(src[i] == '*' and src[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < src.len) {
                    i += 2;
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
