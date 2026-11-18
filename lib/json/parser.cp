// Source: Custom implementation by Antigravity AI (Ultra-fast SIMD zero-copy JSON streaming & typed parser)
// Ultra-fast zero-copy streaming & typed JSON parser in CP with SIMD acceleration
// String keys and values borrow directly from input slice without heap allocation

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "./value";

type Vec16u8 = @Vector(16, u8);

export error JsonError {
    UnexpectedEof,
    InvalidToken,
    ExpectedColon,
    ExpectedComma,
    ExpectedBrace,
    UnknownField,
}

pub struct FastJsonScanner {
    /// 16-byte SIMD search for either '"' or '\\'
    pub static fn indexOfQuoteOrEscape(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const t1: Vec16u8 = @splat('"');
        const t2: Vec16u8 = @splat('\\');

        while (i + 16 <= n) {
            let chunk: Vec16u8 = @splat(0);
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
            if (c == '"' or c == '\\') {
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

    pub static fn init(allocator: std.mem.Allocator, src: []const u8): JsonParser {
        return JsonParser{
            .src = src,
            .pos = 0,
            .allocator = allocator,
        };
    }

    fn skipWhitespace(*_) {
        while (pos < src.len) {
            const b = src[pos];
            if (b == ' ' or b == '\t' or b == '\r' or b == '\n') {
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

    /// SIMD-accelerated string extraction (zero-copy borrow from src)
    pub fn parseString(*_): ![]const u8 {
        skipWhitespace();
        if (pos >= src.len or src[pos] != '"') {
            return error.InvalidToken;
        }
        pos += 1; // skip opening quote
        const start = pos;

        while (pos < src.len) {
            const rem = src[pos..];
            if (hit := FastJsonScanner.indexOfQuoteOrEscape(rem)) {
                pos += hit;
                if (src[pos] == '\\') {
                    pos += 2; // skip escape
                } else {
                    const slice = src[start..pos];
                    pos += 1; // skip closing quote
                    return slice;
                }
            } else {
                return error.UnexpectedEof;
            }
        }
        return error.UnexpectedEof;
    }

    pub fn parseNumber(*_): !f64 {
        skipWhitespace();
        const start = pos;
        if (pos < src.len and (src[pos] == '-' or src[pos] == '+')) {
            pos += 1;
        }
        while (pos < src.len) {
            const b = src[pos];
            if ((b >= '0' and b <= '9') or b == '.' or b == 'e' or b == 'E' or b == '+' or b == '-') {
                pos += 1;
            } else {
                break;
            }
        }
        const num_str = src[start..pos];
        return std.fmt.parseFloat(f64, num_str) catch return error.InvalidToken;
    }

    /// Fast skip over an arbitrary JSON value without heap allocations
    pub fn skipValue(*_): !void {
        skipWhitespace();
        if (pos >= src.len) return error.UnexpectedEof;
        const c = src[pos];
        if (c == '"') {
            * = try parseString();
            return;
        }
        if (c == '{' or c == '[') {
            const open_char = c;
            const close_char: u8 = if (c == '{') '}' else ']';
            pos += 1;
            let depth: usize = 1;
            while (pos < src.len and depth > 0) {
                const cur = src[pos];
                if (cur == '"') {
                    * = try parseString();
                    continue;
                }
                if (cur == open_char) depth += 1;
                if (cur == close_char) depth -= 1;
                pos += 1;
            }
            return;
        }
        // Scalar value: number, bool, null
        while (pos < src.len) {
            const cur = src[pos];
            if (cur == ',' or cur == '}' or cur == ']' or cur == ' ' or cur == '\t' or cur == '\r' or cur == '\n') {
                break;
            }
            pos += 1;
        }
    }

    pub fn parseValue(*_): !JsonValue {
        skipWhitespace();
        if (pos >= src.len) return error.UnexpectedEof;
        const ch = src[pos];

        if (ch == 'n') {
            if (pos + 4 <= src.len and std.mem.eql(u8, src[pos..pos + 4], "null")) {
                pos += 4;
                return JsonValue{ .tag = .Null };
            }
            return error.InvalidToken;
        }

        if (ch == 't') {
            if (pos + 4 <= src.len and std.mem.eql(u8, src[pos..pos + 4], "true")) {
                pos += 4;
                return JsonValue{ .tag = .Bool, .bool_val = true };
            }
            return error.InvalidToken;
        }

        if (ch == 'f') {
            if (pos + 5 <= src.len and std.mem.eql(u8, src[pos..pos + 5], "false")) {
                pos += 5;
                return JsonValue{ .tag = .Bool, .bool_val = false };
            }
            return error.InvalidToken;
        }

        if (ch == '"') {
            const s = try parseString();
            return JsonValue{ .tag = .String, .str_val = s };
        }

        if (ch == '-' or (ch >= '0' and ch <= '9')) {
            const n = try parseNumber();
            return JsonValue{ .tag = .Number, .num_val = n };
        }

        if (ch == '[') {
            return parseArray();
        }

        if (ch == '{') {
            return parseObject();
        }

        return error.InvalidToken;
    }

    fn parseArray(*_): !JsonValue {
        pos += 1; // skip '['
        skipWhitespace();

        let list: std.ArrayList(JsonValue) = .empty;
        errdefer list.deinit(allocator);

        if (next_ch := _.peek()) {
            if (next_ch == ']') {
                pos += 1;
                return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(allocator) };
            }
        }

        while (true) {
            const val = try parseValue();
            try list.append(allocator, val);

            skipWhitespace();
            if (pos >= src.len) return error.UnexpectedEof;
            const sep = src[pos];
            if (sep == ']') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
            } else {
                return error.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(allocator) };
    }

    fn parseObject(*_): !JsonValue {
        pos += 1; // skip '{'
        skipWhitespace();

        let list: std.ArrayList(JsonField) = .empty;
        errdefer list.deinit(allocator);

        if (next_ch := _.peek()) {
            if (next_ch == '}') {
                pos += 1;
                return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(allocator) };
            }
        }

        while (true) {
            const key = try parseString();
            skipWhitespace();
            if (pos >= src.len or src[pos] != ':') {
                return error.ExpectedColon;
            }
            pos += 1; // skip ':'

            const val = try parseValue();
            try list.append(allocator, JsonField{ .key = key, .value = val });

            skipWhitespace();
            if (pos >= src.len) return error.UnexpectedEof;
            const sep = src[pos];
            if (sep == '}') {
                pos += 1;
                break;
            } else if (sep == ',') {
                pos += 1;
            } else {
                return error.ExpectedComma;
            }
        }

        return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(allocator) };
    }

    /// Parse JSON directly into CP dynamic JsonValue AST.
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): !JsonValue {
        let parser = JsonParser.init(allocator, src);
        return parser.parseValue();
    }

    /// High-performance compiled-in typed JSON parser
    pub static fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): !T {
        zig {
            var pos: usize = 0;
            return fastParseType(T, allocator, src, &pos);
        }
    }
}

zig {
    pub fn fastParseType(comptime T: type, allocator: std.mem.Allocator, src: []const u8, pos: *usize) anyerror!T {
        // Skip whitespace
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
                if (pos.* < src.len and (src[pos.*] == '-' or src[pos.*] == '+')) {
                    pos.* += 1;
                }
                while (pos.* < src.len and (src[pos.*] >= '0' and src[pos.*] <= '9')) {
                    pos.* += 1;
                }
                const num_slice = src[start..pos.*];
                return std.fmt.parseInt(T, num_slice, 10) catch return error.InvalidToken;
            },
            .float => {
                const start = pos.*;
                if (pos.* < src.len and (src[pos.*] == '-' or src[pos.*] == '+')) {
                    pos.* += 1;
                }
                while (pos.* < src.len) {
                    const b = src[pos.*];
                    if ((b >= '0' and b <= '9') or b == '.' or b == 'e' or b == 'E' or b == '+' or b == '-') {
                        pos.* += 1;
                    } else {
                        break;
                    }
                }
                const num_slice = src[start..pos.*];
                return std.fmt.parseFloat(T, num_slice) catch return error.InvalidToken;
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
                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* >= src.len or src[pos.*] != '[') return error.InvalidToken;
                    pos.* += 1; // skip '['

                    var list = std.ArrayList(ptr.child).init(allocator);
                    errdefer list.deinit();

                    while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                    if (pos.* < src.len and src[pos.*] == ']') {
                        pos.* += 1;
                        return list.toOwnedSlice();
                    }

                    while (true) {
                        const item = try fastParseType(ptr.child, allocator, src, pos);
                        try list.append(item);

                        while (pos.* < src.len and (src[pos.*] == ' ' or src[pos.*] == '\t' or src[pos.*] == '\r' or src[pos.*] == '\n')) pos.* += 1;
                        if (pos.* >= src.len) return error.UnexpectedEof;
                        const sep = src[pos.*];
                        if (sep == ']') {
                            pos.* += 1;
                            break;
                        } else if (sep == ',') {
                            pos.* += 1;
                        } else {
                            return error.ExpectedComma;
                        }
                    }
                    return list.toOwnedSlice();
                }
                @compileError("Unsupported pointer type in fast JSON deserializer: " ++ @typeName(T));
            },
            .optional => |opt| {
                if (pos.* + 4 <= src.len and std.mem.eql(u8, src[pos.*..pos.* + 4], "null")) {
                    pos.* += 4;
                    return null;
                }
                return try fastParseType(opt.child, allocator, src, pos);
            },
            .@"struct" => |s| {
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
                            @field(result, fname) = try fastParseType(ftype, allocator, src, pos);
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
        return try fastParseType(T, allocator, src, &pos);
    }
}
