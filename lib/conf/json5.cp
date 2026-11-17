// Source: Custom implementation by Antigravity AI
// lib/conf/json5.cp
// JSON5 Token Scanner & Comment-Stripping Preprocessor
// Absorbed from Bun src/interchange/json5.cp

const std = @import("std");

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

pub struct JSON5 {
    /// Strips JSON5 single-line (//) and multi-line (/* */) comments and trailing commas into `out_buf`.
    /// Returns the valid slice of `out_buf` containing standard JSON-compatible bytes.
    pub static fn stripCommentsAndTrailingCommas(src: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_len: usize = 0;
        let in_string: bool = false;
        let quote_char: u8 = 0;

        while (i < src.len && out_len < out_buf.len) {
            const c = src[i];

            if (in_string) {
                if (c == '\\' && i + 1 < src.len) {
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

            if (c == '"' || c == '\'') {
                in_string = true;
                quote_char = c;
                out_buf[out_len] = '"';
                out_len += 1;
                i += 1;
                continue;
            }

            // Single-line comment //
            if (c == '/' && i + 1 < src.len && src[i + 1] == '/') {
                i += 2;
                while (i < src.len && src[i] != '\n') {
                    i += 1;
                }
                continue;
            }

            // Multi-line comment /* ... */
            if (c == '/' && i + 1 < src.len && src[i + 1] == '*') {
                i += 2;
                while (i + 1 < src.len && !(src[i] == '*' && src[i + 1] == '/')) {
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
                while (lookahead < src.len && (src[lookahead] == ' ' || src[lookahead] == '\t' || src[lookahead] == '\r' || src[lookahead] == '\n')) {
                    lookahead += 1;
                }
                if (lookahead < src.len && (src[lookahead] == '}' || src[lookahead] == ']')) {
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
