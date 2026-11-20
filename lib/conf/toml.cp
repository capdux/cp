// Source: /data/dev/bun/src/interchange/toml/
// lib/conf/toml.cp
// Industrial-grade TOML 1.0 Parser, AST Builder, Serializer, and Document Model:
// - Inline tables (`point = { x = 1, y = 2 }`), nested dotted keys (`a.b.c = 1`), quoted keys (`"a.b" = 1`)
// - RFC 3339 Date/Time literals (Offset Date-Time, Local Date-Time, Local Date, Local Time) with calendar validation
// - Multi-line basic/literal strings (`"""..."""`, `'''...'''`) & multi-dimensional arrays
// - Table arrays (`[[products]]`) & nested sub-tables
// - 100% backward compatible `TomlDoc` (`TOML` / `toml`) + full recursive `TomlParser` -> `ConfValue`

const std = @import("std");
import { ConfTag, ConfDateTime, ConfField, ConfValue } from "./value";

pub error TomlError {
    OutOfMemory,
    UnexpectedEof,
    SyntaxError,
    DuplicateKey,
    TableRedefined,
    InvalidEscape,
    InvalidNumber,
    InvalidDateTime,
}

type TomlFieldList = std.ArrayListUnmanaged(ConfField);
type TomlValueList = std.ArrayListUnmanaged(ConfValue);
type TomlByteList = std.ArrayListUnmanaged(u8);
type TomlStrList = std.ArrayListUnmanaged(string);

pub struct TomlEntry {
    pub table: []const u8 = "";
    pub key: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct TomlParser {
    src: []const u8 = "",
    pos: usize = 0,
    allocator: std.mem.Allocator = undefined,

    pub static fn parse(allocator: std.mem.Allocator, content: []const u8): ConfValue! {
        let p = TomlParser{
            src: content,
            pos: 0,
            allocator: allocator,
        };
        return try p.runParser();
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
            _.pos += 1;
        }
    }

    fn skipWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else if (c == '#') {
                while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn skipCommentOnLine(*_) {
        _.skipWs();
        if (_.pos < _.src.len && _.src[_.pos] == '#') {
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
        }
    }

    fn expectLineEndOrEof(*_): ! {
        _.skipCommentOnLine();
        if (_.pos >= _.src.len) return;
        if (_.src[_.pos] == '\r') _.pos += 1;
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
            return;
        }
        if (_.pos >= _.src.len) return;
        return error.SyntaxError;
    }

    fn isBareKeyChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    fn parseKeySegment(*_): ([]const u8)! {
        _.skipWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') return try _.parseBasicString();
        if (c == '\'') return try _.parseLiteralString();
        const start = _.pos;
        while (_.pos < _.src.len && TomlParser.isBareKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == start) return error.SyntaxError;
        return _.src[start.._.pos];
    }

    fn parseKeyPath(*_, seg_buf: *[32][]const u8): usize! {
        let count: usize = 0;
        const first = try _.parseKeySegment();
        seg_buf[0] = first;
        count = 1;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '.') break;
            _.pos += 1;
            if (count >= 32) return error.SyntaxError;
            seg_buf[count] = try _.parseKeySegment();
            count += 1;
        }
        return count;
    }

    fn parseHex(s: []const u8): u32? {
        let acc: u32 = 0;
        for (c of s) {
            acc <<= 4;
            if (c >= '0' && c <= '9') {
                acc |= (c - '0') as u32;
            } else if (c >= 'a' && c <= 'f') {
                acc |= ((c - 'a') + 10) as u32;
            } else if (c >= 'A' && c <= 'F') {
                acc |= ((c - 'A') + 10) as u32;
            } else {
                return null;
            }
        }
        return acc;
    }

    fn appendUtf8(out: *TomlByteList, allocator: std.mem.Allocator, cp: u32): ! {
        if (cp > 0x10FFFF) return error.InvalidEscape;
        let buf: [4]u8 = undefined;
        const n = std.unicode.utf8Encode(@intCast(cp), &buf) catch return error.InvalidEscape;
        try out.appendSlice(allocator, buf[0..n]);
    }

    fn parseBasicEscape(*_, out: *TomlByteList): ! {
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const esc = _.src[_.pos];
        _.pos += 1;
        switch (esc) {
            'b' => try out.append(_.allocator, 0x08),
            't' => try out.append(_.allocator, '\t'),
            'n' => try out.append(_.allocator, '\n'),
            'f' => try out.append(_.allocator, 0x0C),
            'r' => try out.append(_.allocator, '\r'),
            '"' => try out.append(_.allocator, '"'),
            '\\' => try out.append(_.allocator, '\\'),
            'u' => {
                if (_.pos + 4 > _.src.len) return error.InvalidEscape;
                const cp = TomlParser.parseHex(_.src[_.pos .. _.pos + 4]) orelse return error.InvalidEscape;
                _.pos += 4;
                try TomlParser.appendUtf8(out, _.allocator, cp);
            },
            'U' => {
                if (_.pos + 8 > _.src.len) return error.InvalidEscape;
                const cp = TomlParser.parseHex(_.src[_.pos .. _.pos + 8]) orelse return error.InvalidEscape;
                _.pos += 8;
                try TomlParser.appendUtf8(out, _.allocator, cp);
            },
            else => return error.InvalidEscape,
        }
    }

    fn parseBasicString(*_): ([]const u8)! {
        _.pos += 1; // '"'
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '"' && _.src[_.pos + 1] == '"') {
            _.pos += 2;
            return try _.parseMultilineBasicString();
        }
        let out = TomlByteList.empty;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return try out.toOwnedSlice(_.allocator);
            }
            if (c == '\n' || c == '\r') return error.SyntaxError;
            if (c == '\\') {
                _.pos += 1;
                try _.parseBasicEscape(&out);
                continue;
            }
            try out.append(_.allocator, c);
            _.pos += 1;
        }
        return error.UnexpectedEof;
    }

    fn parseMultilineBasicString(*_): ([]const u8)! {
        // Trim immediate newline after opening """
        if (_.pos < _.src.len && _.src[_.pos] == '\r') _.pos += 1;
        if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;

        let out = TomlByteList.empty;
        while (_.pos < _.src.len) {
            if (_.pos + 2 < _.src.len && _.src[_.pos] == '"' && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                _.pos += 3;
                // Handle up to 2 extra quotes immediately following closing delimiter
                let extra: usize = 0;
                while (extra < 2 && _.pos < _.src.len && _.src[_.pos] == '"') : (extra += 1) {
                    try out.append(_.allocator, '"');
                    _.pos += 1;
                }
                return try out.toOwnedSlice(_.allocator);
            }
            const c = _.src[_.pos];
            if (c == '\\') {
                _.pos += 1;
                if (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t' || _.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                    // Line-ending backslash trims all whitespace & newlines
                    let scan = _.pos;
                    while (scan < _.src.len && (_.src[scan] == ' ' || _.src[scan] == '\t')) scan += 1;
                    if (scan < _.src.len && (_.src[scan] == '\r' || _.src[scan] == '\n')) {
                        _.pos = scan;
                        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t' || _.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
                            _.pos += 1;
                        }
                        continue;
                    }
                }
                try _.parseBasicEscape(&out);
                continue;
            }
            try out.append(_.allocator, c);
            _.pos += 1;
        }
        return error.UnexpectedEof;
    }

    fn parseLiteralString(*_): ([]const u8)! {
        _.pos += 1; // '\''
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '\'' && _.src[_.pos + 1] == '\'') {
            _.pos += 2;
            if (_.pos < _.src.len && _.src[_.pos] == '\r') _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
            const start = _.pos;
            while (_.pos + 2 < _.src.len) {
                if (_.src[_.pos] == '\'' && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
                    const inner = _.src[start.._.pos];
                    _.pos += 3;
                    let extra: usize = 0;
                    let out = TomlByteList.empty;
                    try out.appendSlice(_.allocator, inner);
                    while (extra < 2 && _.pos < _.src.len && _.src[_.pos] == '\'') : (extra += 1) {
                        try out.append(_.allocator, '\'');
                        _.pos += 1;
                    }
                    return try out.toOwnedSlice(_.allocator);
                }
                _.pos += 1;
            }
            return error.UnexpectedEof;
        }
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\'') {
                const s = _.src[start.._.pos];
                _.pos += 1;
                return s;
            }
            if (c == '\n' || c == '\r') return error.SyntaxError;
            _.pos += 1;
        }
        return error.UnexpectedEof;
    }

    fn parseValue(*_): ConfValue! {
        _.skipWs();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '"') {
            return ConfValue.makeString(try _.parseBasicString());
        }
        if (c == '\'') {
            return ConfValue.makeString(try _.parseLiteralString());
        }
        if (c == '[') {
            return try _.parseArray();
        }
        if (c == '{') {
            return try _.parseInlineTable();
        }

        // Scan unquoted token (bool, int, float, or RFC 3339 date/time)
        const start = _.pos;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == ',' || ch == ']' || ch == '}' || ch == '#' || ch == '\r' || ch == '\n') {
                break;
            }
            _.pos += 1;
        }
        let tok = std.mem.trim(u8, _.src[start.._.pos], " \t");
        if (tok.len == 0) return error.SyntaxError;

        // Note: Offset Date-Time may have a space between date and time (e.g. `1979-05-27 07:32:00Z`),
        // which stays in `tok` because we only stop on `,`, `]`, `}`, `#`, `\r`, `\n`.
        if (std.mem.eql(u8, tok, "true")) return ConfValue.makeBool(true);
        if (std.mem.eql(u8, tok, "false")) return ConfValue.makeBool(false);

        if (dt := ConfDateTime.parse(tok)) {
            return ConfValue.makeDateTime(dt);
        }

        const clean = TomlDoc.stripUnderscores(tok);
        if (std.mem.eql(u8, clean, "inf") || std.mem.eql(u8, clean, "+inf")) return ConfValue.makeFloat(std.math.inf(f64));
        if (std.mem.eql(u8, clean, "-inf")) return ConfValue.makeFloat(-1.0 * std.math.inf(f64));
        if (std.mem.eql(u8, clean, "nan") || std.mem.eql(u8, clean, "+nan") || std.mem.eql(u8, clean, "-nan")) return ConfValue.makeFloat(std.math.nan(f64));

        if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
            const n = std.fmt.parseInt(i64, clean[2..], 16) catch return error.InvalidNumber;
            return ConfValue.makeInt(n);
        }
        if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'o' || clean[1] == 'O')) {
            const n = std.fmt.parseInt(i64, clean[2..], 8) catch return error.InvalidNumber;
            return ConfValue.makeInt(n);
        }
        if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'b' || clean[1] == 'B')) {
            const n = std.fmt.parseInt(i64, clean[2..], 2) catch return error.InvalidNumber;
            return ConfValue.makeInt(n);
        }

        let is_float = false;
        for (ch of clean) {
            if (ch == '.' || ch == 'e' || ch == 'E') {
                is_float = true;
                break;
            }
        }
        if (is_float) {
            let num_s = clean;
            if (clean[0] == '+') num_s = clean[1..];
            const f = std.fmt.parseFloat(f64, num_s) catch return error.InvalidNumber;
            return ConfValue.makeFloat(f);
        }
        let num_s = clean;
        if (clean[0] == '+') num_s = clean[1..];
        const iv = std.fmt.parseInt(i64, num_s, 10) catch return error.InvalidNumber;
        return ConfValue.makeInt(iv);
    }

    fn parseArray(*_): ConfValue! {
        _.pos += 1; // '['
        let items = TomlValueList.empty;
        while (true) {
            _.skipWsAndComments();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            const elem = try _.parseValue();
            try items.append(_.allocator, elem);
            _.skipWsAndComments();
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.pos < _.src.len && _.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            return error.SyntaxError;
        }
        return ConfValue.makeArray(try items.toOwnedSlice(_.allocator));
    }

    fn parseInlineTable(*_): ConfValue! {
        _.pos += 1; // '{'
        let tbl = ConfValue.makeEmptyObject();
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) return error.UnexpectedEof;
            if (_.src[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            let segs: [32][]const u8 = undefined;
            const seg_count = try _.parseKeyPath(&segs);
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') return error.SyntaxError;
            _.pos += 1;
            const val = try _.parseValue();
            try TomlParser.setNestedPath(&tbl, segs[0..seg_count], val, _.allocator);
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            return error.SyntaxError;
        }
        return tbl;
    }

    fn setObjectField(obj: *ConfValue, key: []const u8, val: ConfValue, allocator: std.mem.Allocator): ! {
        if (obj.tag != ConfTag.Object) return error.SyntaxError;
        let i: usize = 0;
        while (i < obj.obj_val.len) : (i += 1) {
            if (std.mem.eql(u8, obj.obj_val[i].key, key)) {
                obj.obj_val[i].value = val;
                return;
            }
        }
        let next = try allocator.alloc(ConfField, obj.obj_val.len + 1);
        if (obj.obj_val.len > 0) {
            @memcpy(next[0..obj.obj_val.len], obj.obj_val);
            allocator.free(obj.obj_val);
        }
        next[obj.obj_val.len] = ConfField{ key: key, value: val };
        obj.obj_val = next;
    }

    fn getOrCreateChildObject(obj: *ConfValue, key: []const u8, allocator: std.mem.Allocator): (*ConfValue)! {
        if (obj.tag != ConfTag.Object) return error.TableRedefined;
        let i: usize = 0;
        while (i < obj.obj_val.len) : (i += 1) {
            if (std.mem.eql(u8, obj.obj_val[i].key, key)) {
                if (obj.obj_val[i].value.tag == ConfTag.Array) {
                    const arr = obj.obj_val[i].value.arr_val;
                    if (arr.len == 0) return error.TableRedefined;
                    return &arr[arr.len - 1];
                }
                if (obj.obj_val[i].value.tag != ConfTag.Object) return error.TableRedefined;
                return &obj.obj_val[i].value;
            }
        }
        try TomlParser.setObjectField(obj, key, ConfValue.makeEmptyObject(), allocator);
        return &obj.obj_val[obj.obj_val.len - 1].value;
    }

    fn appendTableArrayItem(obj: *ConfValue, key: []const u8, allocator: std.mem.Allocator): (*ConfValue)! {
        if (obj.tag != ConfTag.Object) return error.TableRedefined;
        let arr_idx: usize? = null;
        let i: usize = 0;
        while (i < obj.obj_val.len) : (i += 1) {
            if (std.mem.eql(u8, obj.obj_val[i].key, key)) {
                if (obj.obj_val[i].value.tag != ConfTag.Array) return error.TableRedefined;
                arr_idx = i;
                break;
            }
        }
        if (idx := arr_idx) {
            const old_arr = obj.obj_val[idx].value.arr_val;
            let next_arr = try allocator.alloc(ConfValue, old_arr.len + 1);
            if (old_arr.len > 0) {
                @memcpy(next_arr[0..old_arr.len], old_arr);
                allocator.free(old_arr);
            }
            next_arr[old_arr.len] = ConfValue.makeEmptyObject();
            obj.obj_val[idx].value.arr_val = next_arr;
            return &next_arr[old_arr.len];
        }
        let fresh = try allocator.alloc(ConfValue, 1);
        fresh[0] = ConfValue.makeEmptyObject();
        try TomlParser.setObjectField(obj, key, ConfValue.makeArray(fresh), allocator);
        return &obj.obj_val[obj.obj_val.len - 1].value.arr_val[0];
    }

    fn setNestedPath(target: *ConfValue, segs: []const []const u8, val: ConfValue, allocator: std.mem.Allocator): ! {
        if (segs.len == 0) return error.SyntaxError;
        let cur = target;
        let i: usize = 0;
        while (i + 1 < segs.len) : (i += 1) {
            cur = try TomlParser.getOrCreateChildObject(cur, segs[i], allocator);
        }
        try TomlParser.setObjectField(cur, segs[segs.len - 1], val, allocator);
    }

    fn runParser(*_): ConfValue! {
        let root = ConfValue.makeEmptyObject();
        let active_table: *ConfValue = &root;

        while (true) {
            _.skipWsAndComments();
            if (_.pos >= _.src.len) break;

            if (_.src[_.pos] == '[') {
                _.pos += 1;
                const is_table_array = (_.pos < _.src.len && _.src[_.pos] == '[');
                if (is_table_array) _.pos += 1;

                let segs: [32][]const u8 = undefined;
                const seg_count = try _.parseKeyPath(&segs);
                _.skipWs();
                if (_.pos >= _.src.len || _.src[_.pos] != ']') return error.SyntaxError;
                _.pos += 1;
                if (is_table_array) {
                    if (_.pos >= _.src.len || _.src[_.pos] != ']') return error.SyntaxError;
                    _.pos += 1;
                }
                try _.expectLineEndOrEof();

                let cur: *ConfValue = &root;
                let i: usize = 0;
                while (i + 1 < seg_count) : (i += 1) {
                    cur = try TomlParser.getOrCreateChildObject(cur, segs[i], _.allocator);
                }
                if (is_table_array) {
                    active_table = try TomlParser.appendTableArrayItem(cur, segs[seg_count - 1], _.allocator);
                } else {
                    active_table = try TomlParser.getOrCreateChildObject(cur, segs[seg_count - 1], _.allocator);
                }
                continue;
            }

            // Key = Value
            let segs: [32][]const u8 = undefined;
            const seg_count = try _.parseKeyPath(&segs);
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != '=') return error.SyntaxError;
            _.pos += 1;
            const val = try _.parseValue();
            try _.expectLineEndOrEof();
            try TomlParser.setNestedPath(active_table, segs[0..seg_count], val, _.allocator);
        }

        return root;
    }

    // ========================================================================
    // Bidirectional TOML 1.0 Serialization
    // ========================================================================

    pub static fn stringify(allocator: std.mem.Allocator, val: ConfValue): ([]const u8)! {
        let out = TomlByteList.empty;
        if (val.tag != ConfTag.Object) return error.SyntaxError;
        try TomlParser.writeTableBody(&out, allocator, "", val);
        return try out.toOwnedSlice(allocator);
    }

    fn isBareKey(s: []const u8): bool {
        if (s.len == 0) return false;
        for (c of s) {
            if (!TomlParser.isBareKeyChar(c)) return false;
        }
        return true;
    }

    fn writeKey(out: *TomlByteList, allocator: std.mem.Allocator, k: []const u8): ! {
        if (TomlParser.isBareKey(k)) {
            try out.appendSlice(allocator, k);
            return;
        }
        try TomlParser.writeQuotedString(out, allocator, k);
    }

    fn writeQuotedString(out: *TomlByteList, allocator: std.mem.Allocator, s: []const u8): ! {
        try out.append(allocator, '"');
        for (c of s) {
            switch (c) {
                '"' => try out.appendSlice(allocator, "\\\""),
                '\\' => try out.appendSlice(allocator, "\\\\"),
                '\n' => try out.appendSlice(allocator, "\\n"),
                '\r' => try out.appendSlice(allocator, "\\r"),
                '\t' => try out.appendSlice(allocator, "\\t"),
                else => try out.append(allocator, c),
            }
        }
        try out.append(allocator, '"');
    }

    fn isTableArray(v: ConfValue): bool {
        return v.tag == ConfTag.Array && v.arr_val.len > 0 && v.arr_val[0].tag == ConfTag.Object;
    }

    fn writeInlineValue(out: *TomlByteList, allocator: std.mem.Allocator, val: ConfValue): ! {
        switch (val.tag) {
            .Null => try out.appendSlice(allocator, "\"\""),
            .Bool => try out.appendSlice(allocator, if (val.bool_val) "true" else "false"),
            .Int => {
                const s = try std.fmt.allocPrint(allocator, "{d}", .{val.int_val});
                defer allocator.free(s);
                try out.appendSlice(allocator, s);
            },
            .Float => {
                if (std.math.isNan(val.float_val)) {
                    try out.appendSlice(allocator, "nan");
                } else if (std.math.isInf(val.float_val)) {
                    try out.appendSlice(allocator, if (val.float_val < 0) "-inf" else "inf");
                } else {
                    const s = try std.fmt.allocPrint(allocator, "{d}", .{val.float_val});
                    defer allocator.free(s);
                    try out.appendSlice(allocator, s);
                }
            },
            .String => try TomlParser.writeQuotedString(out, allocator, val.str_val),
            .DateTime => try out.appendSlice(allocator, val.dt_val.raw),
            .Array => {
                try out.appendSlice(allocator, "[ ");
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) try out.appendSlice(allocator, ", ");
                    try TomlParser.writeInlineValue(out, allocator, val.arr_val[i]);
                }
                try out.appendSlice(allocator, " ]");
            },
            .Object => {
                try out.appendSlice(allocator, "{ ");
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    if (i > 0) try out.appendSlice(allocator, ", ");
                    try TomlParser.writeKey(out, allocator, val.obj_val[i].key);
                    try out.appendSlice(allocator, " = ");
                    try TomlParser.writeInlineValue(out, allocator, val.obj_val[i].value);
                }
                try out.appendSlice(allocator, " }");
            },
        }
    }

    fn writeTableBody(out: *TomlByteList, allocator: std.mem.Allocator, prefix: []const u8, obj: ConfValue): ! {
        // 1. Scalar / regular array / empty-object fields
        for (f of obj.obj_val) {
            if (f.value.tag == ConfTag.Object || TomlParser.isTableArray(f.value)) continue;
            try TomlParser.writeKey(out, allocator, f.key);
            try out.appendSlice(allocator, " = ");
            try TomlParser.writeInlineValue(out, allocator, f.value);
            try out.append(allocator, '\n');
        }
        // 2. Sub-tables
        for (f of obj.obj_val) {
            if (f.value.tag != ConfTag.Object) continue;
            let key_buf = TomlByteList.empty;
            defer key_buf.deinit(allocator);
            try TomlParser.writeKey(&key_buf, allocator, f.key);
            const full_name = if (prefix.len == 0)
                try allocator.dupe(u8, key_buf.items)
            else
                try std.fmt.allocPrint(allocator, "{s}.{s}", .{ prefix, key_buf.items });
            defer allocator.free(full_name);

            if (out.items.len > 0) try out.append(allocator, '\n');
            try out.append(allocator, '[');
            try out.appendSlice(allocator, full_name);
            try out.appendSlice(allocator, "]\n");
            try TomlParser.writeTableBody(out, allocator, full_name, f.value);
        }
        // 3. Array of tables [[...]]
        for (f of obj.obj_val) {
            if (!TomlParser.isTableArray(f.value)) continue;
            let key_buf = TomlByteList.empty;
            defer key_buf.deinit(allocator);
            try TomlParser.writeKey(&key_buf, allocator, f.key);
            const full_name = if (prefix.len == 0)
                try allocator.dupe(u8, key_buf.items)
            else
                try std.fmt.allocPrint(allocator, "{s}.{s}", .{ prefix, key_buf.items });
            defer allocator.free(full_name);

            for (elem of f.value.arr_val) {
                if (out.items.len > 0) try out.append(allocator, '\n');
                try out.appendSlice(allocator, "[[");
                try out.appendSlice(allocator, full_name);
                try out.appendSlice(allocator, "]]\n");
                try TomlParser.writeTableBody(out, allocator, full_name, elem);
            }
        }
    }
}

pub struct TomlDoc {
    pub entries: [512]TomlEntry = undefined;
    pub count: usize = 0;

    pub static fn parse(content: []const u8): TomlDoc {
        let doc = TomlDoc {
            entries: undefined,
            count: 0,
        };
        doc.parseContent(content);
        return doc;
    }

    pub static fn parseTree(allocator: std.mem.Allocator, content: []const u8): ConfValue! {
        return try TomlParser.parse(allocator, content);
    }

    pub fn get(table: []const u8, key: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn getRoot(key: []const u8): ([]const u8)? {
        return _.get("", key);
    }

    pub fn getOr(table: []const u8, key: []const u8, default_val: []const u8): []const u8 {
        if (v := _.get(table, key)) {
            return v;
        }
        return default_val;
    }

    pub fn getDateTime(table: []const u8, key: []const u8): ConfDateTime? {
        if (v := _.get(table, key)) {
            return ConfDateTime.parse(v);
        }
        return null;
    }

    pub fn getInt(table: []const u8, key: []const u8, default_val: i64): i64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 16) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'o' || clean[1] == 'O')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 8) catch null) {
                    return val;
                }
            } else if (clean.len > 2 && clean[0] == '0' && (clean[1] == 'b' || clean[1] == 'B')) {
                if (val := std.fmt.parseInt(i64, clean[2..], 2) catch null) {
                    return val;
                }
            } else {
                let to_parse = clean;
                if (clean.len > 0 && clean[0] == '+') {
                    to_parse = clean[1..];
                }
                if (val := std.fmt.parseInt(i64, to_parse, 10) catch null) {
                    return val;
                }
            }
        }
        return default_val;
    }

    pub fn getFloat(table: []const u8, key: []const u8, default_val: f64): f64 {
        if (v := _.get(table, key)) {
            const clean = TomlDoc.stripUnderscores(v);
            if (std.mem.eql(u8, clean, "inf") || std.mem.eql(u8, clean, "+inf")) {
                return std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "-inf")) {
                return -1.0 * std.math.inf(f64);
            }
            if (std.mem.eql(u8, clean, "nan") || std.mem.eql(u8, clean, "+nan") || std.mem.eql(u8, clean, "-nan")) {
                return std.math.nan(f64);
            }
            let to_parse = clean;
            if (clean.len > 0 && clean[0] == '+') {
                to_parse = clean[1..];
            }
            if (val := std.fmt.parseFloat(f64, to_parse) catch null) {
                return val;
            }
        }
        return default_val;
    }

    pub fn getBool(table: []const u8, key: []const u8, default_val: bool): bool {
        if (v := _.get(table, key)) {
            if (std.mem.eql(u8, v, "true")) {
                return true;
            }
            if (std.mem.eql(u8, v, "false")) {
                return false;
            }
        }
        return default_val;
    }

    pub fn getArray(table: []const u8, key: []const u8): [][]const u8 {
        if (v := _.get(table, key)) {
            return TomlDoc.parseArrayElements(v);
        }
        let empty_buf = std.heap.page_allocator.alloc(string, 0) catch unreachable;
        return empty_buf;
    }

    pub fn hasTable(table: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn hasKey(table: []const u8, key: []const u8): bool {
        return _.get(table, key) != null;
    }

    pub fn set(*_, table: []const u8, key: []const u8, value: []const u8) {
        _.put(table, key, value);
    }

    pub fn setRoot(*_, key: []const u8, value: []const u8) {
        _.put("", key, value);
    }

    pub fn remove(*_, table: []const u8, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && std.mem.eql(u8, _.entries[i].key, key)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn removeTable(*_, table: []const u8): bool {
        let removed = false;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                removed = true;
            } else {
                i += 1;
            }
        }
        return removed;
    }

    pub fn getTables(*_): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < n) {
                    if (std.mem.eql(u8, buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && n < 64) {
                    buf[n] = tbl;
                    n += 1;
                }
            }
            i += 1;
        }
        return buf[0..n];
    }

    pub fn getKeys(*_, table: []const u8): [][]const u8 {
        let buf = std.heap.page_allocator.alloc(string, 128) catch unreachable;
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) && n < 128) {
                buf[n] = _.entries[i].key;
                n += 1;
            }
            i += 1;
        }
        return buf[0..n];
    }

    fn formatScalarToString(val: ConfValue): []const u8 {
        const alloc = std.heap.page_allocator;
        return switch (val.tag) {
            .Null => "",
            .Bool => if (val.bool_val) "true" else "false",
            .Int => std.fmt.allocPrint(alloc, "{d}", .{val.int_val}) catch "",
            .Float => std.fmt.allocPrint(alloc, "{d}", .{val.float_val}) catch "",
            .String => val.str_val,
            .DateTime => val.dt_val.raw,
            .Array => {
                let out = TomlByteList.empty;
                out.append(alloc, '[') catch return "[]";
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    if (i > 0) out.appendSlice(alloc, ", ") catch {};
                    const item = val.arr_val[i];
                    if (item.tag == ConfTag.String) {
                        out.append(alloc, '"') catch {};
                        out.appendSlice(alloc, item.str_val) catch {};
                        out.append(alloc, '"') catch {};
                    } else {
                        out.appendSlice(alloc, TomlDoc.formatScalarToString(item)) catch {};
                    }
                }
                out.append(alloc, ']') catch {};
                return out.toOwnedSlice(alloc) catch "[]";
            },
            .Object => "{...}",
        };
    }

    fn flattenTree(*_, prefix: []const u8, val: ConfValue) {
        if (val.tag != ConfTag.Object) return;
        const alloc = std.heap.page_allocator;
        for (f of val.obj_val) {
            if (f.value.tag == ConfTag.Object) {
                let sub = f.key;
                if (prefix.len > 0) {
                    sub = std.fmt.allocPrint(alloc, "{s}.{s}", .{ prefix, f.key }) catch f.key;
                }
                _.flattenTree(sub, f.value);
            } else if (f.value.tag == ConfTag.Array && f.value.arr_val.len > 0 && f.value.arr_val[0].tag == ConfTag.Object) {
                let arr_tbl = f.key;
                if (prefix.len > 0) {
                    arr_tbl = std.fmt.allocPrint(alloc, "{s}.{s}", .{ prefix, f.key }) catch f.key;
                }
                let idx: usize = 0;
                while (idx < f.value.arr_val.len) : (idx += 1) {
                    const indexed_tbl = std.fmt.allocPrint(alloc, "{s}.{d}", .{ arr_tbl, idx }) catch arr_tbl;
                    _.flattenTree(indexed_tbl, f.value.arr_val[idx]);
                    _.flattenTree(arr_tbl, f.value.arr_val[idx]);
                }
            } else {
                _.put(prefix, f.key, TomlDoc.formatScalarToString(f.value));
            }
        }
    }

    fn parseContent(*_, content: []const u8) {
        const tree = TomlParser.parse(std.heap.page_allocator, content) catch {
            _.parseFallback(content);
            return;
        };
        _.flattenTree("", tree);
    }

    fn parseFallback(*_, content: []const u8) {
        let current_table: []const u8 = "";
        let line_iter = std.mem.splitScalar(u8, content, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#') {
                continue;
            }

            if (line.len >= 4 && line[0] == '[' && line[1] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 2 && line[close_idx - 1] == ']') {
                        current_table = std.mem.trim(u8, line[2 .. close_idx - 1], " \t");
                    }
                }
                continue;
            }

            if (line[0] == '[') {
                if (close_idx := std.mem.lastIndexOfScalar(u8, line, ']')) {
                    if (close_idx > 1) {
                        current_table = std.mem.trim(u8, line[1..close_idx], " \t");
                    }
                }
                continue;
            }

            if (eq_idx := std.mem.indexOfScalar(u8, line, '=')) {
                const raw_key = std.mem.trim(u8, line[0..eq_idx], " \t\"'");
                if (raw_key.len == 0) {
                    continue;
                }

                let target_table = current_table;
                let target_key = raw_key;

                if (dot_idx := std.mem.lastIndexOfScalar(u8, raw_key, '.')) {
                    if (dot_idx > 0 && dot_idx + 1 < raw_key.len) {
                        const prefix = raw_key[0..dot_idx];
                        target_key = raw_key[dot_idx + 1 ..];
                        if (current_table.len > 0) {
                            let comb = std.heap.page_allocator.alloc(u8, current_table.len + 1 + prefix.len) catch unreachable;
                            @memcpy(comb[0..current_table.len], current_table);
                            comb[current_table.len] = '.';
                            @memcpy(comb[current_table.len + 1 ..], prefix);
                            target_table = comb;
                        } else {
                            target_table = prefix;
                        }
                    }
                }

                const raw_val = std.mem.trim(u8, line[eq_idx + 1 ..], " \t");
                const val = TomlDoc.cleanTomlValue(raw_val);
                _.put(target_table, target_key, val);
            }
        }
    }

    static fn cleanTomlValue(raw: []const u8): []const u8 {
        if (raw.len == 0) {
            return "";
        }
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "\"\"\"") && std.mem.endsWith(u8, raw, "\"\"\"")) {
            return TomlDoc.unescapeToml(raw[3 .. raw.len - 3]);
        }
        if (raw.len >= 6 && std.mem.startsWith(u8, raw, "'''") && std.mem.endsWith(u8, raw, "'''")) {
            return raw[3 .. raw.len - 3];
        }
        if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
            return TomlDoc.unescapeToml(raw[1 .. raw.len - 1]);
        }
        if (raw.len >= 2 && raw[0] == '\'' && raw[raw.len - 1] == '\'') {
            return raw[1 .. raw.len - 1];
        }
        if ((raw[0] == '[' && raw[raw.len - 1] == ']') ||
            (raw[0] == '{' && raw[raw.len - 1] == '}'))
        {
            return raw;
        }
        let end = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '#' && i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) {
                end = i - 1;
                break;
            }
            i += 1;
        }
        return std.mem.trim(u8, raw[0..end], " \t\r");
    }

    static fn unescapeToml(s: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, s, '\\') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                const next_ch = s[i + 1];
                if (next_ch == 'n') {
                    buf[out_idx] = '\n';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 'r') {
                    buf[out_idx] = '\r';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == 't') {
                    buf[out_idx] = '\t';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '\\') {
                    buf[out_idx] = '\\';
                    out_idx += 1;
                    i += 2;
                    continue;
                } else if (next_ch == '"') {
                    buf[out_idx] = '"';
                    out_idx += 1;
                    i += 2;
                    continue;
                }
            }
            buf[out_idx] = s[i];
            out_idx += 1;
            i += 1;
        }
        return buf[0..out_idx];
    }

    pub static fn stripUnderscores(s: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, s, '_') == null) {
            return s;
        }
        let buf = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] != '_') {
                buf[out_idx] = s[i];
                out_idx += 1;
            }
            i += 1;
        }
        return buf[0..out_idx];
    }

    static fn parseArrayElements(raw: []const u8): [][]const u8 {
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        if (trimmed.len < 2 || trimmed[0] != '[' || trimmed[trimmed.len - 1] != ']') {
            let empty = std.heap.page_allocator.alloc(string, 0) catch unreachable;
            return empty;
        }
        const inner = trimmed[1 .. trimmed.len - 1];
        let buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let count: usize = 0;

        let in_quotes = false;
        let quote_char: u8 = 0;
        let start: usize = 0;
        let i: usize = 0;

        while (i < inner.len) {
            const ch = inner[i];
            if (in_quotes) {
                if (ch == quote_char && (i == 0 || inner[i - 1] != '\\')) {
                    in_quotes = false;
                }
            } else {
                if (ch == '"' || ch == '\'') {
                    in_quotes = true;
                    quote_char = ch;
                } else if (ch == ',') {
                    const item = std.mem.trim(u8, inner[start..i], " \t\r\n");
                    if (item.len > 0 && count < 64) {
                        buf[count] = TomlDoc.cleanTomlValue(item);
                        count += 1;
                    }
                    start = i + 1;
                }
            }
            i += 1;
        }

        const last_item = std.mem.trim(u8, inner[start..], " \t\r\n");
        if (last_item.len > 0 && count < 64) {
            buf[count] = TomlDoc.cleanTomlValue(last_item);
            count += 1;
        }

        return buf[0..count];
    }

    fn put(*_, table: []const u8, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].table, table) &&
                std.mem.eql(u8, _.entries[i].key, key))
            {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }
        if (_.count < _.entries.len) {
            _.entries[_.count] = TomlEntry {
                table: table,
                key: key,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn toString(): []const u8 {
        let buf = std.heap.page_allocator.alloc(u8, 32768) catch unreachable;
        let len: usize = 0;

        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].table.len == 0) {
                const k = _.entries[i].key;
                const v = _.entries[i].value;
                let idx: usize = 0;
                while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                buf[len] = ' '; len += 1;
                buf[len] = '='; len += 1;
                buf[len] = ' '; len += 1;
                idx = 0;
                while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                buf[len] = '\n'; len += 1;
            }
            i += 1;
        }

        let tbl_buf = std.heap.page_allocator.alloc(string, 64) catch unreachable;
        let tbl_count: usize = 0;
        i = 0;
        while (i < _.count) {
            const tbl = _.entries[i].table;
            if (tbl.len > 0) {
                let found = false;
                let k: usize = 0;
                while (k < tbl_count) {
                    if (std.mem.eql(u8, tbl_buf[k], tbl)) {
                        found = true;
                        break;
                    }
                    k += 1;
                }
                if (!found && tbl_count < 64) {
                    tbl_buf[tbl_count] = tbl;
                    tbl_count += 1;
                }
            }
            i += 1;
        }

        let t: usize = 0;
        while (t < tbl_count) {
            const tbl_name = tbl_buf[t];
            if (len > 0) {
                buf[len] = '\n'; len += 1;
            }
            buf[len] = '['; len += 1;
            let idx: usize = 0;
            while (idx < tbl_name.len) { buf[len] = tbl_name[idx]; len += 1; idx += 1; }
            buf[len] = ']'; len += 1;
            buf[len] = '\n'; len += 1;

            i = 0;
            while (i < _.count) {
                if (std.mem.eql(u8, _.entries[i].table, tbl_name)) {
                    const k = _.entries[i].key;
                    const v = _.entries[i].value;
                    idx = 0;
                    while (idx < k.len) { buf[len] = k[idx]; len += 1; idx += 1; }
                    buf[len] = ' '; len += 1;
                    buf[len] = '='; len += 1;
                    buf[len] = ' '; len += 1;
                    idx = 0;
                    while (idx < v.len) { buf[len] = v[idx]; len += 1; idx += 1; }
                    buf[len] = '\n'; len += 1;
                }
                i += 1;
            }
            t += 1;
        }

        return buf[0..len];
    }

    pub fn encode(): []const u8 {
        return _.toString();
    }
}

pub type toml = TomlDoc;
pub type TOML = TomlDoc;
