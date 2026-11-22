// Source: cp/lib/jsonpath/json_val.cp (RFC 9535 lightweight JSON DOM & codec)

const std = @import("std");

pub enum JsonKind {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
}

pub struct JsonMember {
    pub key: []const u8,
    pub value: JsonVal,
}

pub struct JsonVal {
    pub kind: JsonKind,
    pub bool_val: bool,
    pub num_val: f64,
    pub str_val: []const u8,
    pub arr_val: []JsonVal,
    pub obj_val: []JsonMember,
}

pub fn makeNull(): JsonVal {
    return JsonVal{
        .kind = JsonKind.Null,
        .bool_val = false,
        .num_val = 0.0,
        .str_val = "",
        .arr_val = &.{},
        .obj_val = &.{},
    };
}

pub fn makeBool(v: bool): JsonVal {
    return JsonVal{
        .kind = JsonKind.Bool,
        .bool_val = v,
        .num_val = 0.0,
        .str_val = "",
        .arr_val = &.{},
        .obj_val = &.{},
    };
}

pub fn makeNumber(v: f64): JsonVal {
    return JsonVal{
        .kind = JsonKind.Number,
        .bool_val = false,
        .num_val = v,
        .str_val = "",
        .arr_val = &.{},
        .obj_val = &.{},
    };
}

pub fn makeString(s: []const u8): JsonVal {
    return JsonVal{
        .kind = JsonKind.String,
        .bool_val = false,
        .num_val = 0.0,
        .str_val = s,
        .arr_val = &.{},
        .obj_val = &.{},
    };
}

pub fn deinitJsonVal(val: *JsonVal, alloc: std.mem.Allocator) {
    switch (val.kind) {
        JsonKind.String => {
            if (val.str_val.len > 0) {
                alloc.free(val.str_val);
            }
        },
        JsonKind.Array => {
            for (*item of val.arr_val) {
                deinitJsonVal(item, alloc);
            }
            if (val.arr_val.len > 0) {
                alloc.free(val.arr_val);
            }
        },
        JsonKind.Object => {
            for (*m of val.obj_val) {
                if (m.key.len > 0) {
                    alloc.free(m.key);
                }
                deinitJsonVal(&m.value, alloc);
            }
            if (val.obj_val.len > 0) {
                alloc.free(val.obj_val);
            }
        },
        else => {},
    }
}

pub fn jsonEqual(a: *const JsonVal, b: *const JsonVal): bool {
    if (a.kind != b.kind) {
        return false;
    }
    switch (a.kind) {
        JsonKind.Null => return true,
        JsonKind.Bool => return a.bool_val == b.bool_val,
        JsonKind.Number => return a.num_val == b.num_val,
        JsonKind.String => return std.mem.eql(u8, a.str_val, b.str_val),
        JsonKind.Array => {
            if (a.arr_val.len != b.arr_val.len) {
                return false;
            }
            let i: usize = 0;
            while (i < a.arr_val.len) {
                if (!jsonEqual(&a.arr_val[i], &b.arr_val[i])) {
                    return false;
                }
                i += 1;
            }
            return true;
        },
        JsonKind.Object => {
            if (a.obj_val.len != b.obj_val.len) {
                return false;
            }
            let i: usize = 0;
            while (i < a.obj_val.len) {
                let found = false;
                let j: usize = 0;
                while (j < b.obj_val.len) {
                    if (std.mem.eql(u8, a.obj_val[i].key, b.obj_val[j].key)) {
                        if (!jsonEqual(&a.obj_val[i].value, &b.obj_val[j].value)) {
                            return false;
                        }
                        found = true;
                        break;
                    }
                    j += 1;
                }
                if (!found) {
                    return false;
                }
                i += 1;
            }
            return true;
        },
    }
}

pub enum JsonParseError {
    UnexpectedEof,
    InvalidSyntax,
    InvalidEscape,
    InvalidNumber,
    NestingTooDeep,
    OutOfMemory,
}

struct JsonParser {
    src: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,
    depth: usize,
}

fn skipWs(p: *JsonParser) {
    while (p.pos < p.src.len) {
        const c = p.src[p.pos];
        if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
            p.pos += 1;
        } else {
            break;
        }
    }
}

fn parseHex4(s: []const u8): u21? {
    if (s.len < 4) {
        return null;
    }
    let val: u21 = 0;
    let i: usize = 0;
    while (i < 4) {
        const c = s[i];
        let d: u21 = 0;
        if (c >= '0' and c <= '9') {
            d = @as(u21, c - '0');
        } else if (c >= 'a' and c <= 'f') {
            d = @as(u21, c - 'a' + 10);
        } else if (c >= 'A' and c <= 'F') {
            d = @as(u21, c - 'A' + 10);
        } else {
            return null;
        }
        val = (val << 4) | d;
        i += 1;
    }
    return val;
}

fn parseJsonString(p: *JsonParser): JsonParseError![]const u8 {
    if (p.pos >= p.src.len or p.src[p.pos] != '"') {
        return JsonParseError.InvalidSyntax;
    }
    p.pos += 1;
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(p.alloc);

    while (p.pos < p.src.len) {
        const c = p.src[p.pos];
        if (c == '"') {
            p.pos += 1;
            return try buf.toOwnedSlice(p.alloc);
        }
        if (c == '\\') {
            p.pos += 1;
            if (p.pos >= p.src.len) {
                return JsonParseError.UnexpectedEof;
            }
            const esc = p.src[p.pos];
            p.pos += 1;
            switch (esc) {
                '"' => try buf.append(p.alloc, '"'),
                '\\' => try buf.append(p.alloc, '\\'),
                '/' => try buf.append(p.alloc, '/'),
                'b' => try buf.append(p.alloc, 0x08),
                'f' => try buf.append(p.alloc, 0x0c),
                'n' => try buf.append(p.alloc, '\n'),
                'r' => try buf.append(p.alloc, '\r'),
                't' => try buf.append(p.alloc, '\t'),
                'u' => {
                    if (p.pos + 4 > p.src.len) {
                        return JsonParseError.UnexpectedEof;
                    }
                    const cp_opt = parseHex4(p.src[p.pos .. p.pos + 4]);
                    if (cp_opt == null) {
                        return JsonParseError.InvalidEscape;
                    }
                    let cp = cp_opt.?;
                    p.pos += 4;
                    if (cp >= 0xD800 and cp <= 0xDBFF) {
                        if (p.pos + 6 <= p.src.len and p.src[p.pos] == '\\' and p.src[p.pos + 1] == 'u') {
                            const low_opt = parseHex4(p.src[p.pos + 2 .. p.pos + 6]);
                            if (low_opt != null and low_opt.? >= 0xDC00 and low_opt.? <= 0xDFFF) {
                                cp = 0x10000 + (((cp - 0xD800) << 10) | (low_opt.? - 0xDC00));
                                p.pos += 6;
                            }
                        }
                    }
                    let utf8_buf: [4]u8 = undefined;
                    const len = std.unicode.utf8Encode(cp, &utf8_buf) catch return JsonParseError.InvalidEscape;
                    try buf.appendSlice(p.alloc, utf8_buf[0..len]);
                },
                else => return JsonParseError.InvalidEscape,
            }
        } else {
            if (c < 0x20) {
                return JsonParseError.InvalidSyntax;
            }
            try buf.append(p.alloc, c);
            p.pos += 1;
        }
    }
    return JsonParseError.UnexpectedEof;
}

fn parseJsonNumber(p: *JsonParser): JsonParseError!f64 {
    const start = p.pos;
    if (p.pos < p.src.len and p.src[p.pos] == '-') {
        p.pos += 1;
    }
    if (p.pos >= p.src.len) {
        return JsonParseError.InvalidNumber;
    }
    if (p.src[p.pos] == '0') {
        p.pos += 1;
    } else if (p.src[p.pos] >= '1' and p.src[p.pos] <= '9') {
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    } else {
        return JsonParseError.InvalidNumber;
    }
    if (p.pos < p.src.len and p.src[p.pos] == '.') {
        p.pos += 1;
        if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
            return JsonParseError.InvalidNumber;
        }
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    }
    if (p.pos < p.src.len and (p.src[p.pos] == 'e' or p.src[p.pos] == 'E')) {
        p.pos += 1;
        if (p.pos < p.src.len and (p.src[p.pos] == '+' or p.src[p.pos] == '-')) {
            p.pos += 1;
        }
        if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
            return JsonParseError.InvalidNumber;
        }
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    }
    return std.fmt.parseFloat(f64, p.src[start..p.pos]) catch return JsonParseError.InvalidNumber;
}

fn parseJsonValue(p: *JsonParser): JsonParseError!JsonVal {
    skipWs(p);
    if (p.pos >= p.src.len) {
        return JsonParseError.UnexpectedEof;
    }
    if (p.depth > 256) {
        return JsonParseError.NestingTooDeep;
    }
    const c = p.src[p.pos];
    if (c == 'n') {
        if (p.pos + 4 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 4], "null")) {
            p.pos += 4;
            return makeNull();
        }
        return JsonParseError.InvalidSyntax;
    }
    if (c == 't') {
        if (p.pos + 4 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 4], "true")) {
            p.pos += 4;
            return makeBool(true);
        }
        return JsonParseError.InvalidSyntax;
    }
    if (c == 'f') {
        if (p.pos + 5 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 5], "false")) {
            p.pos += 5;
            return makeBool(false);
        }
        return JsonParseError.InvalidSyntax;
    }
    if (c == '"') {
        const s = try parseJsonString(p);
        return makeString(s);
    }
    if (c == '-' or (c >= '0' and c <= '9')) {
        const num = try parseJsonNumber(p);
        return makeNumber(num);
    }
    if (c == '[') {
        p.pos += 1;
        p.depth += 1;
        defer p.depth -= 1;
        let arr = std.ArrayList(JsonVal).empty;
        errdefer {
            for (*item of arr.items) {
                deinitJsonVal(item, p.alloc);
            }
            arr.deinit(p.alloc);
        }
        skipWs(p);
        if (p.pos < p.src.len and p.src[p.pos] == ']') {
            p.pos += 1;
            return JsonVal{
                .kind = JsonKind.Array,
                .bool_val = false,
                .num_val = 0.0,
                .str_val = "",
                .arr_val = try arr.toOwnedSlice(p.alloc),
                .obj_val = &.{},
            };
        }
        while (true) {
            const elem = try parseJsonValue(p);
            try arr.append(p.alloc, elem);
            skipWs(p);
            if (p.pos >= p.src.len) {
                return JsonParseError.UnexpectedEof;
            }
            if (p.src[p.pos] == ',') {
                p.pos += 1;
                continue;
            }
            if (p.src[p.pos] == ']') {
                p.pos += 1;
                break;
            }
            return JsonParseError.InvalidSyntax;
        }
        return JsonVal{
            .kind = JsonKind.Array,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .arr_val = try arr.toOwnedSlice(p.alloc),
            .obj_val = &.{},
        };
    }
    if (c == '{') {
        p.pos += 1;
        p.depth += 1;
        defer p.depth -= 1;
        let obj = std.ArrayList(JsonMember).empty;
        errdefer {
            for (*m of obj.items) {
                p.alloc.free(m.key);
                deinitJsonVal(&m.value, p.alloc);
            }
            obj.deinit(p.alloc);
        }
        skipWs(p);
        if (p.pos < p.src.len and p.src[p.pos] == '}') {
            p.pos += 1;
            return JsonVal{
                .kind = JsonKind.Object,
                .bool_val = false,
                .num_val = 0.0,
                .str_val = "",
                .arr_val = &.{},
                .obj_val = try obj.toOwnedSlice(p.alloc),
            };
        }
        while (true) {
            skipWs(p);
            const k = try parseJsonString(p);
            errdefer p.alloc.free(k);
            skipWs(p);
            if (p.pos >= p.src.len or p.src[p.pos] != ':') {
                return JsonParseError.InvalidSyntax;
            }
            p.pos += 1;
            const v = try parseJsonValue(p);
            try obj.append(p.alloc, JsonMember{ .key = k, .value = v });
            skipWs(p);
            if (p.pos >= p.src.len) {
                return JsonParseError.UnexpectedEof;
            }
            if (p.src[p.pos] == ',') {
                p.pos += 1;
                continue;
            }
            if (p.src[p.pos] == '}') {
                p.pos += 1;
                break;
            }
            return JsonParseError.InvalidSyntax;
        }
        return JsonVal{
            .kind = JsonKind.Object,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .arr_val = &.{},
            .obj_val = try obj.toOwnedSlice(p.alloc),
        };
    }
    return JsonParseError.InvalidSyntax;
}

pub fn parseJson(alloc: std.mem.Allocator, src: []const u8): JsonParseError!JsonVal {
    let p = JsonParser{
        .src = src,
        .pos = 0,
        .alloc = alloc,
        .depth = 0,
    };
    let val = try parseJsonValue(&p);
    skipWs(&p);
    if (p.pos != p.src.len) {
        deinitJsonVal(&val, alloc);
        return JsonParseError.InvalidSyntax;
    }
    return val;
}

fn writeJsonString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): anyerror!void {
    try buf.append(alloc, '"');
    for (ch of s) {
        switch (ch) {
            '"' => try buf.appendSlice(alloc, "\\\""),
            '\\' => try buf.appendSlice(alloc, "\\\\"),
            '\n' => try buf.appendSlice(alloc, "\\n"),
            '\r' => try buf.appendSlice(alloc, "\\r"),
            '\t' => try buf.appendSlice(alloc, "\\t"),
            0x08 => try buf.appendSlice(alloc, "\\b"),
            0x0c => try buf.appendSlice(alloc, "\\f"),
            else => {
                if (ch < 0x20) {
                    let hex_buf: [6]u8 = undefined;
                    const formatted = try std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{ch});
                    try buf.appendSlice(alloc, formatted);
                } else {
                    try buf.append(alloc, ch);
                }
            },
        }
    }
    try buf.append(alloc, '"');
}

fn stringifyInto(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: *const JsonVal): anyerror!void {
    switch (val.kind) {
        JsonKind.Null => try buf.appendSlice(alloc, "null"),
        JsonKind.Bool => {
            if (val.bool_val) {
                try buf.appendSlice(alloc, "true");
            } else {
                try buf.appendSlice(alloc, "false");
            }
        },
        JsonKind.Number => {
            let num_buf: [64]u8 = undefined;
            const s = try std.fmt.bufPrint(&num_buf, "{d}", .{val.num_val});
            try buf.appendSlice(alloc, s);
        },
        JsonKind.String => try writeJsonString(buf, alloc, val.str_val),
        JsonKind.Array => {
            try buf.append(alloc, '[');
            let i: usize = 0;
            while (i < val.arr_val.len) {
                if (i > 0) {
                    try buf.append(alloc, ',');
                }
                try stringifyInto(buf, alloc, &val.arr_val[i]);
                i += 1;
            }
            try buf.append(alloc, ']');
        },
        JsonKind.Object => {
            try buf.append(alloc, '{');
            let i: usize = 0;
            while (i < val.obj_val.len) {
                if (i > 0) {
                    try buf.append(alloc, ',');
                }
                try writeJsonString(buf, alloc, val.obj_val[i].key);
                try buf.append(alloc, ':');
                try stringifyInto(buf, alloc, &val.obj_val[i].value);
                i += 1;
            }
            try buf.append(alloc, '}');
        },
    }
}

pub fn stringifyJson(alloc: std.mem.Allocator, val: *const JsonVal): anyerror![]const u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try stringifyInto(&buf, alloc, val);
    return try buf.toOwnedSlice(alloc);
}
