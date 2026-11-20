// Source: bun/src/interchange/json5.cp -> cp/lib/interchange/json5.cp (Pure CP JSON / JSONC / JSON5 Parser & Serializer)
// Zero external dependency JSON, JSONC, and JSON5 parser/serializer with depth guard and zero-leak cleanup.

const std = @import("std");
import {
    INTERCHANGE_MAX_DEPTH,
    InterchangeFormat,
    InterchangeError,
    InterchangeEntry,
    InterchangeValue,
} from "./value.cp";

pub class Json5Codec {
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,
    allow_comments: bool = false,
    allow_trailing_comma: bool = false,
    allow_json5: bool = false,
    alloc: std.mem.Allocator,

    pub static fn parse(
        alloc: std.mem.Allocator,
        input: []const u8,
        format: InterchangeFormat,
    ): InterchangeValue! {
        let parser = Json5Codec{
            src: input,
            pos: 0,
            depth: 0,
            allow_comments: format == .Jsonc || format == .Json5,
            allow_trailing_comma: format == .Jsonc || format == .Json5,
            allow_json5: format == .Json5,
            alloc: alloc,
        };
        if (parser.src.len >= 3 && parser.src[0] == 0xEF && parser.src[1] == 0xBB && parser.src[2] == 0xBF) {
            parser.pos = 3;
        }
        parser.skipWsAndComments()?;
        if (parser.pos >= parser.src.len) {
            return InterchangeError.UnexpectedEof;
        }
        let val = parser.parseValue()?;
        errdefer val.deinit(alloc);
        parser.skipWsAndComments()?;
        if (parser.pos < parser.src.len) {
            return InterchangeError.TrailingData;
        }
        return val;
    }

    fn isWs(c: u8): bool {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return true;
        if (_.allow_json5 && (c == 0x0B || c == 0x0C)) return true;
        return false;
    }

    fn skipWsAndComments(*_): ! {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (_.isWs(c)) {
                _.pos += 1;
                continue;
            }
            if (_.allow_comments && c == '/' && _.pos + 1 < _.src.len) {
                const next = _.src[_.pos + 1];
                if (next == '/') {
                    _.pos += 2;
                    while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (next == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < _.src.len) {
                        if (_.src[_.pos] == '*' && _.src[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) {
                        return InterchangeError.UnterminatedComment;
                    }
                    continue;
                }
            }
            break;
        }
    }

    fn parseValue(*_): InterchangeValue! {
        _.skipWsAndComments()?;
        if (_.pos >= _.src.len) return InterchangeError.UnexpectedEof;

        const c = _.src[_.pos];
        if (c == '{') return _.parseObject();
        if (c == '[') return _.parseArray();
        if (c == '"') {
            const s = _.parseQuotedString('"')?;
            return InterchangeValue{ kind: .String, str_val: s };
        }
        if (c == '\'' && _.allow_json5) {
            const s = _.parseQuotedString('\'')?;
            return InterchangeValue{ kind: .String, str_val: s };
        }
        if (c == 't' && _.matchKeyword("true")) {
            _.pos += 4;
            return InterchangeValue.fromBool(true);
        }
        if (c == 'f' && _.matchKeyword("false")) {
            _.pos += 5;
            return InterchangeValue.fromBool(false);
        }
        if (c == 'n' && _.matchKeyword("null")) {
            _.pos += 4;
            return InterchangeValue.initNull();
        }
        if (_.allow_json5) {
            if (c == 'I' && _.matchKeyword("Infinity")) {
                _.pos += 8;
                return InterchangeValue.fromFloat(std.math.inf(f64));
            }
            if (c == 'N' && _.matchKeyword("NaN")) {
                _.pos += 3;
                return InterchangeValue.fromFloat(std.math.nan(f64));
            }
        }
        if (c == '-' || (c >= '0' && c <= '9') || (_.allow_json5 && (c == '+' || c == '.'))) {
            return _.parseNumber();
        }
        return InterchangeError.UnexpectedChar;
    }

    fn matchKeyword(kw: []const u8): bool {
        if (_.pos + kw.len > _.src.len) return false;
        if (!std.mem.eql(u8, _.src[_.pos .. _.pos + kw.len], kw)) return false;
        if (_.pos + kw.len == _.src.len) return true;
        const next = _.src[_.pos + kw.len];
        if (Json5Codec.isIdentContinue(next)) return false;
        return true;
    }

    static fn isIdentStart(c: u8): bool {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '$';
    }

    static fn isIdentContinue(c: u8): bool {
        return Json5Codec.isIdentStart(c) || (c >= '0' && c <= '9');
    }

    static fn isHexDigit(c: u8): bool {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static fn hexVal(c: u8): u8 {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - 'A' + 10;
    }

    fn parseHex4(*_): u16! {
        if (_.pos + 4 > _.src.len) return InterchangeError.InvalidEscape;
        let v: u16 = 0;
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            const c = _.src[_.pos + i];
            if (!Json5Codec.isHexDigit(c)) return InterchangeError.InvalidEscape;
            v = (v << 4) | @as(u16, Json5Codec.hexVal(c));
        }
        _.pos += 4;
        return v;
    }

    fn parseQuotedString(*_, quote: u8): ([]const u8)! {
        _.pos += 1; // skip opening quote
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == quote) {
                _.pos += 1;
                if (buf.items.len == 0) {
                    buf.deinit(_.alloc);
                    return "";
                }
                return buf.toOwnedSlice(_.alloc) catch return InterchangeError.OutOfMemory;
            }
            if (c == '\n' || c == '\r') {
                return InterchangeError.UnterminatedString;
            }
            if (c < 0x20 && !_.allow_json5) {
                return InterchangeError.UnexpectedChar;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return InterchangeError.UnterminatedString;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => buf.append(_.alloc, '"') catch return InterchangeError.OutOfMemory,
                    '\\' => buf.append(_.alloc, '\\') catch return InterchangeError.OutOfMemory,
                    '/' => buf.append(_.alloc, '/') catch return InterchangeError.OutOfMemory,
                    'b' => buf.append(_.alloc, 0x08) catch return InterchangeError.OutOfMemory,
                    'f' => buf.append(_.alloc, 0x0C) catch return InterchangeError.OutOfMemory,
                    'n' => buf.append(_.alloc, '\n') catch return InterchangeError.OutOfMemory,
                    'r' => buf.append(_.alloc, '\r') catch return InterchangeError.OutOfMemory,
                    't' => buf.append(_.alloc, '\t') catch return InterchangeError.OutOfMemory,
                    '\'' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                        buf.append(_.alloc, '\'') catch return InterchangeError.OutOfMemory;
                    },
                    'v' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                        buf.append(_.alloc, 0x0B) catch return InterchangeError.OutOfMemory;
                    },
                    '0' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                        if (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                            return InterchangeError.InvalidEscape;
                        }
                        buf.append(_.alloc, 0) catch return InterchangeError.OutOfMemory;
                    },
                    '\n' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                    },
                    '\r' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                            _.pos += 1;
                        }
                    },
                    'x' => {
                        if (!_.allow_json5) return InterchangeError.InvalidEscape;
                        if (_.pos + 2 > _.src.len) return InterchangeError.InvalidEscape;
                        const h1 = _.src[_.pos];
                        const h2 = _.src[_.pos + 1];
                        if (!Json5Codec.isHexDigit(h1) || !Json5Codec.isHexDigit(h2)) {
                            return InterchangeError.InvalidEscape;
                        }
                        _.pos += 2;
                        const byte: u8 = (Json5Codec.hexVal(h1) << 4) | Json5Codec.hexVal(h2);
                        let utf8_buf: [4]u8 = undefined;
                        const n = std.unicode.utf8Encode(@as(u21, byte), &utf8_buf) catch return InterchangeError.InvalidEscape;
                        buf.appendSlice(_.alloc, utf8_buf[0..n]) catch return InterchangeError.OutOfMemory;
                    },
                    'u' => {
                        const code1 = _.parseHex4()?;
                        let cp: u21 = @as(u21, code1);
                        if (code1 >= 0xD800 && code1 <= 0xDBFF) {
                            if (_.pos + 2 <= _.src.len && _.src[_.pos] == '\\' && _.src[_.pos + 1] == 'u') {
                                _.pos += 2;
                                const code2 = _.parseHex4()?;
                                if (code2 >= 0xDC00 && code2 <= 0xDFFF) {
                                    cp = 0x10000 + ((@as(u21, code1 - 0xD800) << 10) | @as(u21, code2 - 0xDC00));
                                } else {
                                    return InterchangeError.InvalidEscape;
                                }
                            } else {
                                return InterchangeError.InvalidEscape;
                            }
                        }
                        let utf8_buf: [4]u8 = undefined;
                        const n = std.unicode.utf8Encode(cp, &utf8_buf) catch return InterchangeError.InvalidEscape;
                        buf.appendSlice(_.alloc, utf8_buf[0..n]) catch return InterchangeError.OutOfMemory;
                    },
                    else => {
                        if (_.allow_json5 && !(esc >= '1' && esc <= '9')) {
                            buf.append(_.alloc, esc) catch return InterchangeError.OutOfMemory;
                        } else {
                            return InterchangeError.InvalidEscape;
                        }
                    },
                }
            } else {
                buf.append(_.alloc, c) catch return InterchangeError.OutOfMemory;
                _.pos += 1;
            }
        }
        return InterchangeError.UnterminatedString;
    }

    fn parseNumber(*_): InterchangeValue! {
        const start = _.pos;
        let negative = false;
        if (_.src[_.pos] == '-') {
            negative = true;
            _.pos += 1;
        } else if (_.src[_.pos] == '+') {
            if (!_.allow_json5) return InterchangeError.UnexpectedChar;
            _.pos += 1;
        }

        if (_.pos >= _.src.len) return InterchangeError.InvalidNumber;

        if (_.allow_json5) {
            if (_.src[_.pos] == 'I' && _.matchKeyword("Infinity")) {
                _.pos += 8;
                return InterchangeValue.fromFloat(if (negative) -std.math.inf(f64) else std.math.inf(f64));
            }
            if (_.src[_.pos] == 'N' && _.matchKeyword("NaN")) {
                _.pos += 3;
                return InterchangeValue.fromFloat(std.math.nan(f64));
            }
            if (_.pos + 2 <= _.src.len && _.src[_.pos] == '0' && (_.src[_.pos + 1] == 'x' || _.src[_.pos + 1] == 'X')) {
                _.pos += 2;
                const hex_start = _.pos;
                while (_.pos < _.src.len && Json5Codec.isHexDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == hex_start) return InterchangeError.InvalidNumber;
                const hex_digits = _.src[hex_start.._.pos];
                const uval = std.fmt.parseInt(u64, hex_digits, 16) catch {
                    const fval = @as(f64, @floatFromInt(std.fmt.parseInt(u128, hex_digits, 16) catch return InterchangeError.InvalidNumber));
                    return InterchangeValue.fromFloat(if (negative) -fval else fval);
                };
                if (negative) {
                    if (uval <= 9223372036854775808) {
                        if (uval == 9223372036854775808) return InterchangeValue.fromInt(-9223372036854775808);
                        return InterchangeValue.fromInt(-@as(i64, @intCast(uval)));
                    }
                    return InterchangeValue.fromFloat(-@as(f64, @floatFromInt(uval)));
                } else {
                    if (uval <= 9223372036854775807) {
                        return InterchangeValue.fromInt(@as(i64, @intCast(uval)));
                    }
                    return InterchangeValue.fromFloat(@as(f64, @floatFromInt(uval)));
                }
            }
        }

        let has_int_digits = false;
        let has_dot = false;
        let has_exp = false;

        if (_.src[_.pos] == '0') {
            has_int_digits = true;
            _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                return InterchangeError.InvalidNumber;
            }
        } else if (_.src[_.pos] >= '1' && _.src[_.pos] <= '9') {
            has_int_digits = true;
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        } else if (!_.allow_json5 || _.src[_.pos] != '.') {
            return InterchangeError.InvalidNumber;
        }

        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            has_dot = true;
            _.pos += 1;
            let frac_digits: usize = 0;
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                frac_digits += 1;
                _.pos += 1;
            }
            if (!_.allow_json5 && frac_digits == 0) {
                return InterchangeError.InvalidNumber;
            }
            if (!has_int_digits && frac_digits == 0) {
                return InterchangeError.InvalidNumber;
            }
        }

        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            has_exp = true;
            _.pos += 1;
            if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                _.pos += 1;
            }
            let exp_digits: usize = 0;
            while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
                exp_digits += 1;
                _.pos += 1;
            }
            if (exp_digits == 0) return InterchangeError.InvalidNumber;
        }

        const raw = _.src[start.._.pos];
        if (!has_dot && !has_exp) {
            const clean = if (raw.len > 0 && raw[0] == '+') raw[1..] else raw;
            if (iv := (std.fmt.parseInt(i64, clean, 10) catch null)) {
                return InterchangeValue.fromInt(iv);
            }
        }
        const fv = std.fmt.parseFloat(f64, raw) catch return InterchangeError.InvalidNumber;
        return InterchangeValue.fromFloat(fv);
    }

    fn parseArray(*_): InterchangeValue! {
        if (_.depth >= INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // skip '['
        let items: std.ArrayList(InterchangeValue) = .empty;
        errdefer {
            for (*it of items.items) {
                it.deinit(_.alloc);
            }
            items.deinit(_.alloc);
        }

        _.skipWsAndComments()?;
        if (_.pos < _.src.len && _.src[_.pos] == ']') {
            _.pos += 1;
            items.deinit(_.alloc);
            return InterchangeValue.fromArray([]);
        }

        while (true) {
            let elem = _.parseValue()?;
            errdefer elem.deinit(_.alloc);
            items.append(_.alloc, elem) catch return InterchangeError.OutOfMemory;

            _.skipWsAndComments()?;
            if (_.pos >= _.src.len) return InterchangeError.UnexpectedEof;
            const c = _.src[_.pos];
            if (c == ']') {
                _.pos += 1;
                break;
            }
            if (c == ',') {
                _.pos += 1;
                _.skipWsAndComments()?;
                if (_.allow_trailing_comma && _.pos < _.src.len && _.src[_.pos] == ']') {
                    _.pos += 1;
                    break;
                }
                continue;
            }
            return InterchangeError.UnexpectedChar;
        }

        const slice = items.toOwnedSlice(_.alloc) catch return InterchangeError.OutOfMemory;
        return InterchangeValue.fromArray(slice);
    }

    fn parseObjectKey(*_): ([]const u8)! {
        _.skipWsAndComments()?;
        if (_.pos >= _.src.len) return InterchangeError.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') return _.parseQuotedString('"');
        if (c == '\'' && _.allow_json5) return _.parseQuotedString('\'');
        if (_.allow_json5 && Json5Codec.isIdentStart(c)) {
            const start = _.pos;
            _.pos += 1;
            while (_.pos < _.src.len && Json5Codec.isIdentContinue(_.src[_.pos])) {
                _.pos += 1;
            }
            const key = _.src[start.._.pos];
            if (key.len == 0) return "";
            return _.alloc.dupe(u8, key) catch return InterchangeError.OutOfMemory;
        }
        return InterchangeError.UnexpectedChar;
    }

    fn parseObject(*_): InterchangeValue! {
        if (_.depth >= INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // skip '{'
        let entries: std.ArrayList(InterchangeEntry) = .empty;
        errdefer {
            for (*e of entries.items) {
                if (e.key.len > 0) _.alloc.free(e.key);
                e.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }

        _.skipWsAndComments()?;
        if (_.pos < _.src.len && _.src[_.pos] == '}') {
            _.pos += 1;
            entries.deinit(_.alloc);
            return InterchangeValue.fromObject([]);
        }

        while (true) {
            const key = _.parseObjectKey()?;
            errdefer {
                if (key.len > 0) _.alloc.free(key);
            }

            _.skipWsAndComments()?;
            if (_.pos >= _.src.len) return InterchangeError.UnexpectedEof;
            if (_.src[_.pos] != ':') return InterchangeError.UnexpectedChar;
            _.pos += 1;

            let val = _.parseValue()?;
            errdefer val.deinit(_.alloc);

            entries.append(_.alloc, InterchangeEntry{ key: key, value: val }) catch return InterchangeError.OutOfMemory;

            _.skipWsAndComments()?;
            if (_.pos >= _.src.len) return InterchangeError.UnexpectedEof;
            const c = _.src[_.pos];
            if (c == '}') {
                _.pos += 1;
                break;
            }
            if (c == ',') {
                _.pos += 1;
                _.skipWsAndComments()?;
                if (_.allow_trailing_comma && _.pos < _.src.len && _.src[_.pos] == '}') {
                    _.pos += 1;
                    break;
                }
                continue;
            }
            return InterchangeError.UnexpectedChar;
        }

        const slice = entries.toOwnedSlice(_.alloc) catch return InterchangeError.OutOfMemory;
        return InterchangeValue.fromObject(slice);
    }

    pub static fn stringify(
        alloc: std.mem.Allocator,
        val: InterchangeValue,
        format: InterchangeFormat,
        pretty: bool,
    ): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        const allow_non_finite = format == .Json5;
        Json5Codec.writeValue(alloc, &out, val, 0, pretty, allow_non_finite)?;
        return out.toOwnedSlice(alloc) catch return InterchangeError.OutOfMemory;
    }

    static fn writeIndent(alloc: std.mem.Allocator, out: *std.ArrayList(u8), level: usize): ! {
        let i: usize = 0;
        while (i < level) : (i += 1) {
            out.appendSlice(alloc, "  ") catch return InterchangeError.OutOfMemory;
        }
    }

    static fn writeEscapedString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
        out.append(alloc, '"') catch return InterchangeError.OutOfMemory;
        for (c of s) {
            switch (c) {
                '"' => out.appendSlice(alloc, "\\\"") catch return InterchangeError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return InterchangeError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return InterchangeError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return InterchangeError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return InterchangeError.OutOfMemory,
                0x08 => out.appendSlice(alloc, "\\b") catch return InterchangeError.OutOfMemory,
                0x0C => out.appendSlice(alloc, "\\f") catch return InterchangeError.OutOfMemory,
                else => {
                    if (c < 0x20) {
                        const hex = "0123456789abcdef";
                        out.appendSlice(alloc, "\\u00") catch return InterchangeError.OutOfMemory;
                        out.append(alloc, hex[(c >> 4) & 0x0F]) catch return InterchangeError.OutOfMemory;
                        out.append(alloc, hex[c & 0x0F]) catch return InterchangeError.OutOfMemory;
                    } else {
                        out.append(alloc, c) catch return InterchangeError.OutOfMemory;
                    }
                },
            }
        }
        out.append(alloc, '"') catch return InterchangeError.OutOfMemory;
    }

    static fn writeValue(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        val: InterchangeValue,
        depth: usize,
        pretty: bool,
        allow_non_finite: bool,
    ): ! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (val.kind) {
            .Null => out.appendSlice(alloc, "null") catch return InterchangeError.OutOfMemory,
            .Bool => out.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return InterchangeError.OutOfMemory,
            .Int => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch return InterchangeError.OutOfMemory;
                out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    out.appendSlice(alloc, if (allow_non_finite) "NaN" else "null") catch return InterchangeError.OutOfMemory;
                } else if (std.math.isInf(val.float_val)) {
                    if (allow_non_finite) {
                        out.appendSlice(alloc, if (val.float_val < 0) "-Infinity" else "Infinity") catch return InterchangeError.OutOfMemory;
                    } else {
                        out.appendSlice(alloc, "null") catch return InterchangeError.OutOfMemory;
                    }
                } else {
                    let buf: [64]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch return InterchangeError.OutOfMemory;
                    out.appendSlice(alloc, s) catch return InterchangeError.OutOfMemory;
                }
            },
            .String => Json5Codec.writeEscapedString(alloc, out, val.str_val)?,
            .Array => {
                if (val.arr_val.len == 0) {
                    out.appendSlice(alloc, "[]") catch return InterchangeError.OutOfMemory;
                    return;
                }
                out.append(alloc, '[') catch return InterchangeError.OutOfMemory;
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) out.append(alloc, ',') catch return InterchangeError.OutOfMemory;
                    if (pretty) {
                        out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                        Json5Codec.writeIndent(alloc, out, depth + 1)?;
                    }
                    Json5Codec.writeValue(alloc, out, val.arr_val[i], depth + 1, pretty, allow_non_finite)?;
                }
                if (pretty) {
                    out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                    Json5Codec.writeIndent(alloc, out, depth)?;
                }
                out.append(alloc, ']') catch return InterchangeError.OutOfMemory;
            },
            .Object => {
                if (val.obj_val.len == 0) {
                    out.appendSlice(alloc, "{}") catch return InterchangeError.OutOfMemory;
                    return;
                }
                out.append(alloc, '{') catch return InterchangeError.OutOfMemory;
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) out.append(alloc, ',') catch return InterchangeError.OutOfMemory;
                    if (pretty) {
                        out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                        Json5Codec.writeIndent(alloc, out, depth + 1)?;
                    }
                    Json5Codec.writeEscapedString(alloc, out, val.obj_val[i].key)?;
                    out.appendSlice(alloc, if (pretty) ": " else ":") catch return InterchangeError.OutOfMemory;
                    Json5Codec.writeValue(alloc, out, val.obj_val[i].value, depth + 1, pretty, allow_non_finite)?;
                }
                if (pretty) {
                    out.append(alloc, '\n') catch return InterchangeError.OutOfMemory;
                    Json5Codec.writeIndent(alloc, out, depth)?;
                }
                out.append(alloc, '}') catch return InterchangeError.OutOfMemory;
            },
        }
    }
}
