// Source: Absorbed from /data/dev/bun/src/interchange/json.cp | Synced: 2026-11-22T05:55:50Z
// RFC 8259 Compliant Zero-Copy Streaming & Typed JSON Parser with SIMD Acceleration
// Handles full escape sequences, UTF-16 surrogate pairs, strict numbers, and max depth guards

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "./value";
import { Utf8 } from "cap:string";

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
    max_depth: usize = 512,
    allow_trailing_garbage: bool = false,
}

pub struct FastJsonScanner {
    /// 16-byte SIMD search for either '"' or '\\'
    pub static fn indexOfQuoteOrEscape(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const t1: @Vector(16, u8) = @splat('"');
        const t2: @Vector(16, u8) = @splat('\\');

        while (i + 16 <= n) {
            let chunk: @Vector(16, u8) = @splat(0);
            zig {
                chunk = input[i..][0..16].*;
            }
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const mask: u16 = m1 | m2;
            if (mask != 0) {
                const bit_idx = @ctz(mask) as usize;
                return i + bit_idx;
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
        const t1: @Vector(16, u8) = @splat('"');
        const t2: @Vector(16, u8) = @splat('\\');
        const t_ctrl: @Vector(16, u8) = @splat(0x20);

        while (i + 16 <= n) {
            let chunk: @Vector(16, u8) = @splat(0);
            zig {
                chunk = input[i..][0..16].*;
            }
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const m3: u16 = @bitCast(chunk < t_ctrl);
            const mask: u16 = m1 | m2 | m3;
            if (mask != 0) {
                const bit_idx = @ctz(mask) as usize;
                return i + bit_idx;
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
}

pub struct JsonParser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,
    depth: usize = 0,
    max_depth: usize = 512,

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): JsonParser {
        return JsonParser{
            .src = src,
            .pos = 0,
            .allocator = allocator,
            .depth = 0,
            .max_depth = 512,
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
        while (pos < src.len) {
            const b = src[pos];
            if (b == ' ' || b == '\t' || b == '\r' || b == '\n') {
                pos += 1;
            } else {
                break;
            }
        }
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

    /// SIMD-accelerated RFC 8259 string parser:
    /// - Zero-copy slice borrow when no backslash escapes are present
    /// - Full unescaping including `\uXXXX` and UTF-16 surrogate pairs (`\uD83D\uDE00` -> UTF-8)
    /// - Rejects unescaped ASCII control chars (< 0x20) and invalid escapes
    pub fn parseString(*_): ([]const u8)! {
        skipWhitespace();
        if (pos >= src.len || src[pos] != '"') {
            return err.InvalidToken;
        }
        pos += 1; // skip opening quote
        const start = pos;

        // Fast path: check first special byte
        const first_hit = FastJsonScanner.indexOfQuoteEscapeOrCtrl(src[pos..]) ?? return err.UnexpectedEof;
        const hit_pos = pos + first_hit;
        const hit_ch = src[hit_pos];
        if (hit_ch < 0x20) {
            return err.InvalidControlChar;
        }
        if (hit_ch == '"') {
            pos = hit_pos + 1;
            return src[start..hit_pos];
        }

        // Slow path: string contains backslash escape sequences
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        if (hit_pos > start) {
            out.appendSlice(allocator, src[start..hit_pos])?;
        }
        pos = hit_pos;

        while (pos < src.len) {
            const rem = src[pos..];
            const seg_hit = FastJsonScanner.indexOfQuoteEscapeOrCtrl(rem) ?? return err.UnexpectedEof;
            if (seg_hit > 0) {
                out.appendSlice(allocator, rem[0..seg_hit])?;
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
        return std.fmt.parseFloat(f64, num_str) catch return err.InvalidToken;
    }

    /// Fast skip over an arbitrary JSON value without heap allocations
    pub fn skipValue(*_): ! {
        skipWhitespace();
        if (pos >= src.len) return err.UnexpectedEof;
        const c = src[pos];
        if (c == '"') {
            * = parseString()?;
            return;
        }
        if (c == '{' || c == '[') {
            let depth: usize = 0;
            while (pos < src.len) {
                const cur = src[pos];
                if (cur == '"') {
                    * = parseString()?;
                    continue;
                }
                if (cur == '{' || cur == '[') {
                    depth += 1;
                    if (depth > max_depth) return err.MaxDepthExceeded;
                } else if (cur == '}' || cur == ']') {
                    if (depth == 0) return err.InvalidToken;
                    depth -= 1;
                    if (depth == 0) {
                        pos += 1;
                        return;
                    }
                }
                pos += 1;
            }
            return err.UnexpectedEof;
        }
        if (c == 'n') {
            if (pos + 4 <= src.len && std.mem.eql(u8, src[pos .. pos + 4], "null")) {
                pos += 4;
                return;
            }
            return err.InvalidToken;
        }
        if (c == 't') {
            if (pos + 4 <= src.len && std.mem.eql(u8, src[pos .. pos + 4], "true")) {
                pos += 4;
                return;
            }
            return err.InvalidToken;
        }
        if (c == 'f') {
            if (pos + 5 <= src.len && std.mem.eql(u8, src[pos .. pos + 5], "false")) {
                pos += 5;
                return;
            }
            return err.InvalidToken;
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            * = parseNumber()?;
            return;
        }
        return err.InvalidToken;
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

    /// High-performance compiled-in typed JSON parser
    pub static fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): T! {
        zig {
            var pos: usize = 0;
            const val = try fastParseType(T, allocator, src, &pos, 0);
            while (pos < src.len and (src[pos] == ' ' or src[pos] == '\t' or src[pos] == '\r' or src[pos] == '\n')) pos += 1;
            if (pos < src.len) return error.TrailingCharacters;
            return val;
        }
    }
}

zig {
    pub fn fastParseType(comptime T: type, allocator: std.mem.Allocator, src: []const u8, pos: *usize, depth: usize) anyerror!T {
        while (pos.* < src.len) {
            const b = src[pos.*];
            if (b == ' ' or b == '\t' or b == '\r' or b == '\n') {
                pos.* += 1;
            } else {
                break;
            }
        }
        if (pos.* >= src.len) return error.UnexpectedEof;

        const info = @typeInfo(T);

        switch (info) {
            .bool => {
                if (pos.* + 4 <= src.len and std.mem.eql(u8, src[pos.*..pos.* + 4], "true")) {
                    pos.* += 4;
                    return true;
                }
                if (pos.* + 5 <= src.len and std.mem.eql(u8, src[pos.*..pos.* + 5], "false")) {
                    pos.* += 5;
                    return false;
                }
                return error.InvalidToken;
            },
            .int => {
                const start = pos.*;
                if (pos.* < src.len and src[pos.*] == '-') {
                    pos.* += 1;
                }
                if (pos.* >= src.len) return error.InvalidToken;
                if (src[pos.*] == '0') {
                    pos.* += 1;
                    if (pos.* < src.len and src[pos.*] >= '0' and src[pos.*] <= '9') return error.InvalidToken;
                } else if (src[pos.*] >= '1' and src[pos.*] <= '9') {
                    pos.* += 1;
                    while (pos.* < src.len and src[pos.*] >= '0' and src[pos.*] <= '9') pos.* += 1;
                } else {
                    return error.InvalidToken;
                }
                const num_slice = src[start..pos.*];
                return std.fmt.parseInt(T, num_slice, 10) catch return error.InvalidToken;
            },
            .float => {
                var p = JsonParser.init(allocator, src);
                p.pos = pos.*;
                const f = try p.parseNumber();
                pos.* = p.pos;
                return @floatCast(f);
            },
            .pointer => |ptr| {
                if (ptr.size == .slice and ptr.child == u8) {
                    var p = JsonParser.init(allocator, src);
                    p.pos = pos.*;
                    const s = try p.parseString();
                    pos.* = p.pos;
                    return s;
                }
                if (ptr.size == .slice) {
                    if (depth >= 512) return error.MaxDepthExceeded;
                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* >= src.len or src[pos.*] != '[') return error.InvalidToken;
                    pos.* += 1; // skip '['

                    var list: std.ArrayList(ptr.child) = .empty;
                    errdefer list.deinit(allocator);

                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* < src.len and src[pos.*] == ']') {
                        pos.* += 1;
                        return list.toOwnedSlice(allocator);
                    }

                    while (true) {
                        const item = try fastParseType(ptr.child, allocator, src, pos, depth + 1);
                        try list.append(allocator, item);

                        while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                        if (pos.* >= src.len) return error.UnexpectedEof;
                        const sep = src[pos.*];
                        if (sep == ']') {
                            pos.* += 1;
                            break;
                        } else if (sep == ',') {
                            pos.* += 1;
                            while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                            if (pos.* < src.len and src[pos.*] == ']') return error.TrailingComma;
                        } else {
                            return error.ExpectedComma;
                        }
                    }
                    return list.toOwnedSlice(allocator);
                }
                @compileError("Unsupported pointer type in fast JSON deserializer: " ++ @typeName(T));
            },
            .optional => |opt| {
                if (pos.* + 4 <= src.len and std.mem.eql(u8, src[pos.*..pos.* + 4], "null")) {
                    pos.* += 4;
                    return null;
                }
                return try fastParseType(opt.child, allocator, src, pos, depth);
            },
            .@"struct" => |s| {
                if (depth >= 512) return error.MaxDepthExceeded;
                while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                if (pos.* >= src.len or src[pos.*] != '{') return error.ExpectedBrace;
                pos.* += 1; // skip '{'

                var result: T = undefined;
                inline for (s.field_names, 0..) |fname, fidx| {
                    const ftype = s.field_types[fidx];
                    const fattr = s.field_attrs[fidx];
                    if (fattr.defaultValue(ftype)) |def| {
                        @field(result, fname) = def;
                    }
                }

                while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                if (pos.* < src.len and src[pos.*] == '}') {
                    pos.* += 1;
                    return result;
                }

                var p = JsonParser.init(allocator, src);
                while (true) {
                    p.pos = pos.*;
                    const key = try p.parseString();
                    pos.* = p.pos;

                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* >= src.len or src[pos.*] != ':') return error.ExpectedColon;
                    pos.* += 1; // skip ':'

                    var matched = false;
                    inline for (s.field_names, 0..) |fname, fidx| {
                        if (std.mem.eql(u8, key, fname)) {
                            const ftype = s.field_types[fidx];
                            @field(result, fname) = try fastParseType(ftype, allocator, src, pos, depth + 1);
                            matched = true;
                            break;
                        }
                    }

                    if (!matched) {
                        p.pos = pos.*;
                        try p.skipValue();
                        pos.* = p.pos;
                    }

                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* >= src.len) return error.UnexpectedEof;
                    const sep = src[pos.*];
                    if (sep == '}') {
                        pos.* += 1;
                        break;
                    } else if (sep == ',') {
                        pos.* += 1;
                        while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                        if (pos.* < src.len and src[pos.*] == '}') return error.TrailingComma;
                    } else {
                        return error.ExpectedComma;
                    }
                }
                return result;
            },
            else => @compileError("Unsupported type for fast JSON parser: " ++ @typeName(T)),
        }
    }

    /// High-performance compiled-in typed JSON parser replacing std.json.parseFromSliceLeaky
    pub fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8) anyerror!T {
        var pos: usize = 0;
        const val = try fastParseType(T, allocator, src, &pos, 0);
        while (pos < src.len and (src[pos] == ' ' or src[pos] == '\t' or src[pos] == '\r' or src[pos] == '\n')) pos += 1;
        if (pos < src.len) return error.TrailingCharacters;
        return val;
    }
}
