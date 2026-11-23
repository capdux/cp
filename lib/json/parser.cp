// Source: Absorbed from /data/dev/bun/src/interchange/json.cp | Synced: 2026-11-22T16:05:00Z
// Reference-hardened against serde-rs/json (MIT/Apache-2.0)
// Pure-CP RFC 8259 Compliant Zero-Copy Streaming, Zero-Heap Validator & Typed JSON Parser
// with cap:simd + 16-lane @Vector(16, u8) SIMD Acceleration, UTF-8 / UTF-16 surrogate pair
// validation, strict RFC 8259 number grammar, and max-depth recursion guard (default 128).

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "./value";
import { Utf8 } from "cap:string";
import { Simd } from "cap:simd";

type JsonScanVec16 = @Vector(16, u8);

/// Default recursion depth limit matching `serde-rs/json` (128).
pub const JSON_DEFAULT_MAX_DEPTH: usize = 128;

pub err JsonError {
    UnexpectedEof,
    InvalidToken,
    ExpectedColon,
    ExpectedComma,
    ExpectedBrace,
    UnknownField,
    MaxDepthExceeded,
    TrailingComma,
    InvalidEscape,
    InvalidUnicode,
    InvalidControlChar,
    TrailingCharacters,
}

pub struct JsonParseOptions {
    max_depth: usize = JSON_DEFAULT_MAX_DEPTH,
    allow_trailing_garbage: bool = false,
}

pub struct FastJsonScanner {
    /// 16-byte SIMD search for either '"' or '\\'
    pub static fn indexOfQuoteOrEscape(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const t1: JsonScanVec16 = @splat('"');
        const t2: JsonScanVec16 = @splat('\\');

        while (i + 16 <= n) {
            const chunk: JsonScanVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const mask: u16 = m1 | m2;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c == '"' || c == '\\') {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 16-byte SIMD search for '"', '\\', or any unescaped ASCII control char (< 0x20)
    pub static fn indexOfQuoteEscapeOrCtrl(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const t1: JsonScanVec16 = @splat('"');
        const t2: JsonScanVec16 = @splat('\\');
        const t_ctrl: JsonScanVec16 = @splat(0x20);

        while (i + 16 <= n) {
            const chunk: JsonScanVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const m3: u16 = @bitCast(chunk < t_ctrl);
            const mask: u16 = m1 | m2 | m3;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c == '"' || c == '\\' || c < 0x20) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 16-byte SIMD RFC 8259 whitespace skipper (`' '`, `'\t'`, `'\r'`, `'\n'`).
    pub static fn skipWhitespace(input: []const u8): usize {
        if (input.len == 0) return 0;
        const c0 = input[0];
        if (c0 != ' ' && c0 != '\t' && c0 != '\r' && c0 != '\n') return 0;
        return Simd.skipWhitespace(input);
    }
}

/// Validates that `s` is well-formed UTF-8 (fast-path ASCII via `Simd.firstNonAscii`).
fn isValidUtf8Slice(s: []const u8): bool {
    const first_opt = Simd.firstNonAscii(s);
    if (first_opt == null) return true;
    return std.unicode.utf8ValidateSlice(s[first_opt.?..]);
}

/// Zero-heap RFC 8259 structural validator state (pure stack recursion bounded by `max_depth`).
struct StrictJsonValidator {
    src: []const u8 = "",
    pos: usize = 0,
    depth: usize = 0,
    max_depth: usize = JSON_DEFAULT_MAX_DEPTH,

    fn skipWs(*_) {
        _.pos += FastJsonScanner.skipWhitespace(_.src[_.pos..]);
    }

    static fn parseHex4(s: []const u8): u16! {
        if (s.len < 4) return err.UnexpectedEof;
        let val: u16 = 0;
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            const c = s[i];
            let digit: u16 = 0;
            if (c >= '0' && c <= '9') {
                digit = (c - '0') as u16;
            } else if (c >= 'a' && c <= 'f') {
                digit = (c - 'a' + 10) as u16;
            } else if (c >= 'A' && c <= 'F') {
                digit = (c - 'A' + 10) as u16;
            } else {
                return err.InvalidUnicode;
            }
            val = (val << 4) | digit;
        }
        return val;
    }

    fn validateString(*_): ! {
        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != '"') return err.InvalidToken;
        _.pos += 1;

        while (_.pos < _.src.len) {
            const rem = _.src[_.pos..];
            const hit = FastJsonScanner.indexOfQuoteEscapeOrCtrl(rem) ?? return err.UnexpectedEof;
            if (hit > 0) {
                if (!isValidUtf8Slice(rem[0..hit])) return err.InvalidUnicode;
                _.pos += hit;
            }
            const ch = _.src[_.pos];
            if (ch < 0x20) return err.InvalidControlChar;
            if (ch == '"') {
                _.pos += 1;
                return;
            }
            // ch == '\\'
            if (_.pos + 1 >= _.src.len) return err.UnexpectedEof;
            const esc = _.src[_.pos + 1];
            _.pos += 2;
            switch (esc) {
                '"', '\\', '/', 'b', 'f', 'n', 'r', 't' => {},
                'u' => {
                    if (_.pos + 4 > _.src.len) return err.UnexpectedEof;
                    const code = StrictJsonValidator.parseHex4(_.src[_.pos .. _.pos + 4])?;
                    _.pos += 4;
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        if (_.pos + 6 > _.src.len) return err.InvalidUnicode;
                        if (_.src[_.pos] != '\\' || _.src[_.pos + 1] != 'u') return err.InvalidUnicode;
                        const lo = StrictJsonValidator.parseHex4(_.src[_.pos + 2 .. _.pos + 6])?;
                        if (lo < 0xDC00 || lo > 0xDFFF) return err.InvalidUnicode;
                        _.pos += 6;
                    } else if (code >= 0xDC00 && code <= 0xDFFF) {
                        return err.InvalidUnicode;
                    }
                },
                else => return err.InvalidEscape,
            }
        }
        return err.UnexpectedEof;
    }

    fn validateNumber(*_): ! {
        _.skipWs();
        if (_.pos >= _.src.len) return err.UnexpectedEof;
        const start = _.pos;

        if (_.src[_.pos] == '-') {
            _.pos += 1;
            if (_.pos >= _.src.len) return err.InvalidToken;
        }

        if (_.src[_.pos] == '0') {
            _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                return err.InvalidToken;
            }
        } else if (_.src[_.pos] >= '1' && _.src[_.pos] <= '9') {
            _.pos += 1;
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        } else {
            return err.InvalidToken;
        }

        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            _.pos += 1;
            if (_.pos >= _.src.len || _.src[_.pos] < '0' || _.src[_.pos] > '9') {
                return err.InvalidToken;
            }
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        }

        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            _.pos += 1;
            if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len || _.src[_.pos] < '0' || _.src[_.pos] > '9') {
                return err.InvalidToken;
            }
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        }

        const num_str = _.src[start.._.pos];
        const v = std.fmt.parseFloat(f64, num_str) catch return err.InvalidToken;
        if (!std.math.isFinite(v)) return err.InvalidToken;
    }

    fn validateArray(*_): ! {
        if (_.depth >= _.max_depth) return err.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // skip '['
        _.skipWs();
        if (_.pos >= _.src.len) return err.UnexpectedEof;
        if (_.src[_.pos] == ']') {
            _.pos += 1;
            return;
        }

        while (true) {
            _.validateValue()?;
            _.skipWs();
            if (_.pos >= _.src.len) return err.UnexpectedEof;
            const sep = _.src[_.pos];
            if (sep == ']') {
                _.pos += 1;
                return;
            } else if (sep == ',') {
                _.pos += 1;
                _.skipWs();
                if (_.pos >= _.src.len) return err.UnexpectedEof;
                if (_.src[_.pos] == ']') return err.TrailingComma;
            } else {
                return err.ExpectedComma;
            }
        }
    }

    fn validateObject(*_): ! {
        if (_.depth >= _.max_depth) return err.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // skip '{'
        _.skipWs();
        if (_.pos >= _.src.len) return err.UnexpectedEof;
        if (_.src[_.pos] == '}') {
            _.pos += 1;
            return;
        }

        while (true) {
            _.validateString()?;
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ':') return err.ExpectedColon;
            _.pos += 1; // skip ':'

            _.validateValue()?;
            _.skipWs();
            if (_.pos >= _.src.len) return err.UnexpectedEof;
            const sep = _.src[_.pos];
            if (sep == '}') {
                _.pos += 1;
                return;
            } else if (sep == ',') {
                _.pos += 1;
                _.skipWs();
                if (_.pos >= _.src.len) return err.UnexpectedEof;
                if (_.src[_.pos] == '}') return err.TrailingComma;
            } else {
                return err.ExpectedComma;
            }
        }
    }

    fn validateValue(*_): ! {
        _.skipWs();
        if (_.pos >= _.src.len) return err.UnexpectedEof;
        const ch = _.src[_.pos];
        if (ch == 'n') {
            if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "null")) {
                _.pos += 4;
                return;
            }
            return err.InvalidToken;
        }
        if (ch == 't') {
            if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "true")) {
                _.pos += 4;
                return;
            }
            return err.InvalidToken;
        }
        if (ch == 'f') {
            if (_.pos + 5 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 5], "false")) {
                _.pos += 5;
                return;
            }
            return err.InvalidToken;
        }
        if (ch == '"') return _.validateString();
        if (ch == '-' || (ch >= '0' && ch <= '9')) return _.validateNumber();
        if (ch == '[') return _.validateArray();
        if (ch == '{') return _.validateObject();
        return err.InvalidToken;
    }
}

/// Zero-heap strict RFC 8259 validator returning detailed `JsonError` on failure.
pub fn validateJsonStrictOrErr(src: []const u8, max_depth: usize): ! {
    let v = StrictJsonValidator{
        .src = src,
        .pos = 0,
        .depth = 0,
        .max_depth = max_depth,
    };
    v.validateValue()?;
    v.skipWs();
    if (v.pos < src.len) return err.TrailingCharacters;
}

/// Zero-heap strict RFC 8259 structural validator with explicit recursion depth cap.
pub fn validateJsonWithMaxDepth(src: []const u8, max_depth: usize): bool {
    validateJsonStrictOrErr(src, max_depth) catch return false;
    return true;
}

/// Zero-heap strict RFC 8259 structural validator with default `serde-rs/json` max depth (128).
pub fn validateJsonStrict(src: []const u8): bool {
    return validateJsonWithMaxDepth(src, JSON_DEFAULT_MAX_DEPTH);
}

pub struct JsonParser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,
    depth: usize = 0,
    max_depth: usize = JSON_DEFAULT_MAX_DEPTH,

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): JsonParser {
        return JsonParser{
            .src = src,
            .pos = 0,
            .allocator = allocator,
            .depth = 0,
            .max_depth = JSON_DEFAULT_MAX_DEPTH,
        };
    }

    pub static fn initWithOptions(allocator: std.mem.Allocator, src: []const u8, opts: JsonParseOptions): JsonParser {
        return JsonParser{
            .src = src,
            .pos = 0,
            .allocator = allocator,
            .depth = 0,
            .max_depth = opts.max_depth,
        };
    }

    fn skipWhitespace(*_) {
        pos += FastJsonScanner.skipWhitespace(src[pos..]);
    }

    fn peek(*_): u8? {
        skipWhitespace();
        if (pos >= src.len) return null;
        return src[pos];
    }

    fn next(*_): u8? {
        skipWhitespace();
        if (pos >= src.len) return null;
        const b = src[pos];
        pos += 1;
        return b;
    }

    static fn parseHex4(s: []const u8): u16! {
        return StrictJsonValidator.parseHex4(s);
    }

    /// SIMD-accelerated RFC 8259 string parser:
    /// - Zero-copy slice borrow when no backslash escapes are present
    /// - Full unescaping including `\uXXXX` and UTF-16 surrogate pairs (`\uD83D\uDE00` -> UTF-8)
    /// - Rejects unescaped ASCII control chars (< 0x20), invalid UTF-8, lone surrogates, and invalid escapes
    pub fn parseString(*_): ([]const u8)! {
        skipWhitespace();
        if (pos >= src.len || src[pos] != '"') {
            return err.InvalidToken;
        }
        pos += 1; // skip opening quote
        const start = pos;

        // Fast path: check first special byte via 16-lane SIMD
        const first_hit = FastJsonScanner.indexOfQuoteEscapeOrCtrl(src[pos..]) ?? return err.UnexpectedEof;
        const hit_pos = pos + first_hit;
        const hit_ch = src[hit_pos];
        if (hit_ch < 0x20) {
            return err.InvalidControlChar;
        }
        if (hit_ch == '"') {
            const slice = src[start..hit_pos];
            if (!isValidUtf8Slice(slice)) return err.InvalidUnicode;
            pos = hit_pos + 1;
            return slice;
        }

        // Slow path: string contains backslash escape sequences
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        if (hit_pos > start) {
            const prefix = src[start..hit_pos];
            if (!isValidUtf8Slice(prefix)) return err.InvalidUnicode;
            out.appendSlice(allocator, prefix)?;
        }
        pos = hit_pos;

        while (pos < src.len) {
            const rem = src[pos..];
            const seg_hit = FastJsonScanner.indexOfQuoteEscapeOrCtrl(rem) ?? return err.UnexpectedEof;
            if (seg_hit > 0) {
                const seg = rem[0..seg_hit];
                if (!isValidUtf8Slice(seg)) return err.InvalidUnicode;
                out.appendSlice(allocator, seg)?;
                pos += seg_hit;
            }

            const ch = src[pos];
            if (ch < 0x20) {
                return err.InvalidControlChar;
            }
            if (ch == '"') {
                pos += 1;
                return out.toOwnedSlice(allocator);
            }

            // ch == '\\'
            if (pos + 1 >= src.len) return err.UnexpectedEof;
            const esc = src[pos + 1];
            pos += 2;

            switch (esc) {
                '"' => out.append(allocator, '"')?,
                '\\' => out.append(allocator, '\\')?,
                '/' => out.append(allocator, '/')?,
                'b' => out.append(allocator, 0x08)?,
                'f' => out.append(allocator, 0x0C)?,
                'n' => out.append(allocator, '\n')?,
                'r' => out.append(allocator, '\r')?,
                't' => out.append(allocator, '\t')?,
                'u' => {
                    if (pos + 4 > src.len) return err.UnexpectedEof;
                    const code = JsonParser.parseHex4(src[pos .. pos + 4])?;
                    pos += 4;

                    let cp: u32 = code as u32;
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        // High surrogate MUST be followed by \uXXXX low surrogate
                        if (pos + 6 > src.len) return err.InvalidUnicode;
                        if (src[pos] != '\\' || src[pos + 1] != 'u') return err.InvalidUnicode;
                        const lo = JsonParser.parseHex4(src[pos + 2 .. pos + 6])?;
                        if (lo < 0xDC00 || lo > 0xDFFF) return err.InvalidUnicode;
                        pos += 6;
                        cp = (0x10000 as u32) + ((((code - 0xD800) as u32) << 10) | ((lo - 0xDC00) as u32));
                    } else if (code >= 0xDC00 && code <= 0xDFFF) {
                        return err.InvalidUnicode;
                    }

                    let utf8_buf: [4]u8 = undefined;
                    const n_bytes = Utf8.encodeCodepoint(cp, &utf8_buf);
                    if (n_bytes == 0) return err.InvalidUnicode;
                    out.appendSlice(allocator, utf8_buf[0..n_bytes])?;
                },
                else => return err.InvalidEscape,
            }
        }
        return err.UnexpectedEof;
    }

    /// Strict RFC 8259 number parser: `-? (0 | [1-9][0-9]*) (\.[0-9]+)? ([eE][+-]?[0-9]+)?`
    pub fn parseNumber(*_): f64! {
        skipWhitespace();
        if (pos >= src.len) return err.UnexpectedEof;
        const start = pos;

        if (src[pos] == '-') {
            pos += 1;
            if (pos >= src.len) return err.InvalidToken;
        }

        if (src[pos] == '0') {
            pos += 1;
            // Leading zero cannot be followed by another digit
            if (pos < src.len && src[pos] >= '0' && src[pos] <= '9') {
                return err.InvalidToken;
            }
        } else if (src[pos] >= '1' && src[pos] <= '9') {
            pos += 1;
            while (pos < src.len && src[pos] >= '0' && src[pos] <= '9') {
                pos += 1;
            }
        } else {
            return err.InvalidToken;
        }

        // Fractional part
        if (pos < src.len && src[pos] == '.') {
            pos += 1;
            if (pos >= src.len || src[pos] < '0' || src[pos] > '9') {
                return err.InvalidToken;
            }
            while (pos < src.len && src[pos] >= '0' && src[pos] <= '9') {
                pos += 1;
            }
        }

        // Exponent part
        if (pos < src.len && (src[pos] == 'e' || src[pos] == 'E')) {
            pos += 1;
            if (pos < src.len && (src[pos] == '+' || src[pos] == '-')) {
                pos += 1;
            }
            if (pos >= src.len || src[pos] < '0' || src[pos] > '9') {
                return err.InvalidToken;
            }
            while (pos < src.len && src[pos] >= '0' && src[pos] <= '9') {
                pos += 1;
            }
        }

        const num_str = src[start..pos];
        const parsed = std.fmt.parseFloat(f64, num_str) catch return err.InvalidToken;
        if (!std.math.isFinite(parsed)) return err.InvalidToken;
        return parsed;
    }

    /// Strict zero-heap skip over an arbitrary JSON value (validates full RFC 8259 syntax & depth).
    pub fn skipValue(*_): ! {
        let v = StrictJsonValidator{
            .src = src,
            .pos = pos,
            .depth = depth,
            .max_depth = max_depth,
        };
        v.validateValue()?;
        pos = v.pos;
    }

    pub fn parseValue(*_): JsonValue! {
        skipWhitespace();
        if (pos >= src.len) return err.UnexpectedEof;
        const ch = src[pos];

        if (ch == 'n') {
            if (pos + 4 <= src.len && std.mem.eql(u8, src[pos .. pos + 4], "null")) {
                pos += 4;
                return JsonValue{ .tag = .Null };
            }
            return err.InvalidToken;
        }

        if (ch == 't') {
            if (pos + 4 <= src.len && std.mem.eql(u8, src[pos .. pos + 4], "true")) {
                pos += 4;
                return JsonValue{ .tag = .Bool, .bool_val = true };
            }
            return err.InvalidToken;
        }

        if (ch == 'f') {
            if (pos + 5 <= src.len && std.mem.eql(u8, src[pos .. pos + 5], "false")) {
                pos += 5;
                return JsonValue{ .tag = .Bool, .bool_val = false };
            }
            return err.InvalidToken;
        }

        if (ch == '"') {
            const s = parseString()?;
            return JsonValue{ .tag = .String, .str_val = s };
        }

        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            const n = parseNumber()?;
            return JsonValue{ .tag = .Number, .num_val = n };
        }

        if (ch == '[') {
            return parseArray();
        }

        if (ch == '{') {
            return parseObject();
        }

        return err.InvalidToken;
    }

    fn parseArray(*_): JsonValue! {
        if (depth >= max_depth) return err.MaxDepthExceeded;
        depth += 1;
        defer depth -= 1;

        pos += 1; // skip '['
        skipWhitespace();

        let list: std.ArrayList(JsonValue) = .empty;
        errdefer list.deinit(allocator);

        if (next_ch := _.peek()) {
            if (next_ch == ']') {
                pos += 1;
                return JsonValue{ .tag = .Array, .arr_val = list.toOwnedSlice(allocator)? };
            }
        } else {
            return err.UnexpectedEof;
        }

        while (true) {
            const val = parseValue()?;
            list.append(allocator, val)?;

            skipWhitespace();
            if (pos >= src.len) return err.UnexpectedEof;
            const sep = src[pos];
            if (sep == ']') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
                skipWhitespace();
                if (pos >= src.len) return err.UnexpectedEof;
                if (src[pos] == ']') return err.TrailingComma;
            } else {
                return err.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Array, .arr_val = list.toOwnedSlice(allocator)? };
    }

    fn parseObject(*_): JsonValue! {
        if (depth >= max_depth) return err.MaxDepthExceeded;
        depth += 1;
        defer depth -= 1;

        pos += 1; // skip '{'
        skipWhitespace();

        let list: std.ArrayList(JsonField) = .empty;
        errdefer list.deinit(allocator);

        if (next_ch := _.peek()) {
            if (next_ch == '}') {
                pos += 1;
                return JsonValue{ .tag = .Object, .obj_val = list.toOwnedSlice(allocator)? };
            }
        } else {
            return err.UnexpectedEof;
        }

        while (true) {
            const key = parseString()?;
            skipWhitespace();
            if (pos >= src.len || src[pos] != ':') {
                return err.ExpectedColon;
            }
            pos += 1; // skip ':'

            const val = parseValue()?;
            list.append(allocator, JsonField{ .key = key, .value = val })?;

            skipWhitespace();
            if (pos >= src.len) return err.UnexpectedEof;
            const sep = src[pos];
            if (sep == '}') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
                skipWhitespace();
                if (pos >= src.len) return err.UnexpectedEof;
                if (src[pos] == '}') return err.TrailingComma;
            } else {
                return err.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Object, .obj_val = list.toOwnedSlice(allocator)? };
    }

    /// Parse JSON directly into CP dynamic JsonValue AST (strict RFC 8259).
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): JsonValue! {
        return JsonParser.parseWithOptions(allocator, src, JsonParseOptions{});
    }

    /// Parse JSON with configurable `JsonParseOptions` (e.g. `max_depth`).
    pub static fn parseWithOptions(allocator: std.mem.Allocator, src: []const u8, opts: JsonParseOptions): JsonValue! {
        let parser = JsonParser.initWithOptions(allocator, src, opts);
        const val = parser.parseValue()?;
        if (!opts.allow_trailing_garbage) {
            parser.skipWhitespace();
            if (parser.pos < src.len) return err.TrailingCharacters;
        }
        return val;
    }

    /// High-performance compiled-in typed JSON parser (strict RFC 8259, zero zig{} blocks).
    pub static fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): T! {
        let pos: usize = 0;
        const val = fastParseType(T, allocator, src, &pos, 0)?;
        pos += FastJsonScanner.skipWhitespace(src[pos..]);
        if (pos < src.len) return err.TrailingCharacters;
        return val;
    }
}

pub fn fastParseType(comptime T: type, allocator: std.mem.Allocator, src: []const u8, pos: *usize, depth: usize): T! {
    pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
    if (pos.* >= src.len) return err.UnexpectedEof;

    const info = @typeInfo(T);
    switch (info) {
        .bool => {
            if (pos.* + 4 <= src.len && std.mem.eql(u8, src[pos.* .. pos.* + 4], "true")) {
                pos.* += 4;
                return true;
            }
            if (pos.* + 5 <= src.len && std.mem.eql(u8, src[pos.* .. pos.* + 5], "false")) {
                pos.* += 5;
                return false;
            }
            return err.InvalidToken;
        },
        .int => {
            const start = pos.*;
            if (pos.* < src.len && src[pos.*] == '-') {
                pos.* += 1;
            }
            if (pos.* >= src.len) return err.InvalidToken;
            if (src[pos.*] == '0') {
                pos.* += 1;
                if (pos.* < src.len && src[pos.*] >= '0' && src[pos.*] <= '9') return err.InvalidToken;
            } else if (src[pos.*] >= '1' && src[pos.*] <= '9') {
                pos.* += 1;
                while (pos.* < src.len && src[pos.*] >= '0' && src[pos.*] <= '9') pos.* += 1;
            } else {
                return err.InvalidToken;
            }
            const num_slice = src[start..pos.*];
            return std.fmt.parseInt(T, num_slice, 10) catch return err.InvalidToken;
        },
        .float => {
            let p = JsonParser.init(allocator, src);
            p.pos = pos.*;
            const f = p.parseNumber()?;
            pos.* = p.pos;
            return @floatCast(f);
        },
        .pointer => |ptr| {
            if (ptr.size == .slice && ptr.child == u8) {
                let p = JsonParser.init(allocator, src);
                p.pos = pos.*;
                const s = p.parseString()?;
                pos.* = p.pos;
                return s;
            }
            if (ptr.size == .slice) {
                if (depth >= JSON_DEFAULT_MAX_DEPTH) return err.MaxDepthExceeded;
                pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                if (pos.* >= src.len || src[pos.*] != '[') return err.InvalidToken;
                pos.* += 1;

                let list: std.ArrayList(ptr.child) = .empty;
                errdefer list.deinit(allocator);

                pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                if (pos.* < src.len && src[pos.*] == ']') {
                    pos.* += 1;
                    return list.toOwnedSlice(allocator);
                }

                while (true) {
                    const item = fastParseType(ptr.child, allocator, src, pos, depth + 1)?;
                    list.append(allocator, item)?;

                    pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                    if (pos.* >= src.len) return err.UnexpectedEof;
                    const sep = src[pos.*];
                    if (sep == ']') {
                        pos.* += 1;
                        break;
                    } else if (sep == ',') {
                        pos.* += 1;
                        pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                        if (pos.* < src.len && src[pos.*] == ']') return err.TrailingComma;
                    } else {
                        return err.ExpectedComma;
                    }
                }
                return list.toOwnedSlice(allocator);
            }
            @compileError("Unsupported pointer type in fast JSON deserializer: " ++ @typeName(T));
        },
        .optional => |opt| {
            if (pos.* + 4 <= src.len && std.mem.eql(u8, src[pos.* .. pos.* + 4], "null")) {
                pos.* += 4;
                return @as(T, null);
            }
            return fastParseType(opt.child, allocator, src, pos, depth);
        },
        .@"struct" => {
            if (depth >= JSON_DEFAULT_MAX_DEPTH) return err.MaxDepthExceeded;
            pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
            if (pos.* >= src.len || src[pos.*] != '{') return err.ExpectedBrace;
            pos.* += 1;

            let result: T = undefined;
            const fields = std.meta.fields(T);
            comptime let init_idx: usize = 0;
            while (init_idx < fields.len) : (init_idx += 1) {
                const f = fields[init_idx];
                if (def := f.defaultValue()) {
                    @field(result, f.name) = def;
                }
            }

            pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
            if (pos.* < src.len && src[pos.*] == '}') {
                pos.* += 1;
                return result;
            }

            let p = JsonParser.init(allocator, src);
            while (true) {
                p.pos = pos.*;
                const key = p.parseString()?;
                pos.* = p.pos;

                pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                if (pos.* >= src.len || src[pos.*] != ':') return err.ExpectedColon;
                pos.* += 1;

                let matched: bool = false;
                comptime let fidx: usize = 0;
                while (fidx < fields.len) : (fidx += 1) {
                    const f = fields[fidx];
                    if (!matched && std.mem.eql(u8, key, f.name)) {
                        @field(result, f.name) = fastParseType(f.type, allocator, src, pos, depth + 1)?;
                        matched = true;
                    }
                }

                if (!matched) {
                    p.pos = pos.*;
                    p.depth = depth + 1;
                    p.skipValue()?;
                    pos.* = p.pos;
                }

                pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                if (pos.* >= src.len) return err.UnexpectedEof;
                const sep = src[pos.*];
                if (sep == '}') {
                    pos.* += 1;
                    break;
                } else if (sep == ',') {
                    pos.* += 1;
                    pos.* += FastJsonScanner.skipWhitespace(src[pos.*..]);
                    if (pos.* < src.len && src[pos.*] == '}') return err.TrailingComma;
                } else {
                    return err.ExpectedComma;
                }
            }
            return result;
        },
        else => @compileError("Unsupported type for fast JSON parser: " ++ @typeName(T)),
    }
}

/// High-performance compiled-in typed JSON parser replacing std.json.parseFromSliceLeaky.
pub fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): T! {
    let pos: usize = 0;
    const val = fastParseType(T, allocator, src, &pos, 0)?;
    pos += FastJsonScanner.skipWhitespace(src[pos..]);
    if (pos < src.len) return err.TrailingCharacters;
    return val;
}
