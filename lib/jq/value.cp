// Source: cp/lib/jq/value.cp (Independent RFC 8259 JSON Value & Codec for cap:jq)
const std = @import("std");

pub const MAX_JSON_DEPTH: usize = 64;

pub const JqError = error{
    InvalidJson,
    UnexpectedToken,
    UnexpectedEof,
    MaxDepthExceeded,
    TypeError,
    DivisionByZero,
    IndexOutOfBounds,
    InvalidSyntax,
    OutputLimitExceeded,
    OutOfMemory,
};

pub enum JqTag {
    Null,
    Bool,
    Int,
    Float,
    String,
    Array,
    Object,
}

pub struct JqObjEntry {
    key: []const u8 = "",
    value: JqValue = .{},
}

pub struct JqValue {
    tag: JqTag = .Null,
    bool_val: bool = false,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_val: []const u8 = "",
    arr_val: []JqValue = [],
    obj_val: []JqObjEntry = [],

    pub fn nullVal(): JqValue {
        return .{ .tag = .Null };
    }

    pub fn boolVal(v: bool): JqValue {
        return .{ .tag = .Bool, .bool_val = v };
    }

    pub fn intVal(v: i64): JqValue {
        return .{ .tag = .Int, .int_val = v };
    }

    pub fn floatVal(v: f64): JqValue {
        return .{ .tag = .Float, .float_val = v };
    }

    pub fn stringVal(alloc: std.mem.Allocator, s: []const u8): !JqValue {
        const dup = try alloc.dupe(u8, s);
        return .{ .tag = .String, .str_val = dup };
    }

    pub fn arrayVal(items: []JqValue): JqValue {
        return .{ .tag = .Array, .arr_val = items };
    }

    pub fn objectVal(entries: []JqObjEntry): JqValue {
        return .{ .tag = .Object, .obj_val = entries };
    }

    pub fn typeName(): []const u8 {
        return switch (_.tag) {
            .Null => "null",
            .Bool => "boolean",
            .Int, .Float => "number",
            .String => "string",
            .Array => "array",
            .Object => "object",
        };
    }

    pub fn isTruthy(): bool {
        return switch (_.tag) {
            .Null => false,
            .Bool => _.bool_val,
            else => true,
        };
    }

    pub fn asNumber(): f64? {
        return switch (_.tag) {
            .Int => @as(f64, @floatFromInt(_.int_val)),
            .Float => _.float_val,
            else => null,
        };
    }

    pub fn equals(other: JqValue): bool {
        if (_.tag != other.tag) {
            if ((_.tag == .Int or _.tag == .Float) and (other.tag == .Int or other.tag == .Float)) {
                const a = _.asNumber() orelse return false;
                const b = other.asNumber() orelse return false;
                return a == b;
            }
            return false;
        }
        return switch (_.tag) {
            .Null => true,
            .Bool => _.bool_val == other.bool_val,
            .Int => _.int_val == other.int_val,
            .Float => _.float_val == other.float_val,
            .String => std.mem.eql(u8, _.str_val, other.str_val),
            .Array => blk: {
                if (_.arr_val.len != other.arr_val.len) break :blk false;
                var i: usize = 0;
                while (i < _.arr_val.len) {
                    if (!_.arr_val[i].equals(other.arr_val[i])) break :blk false;
                    i += 1;
                }
                break :blk true;
            },
            .Object => blk: {
                if (_.obj_val.len != other.obj_val.len) break :blk false;
                for (entry of _.obj_val) {
                    var found = false;
                    for (other.obj_val) |oentry| {
                        if (std.mem.eql(u8, entry.key, oentry.key)) {
                            if (!entry.value.equals(oentry.value)) break :blk false;
                            found = true;
                            break;
                        }
                    }
                    if (!found) break :blk false;
                }
                break :blk true;
            },
        };
    }

    fn tagRank(tag: JqTag): u8 {
        return switch (tag) {
            .Null => 0,
            .Bool => 1,
            .Int, .Float => 2,
            .String => 3,
            .Array => 4,
            .Object => 5,
        };
    }

    pub fn compare(other: JqValue): i32 {
        const r1 = tagRank(_.tag);
        const r2 = tagRank(other.tag);
        if (r1 < r2) return -1;
        if (r1 > r2) return 1;

        return switch (_.tag) {
            .Null => 0,
            .Bool => blk: {
                if (_.bool_val == other.bool_val) break :blk 0;
                break :blk if (!_.bool_val) -1 else 1;
            },
            .Int, .Float => blk: {
                const a = _.asNumber() orelse 0.0;
                const b = other.asNumber() orelse 0.0;
                if (a < b) break :blk -1;
                if (a > b) break :blk 1;
                break :blk 0;
            },
            .String => blk: {
                const ord = std.mem.order(u8, _.str_val, other.str_val);
                break :blk switch (ord) {
                    .lt => -1,
                    .eq => 0,
                    .gt => 1,
                };
            },
            .Array => blk: {
                const min_len = @min(_.arr_val.len, other.arr_val.len);
                var i: usize = 0;
                while (i < min_len) {
                    const c = _.arr_val[i].compare(other.arr_val[i]);
                    if (c != 0) break :blk c;
                    i += 1;
                }
                if (_.arr_val.len < other.arr_val.len) break :blk -1;
                if (_.arr_val.len > other.arr_val.len) break :blk 1;
                break :blk 0;
            },
            .Object => blk: {
                if (_.obj_val.len < other.obj_val.len) break :blk -1;
                if (_.obj_val.len > other.obj_val.len) break :blk 1;
                var i: usize = 0;
                while (i < _.obj_val.len) {
                    const ko = std.mem.order(u8, _.obj_val[i].key, other.obj_val[i].key);
                    if (ko == .lt) break :blk -1;
                    if (ko == .gt) break :blk 1;
                    const vc = _.obj_val[i].value.compare(other.obj_val[i].value);
                    if (vc != 0) break :blk vc;
                    i += 1;
                }
                break :blk 0;
            },
        };
    }

    pub fn clone(alloc: std.mem.Allocator): JqError!JqValue {
        return switch (_.tag) {
            .Null => JqValue.nullVal(),
            .Bool => JqValue.boolVal(_.bool_val),
            .Int => JqValue.intVal(_.int_val),
            .Float => JqValue.floatVal(_.float_val),
            .String => JqValue.stringVal(alloc, _.str_val) catch return JqError.OutOfMemory,
            .Array => blk: {
                let arr = alloc.alloc(JqValue, _.arr_val.len) catch return JqError.OutOfMemory;
                var filled: usize = 0;
                errdefer {
                    var j: usize = 0;
                    while (j < filled) {
                        arr[j].deinit(alloc);
                        j += 1;
                    }
                    alloc.free(arr);
                }
                while (filled < _.arr_val.len) {
                    arr[filled] = try _.arr_val[filled].clone(alloc);
                    filled += 1;
                }
                break :blk JqValue.arrayVal(arr);
            },
            .Object => blk: {
                let entries = alloc.alloc(JqObjEntry, _.obj_val.len) catch return JqError.OutOfMemory;
                var filled: usize = 0;
                errdefer {
                    var j: usize = 0;
                    while (j < filled) {
                        alloc.free(entries[j].key);
                        entries[j].value.deinit(alloc);
                        j += 1;
                    }
                    alloc.free(entries);
                }
                while (filled < _.obj_val.len) {
                    const kdup = alloc.dupe(u8, _.obj_val[filled].key) catch return JqError.OutOfMemory;
                    errdefer alloc.free(kdup);
                    const vdup = try _.obj_val[filled].value.clone(alloc);
                    entries[filled] = .{ .key = kdup, .value = vdup };
                    filled += 1;
                }
                break :blk JqValue.objectVal(entries);
            },
        };
    }

    pub fn freeContents(alloc: std.mem.Allocator): void {
        switch (_.tag) {
            .String => {
                if (_.str_val.len > 0) alloc.free(_.str_val);
            },
            .Array => {
                let i: usize = 0;
                while (i < _.arr_val.len) {
                    _.arr_val[i].freeContents(alloc);
                    i += 1;
                }
                if (_.arr_val.len > 0) alloc.free(_.arr_val);
            },
            .Object => {
                let i: usize = 0;
                while (i < _.obj_val.len) {
                    alloc.free(_.obj_val[i].key);
                    _.obj_val[i].value.freeContents(alloc);
                    i += 1;
                }
                if (_.obj_val.len > 0) alloc.free(_.obj_val);
            },
            else => {},
        }
    }

    pub fn deinit(*_, alloc: std.mem.Allocator): void {
        _.freeContents(alloc);
        _.* = JqValue.nullVal();
    }
}

pub fn freeValues(alloc: std.mem.Allocator, values: *[]JqValue): void {
    let i: usize = 0;
    while (i < values.*.len) {
        values.*[i].deinit(alloc);
        i += 1;
    }
    if (values.*.len > 0) alloc.free(values.*);
}

struct JsonParser {
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,

    fn skipWs(*_): void {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn parseValue(*_): JqError!JqValue {
        _.skipWs();
        if (_.pos >= _.src.len) return JqError.UnexpectedEof;
        const c = _.src[_.pos];
        return switch (c) {
            'n' => _.parseNull(),
            't' => _.parseTrue(),
            'f' => _.parseFalse(),
            '"' => blk: {
                const s = try _.parseStringRaw();
                break :blk .{ .tag = .String, .str_val = s };
            },
            '[' => _.parseArray(),
            '{' => _.parseObject(),
            '-', '0'...'9' => _.parseNumber(),
            else => JqError.InvalidJson,
        };
    }

    fn parseNull(*_): JqError!JqValue {
        if (_.pos + 4 <= _.src.len and std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "null")) {
            _.pos += 4;
            return JqValue.nullVal();
        }
        return JqError.InvalidJson;
    }

    fn parseTrue(*_): JqError!JqValue {
        if (_.pos + 4 <= _.src.len and std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "true")) {
            _.pos += 4;
            return JqValue.boolVal(true);
        }
        return JqError.InvalidJson;
    }

    fn parseFalse(*_): JqError!JqValue {
        if (_.pos + 5 <= _.src.len and std.mem.eql(u8, _.src[_.pos .. _.pos + 5], "false")) {
            _.pos += 5;
            return JqValue.boolVal(false);
        }
        return JqError.InvalidJson;
    }

    fn parseHex4(*_): JqError!u16 {
        if (_.pos + 4 > _.src.len) return JqError.UnexpectedEof;
        var val: u16 = 0;
        var i: usize = 0;
        while (i < 4) {
            const ch = _.src[_.pos + i];
            const d: u16 = switch (ch) {
                '0'...'9' => @as(u16, ch - '0'),
                'a'...'f' => @as(u16, ch - 'a' + 10),
                'A'...'F' => @as(u16, ch - 'A' + 10),
                else => return JqError.InvalidJson,
            };
            val = (val << 4) | d;
            i += 1;
        }
        _.pos += 4;
        return val;
    }

    fn parseStringRaw(*_): JqError![]const u8 {
        if (_.pos >= _.src.len or _.src[_.pos] != '"') return JqError.InvalidJson;
        _.pos += 1;
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return buf.toOwnedSlice(_.alloc) catch return JqError.OutOfMemory;
            }
            if (c < 0x20) return JqError.InvalidJson;
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return JqError.UnexpectedEof;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => buf.append(_.alloc, '"') catch return JqError.OutOfMemory,
                    '\\' => buf.append(_.alloc, '\\') catch return JqError.OutOfMemory,
                    '/' => buf.append(_.alloc, '/') catch return JqError.OutOfMemory,
                    'b' => buf.append(_.alloc, 0x08) catch return JqError.OutOfMemory,
                    'f' => buf.append(_.alloc, 0x0c) catch return JqError.OutOfMemory,
                    'n' => buf.append(_.alloc, '\n') catch return JqError.OutOfMemory,
                    'r' => buf.append(_.alloc, '\r') catch return JqError.OutOfMemory,
                    't' => buf.append(_.alloc, '\t') catch return JqError.OutOfMemory,
                    'u' => {
                        const code1 = try _.parseHex4();
                        let cp: u21 = @as(u21, code1);
                        if (code1 >= 0xD800 and code1 <= 0xDBFF) {
                            if (_.pos + 2 <= _.src.len and _.src[_.pos] == '\\' and _.src[_.pos + 1] == 'u') {
                                _.pos += 2;
                                const code2 = try _.parseHex4();
                                if (code2 >= 0xDC00 and code2 <= 0xDFFF) {
                                    cp = 0x10000 + ((@as(u21, code1 - 0xD800) << 10) | @as(u21, code2 - 0xDC00));
                                } else {
                                    return JqError.InvalidJson;
                                }
                            } else {
                                return JqError.InvalidJson;
                            }
                        }
                        let ubuf: [4]u8 = undefined;
                        const ulen = std.unicode.utf8Encode(cp, &ubuf) catch return JqError.InvalidJson;
                        buf.appendSlice(_.alloc, ubuf[0..ulen]) catch return JqError.OutOfMemory;
                    },
                    else => return JqError.InvalidJson,
                }
            } else {
                buf.append(_.alloc, c) catch return JqError.OutOfMemory;
                _.pos += 1;
            }
        }
        return JqError.UnexpectedEof;
    }

    fn parseNumber(*_): JqError!JqValue {
        const start = _.pos;
        if (_.src[_.pos] == '-') _.pos += 1;
        if (_.pos >= _.src.len) return JqError.InvalidJson;
        if (_.src[_.pos] == '0') {
            _.pos += 1;
        } else if (_.src[_.pos] >= '1' and _.src[_.pos] <= '9') {
            while (_.pos < _.src.len and _.src[_.pos] >= '0' and _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        } else {
            return JqError.InvalidJson;
        }

        let is_float = false;
        if (_.pos < _.src.len and _.src[_.pos] == '.') {
            is_float = true;
            _.pos += 1;
            if (_.pos >= _.src.len or _.src[_.pos] < '0' or _.src[_.pos] > '9') return JqError.InvalidJson;
            while (_.pos < _.src.len and _.src[_.pos] >= '0' and _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        }
        if (_.pos < _.src.len and (_.src[_.pos] == 'e' or _.src[_.pos] == 'E')) {
            is_float = true;
            _.pos += 1;
            if (_.pos < _.src.len and (_.src[_.pos] == '+' or _.src[_.pos] == '-')) _.pos += 1;
            if (_.pos >= _.src.len or _.src[_.pos] < '0' or _.src[_.pos] > '9') return JqError.InvalidJson;
            while (_.pos < _.src.len and _.src[_.pos] >= '0' and _.src[_.pos] <= '9') {
                _.pos += 1;
            }
        }

        const num_str = _.src[start.._.pos];
        if (!is_float) {
            if (iv := std.fmt.parseInt(i64, num_str, 10)) {
                return JqValue.intVal(iv);
            } else {
                const fv = std.fmt.parseFloat(f64, num_str) catch return JqError.InvalidJson;
                return JqValue.floatVal(fv);
            }
        } else {
            const fv = std.fmt.parseFloat(f64, num_str) catch return JqError.InvalidJson;
            return JqValue.floatVal(fv);
        }
    }

    fn parseArray(*_): JqError!JqValue {
        if (_.depth >= MAX_JSON_DEPTH) return JqError.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // '['
        let items = std.ArrayList(JqValue).empty;
        errdefer {
            for (*it of items.items) it.deinit(_.alloc);
            items.deinit(_.alloc);
        }

        _.skipWs();
        if (_.pos < _.src.len and _.src[_.pos] == ']') {
            _.pos += 1;
            return JqValue.arrayVal(items.toOwnedSlice(_.alloc) catch return JqError.OutOfMemory);
        }

        while (true) {
            const val = try _.parseValue();
            items.append(_.alloc, val) catch {
                val.freeContents(_.alloc);
                return JqError.OutOfMemory;
            };
            _.skipWs();
            if (_.pos >= _.src.len) return JqError.UnexpectedEof;
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            return JqError.InvalidJson;
        }
        return JqValue.arrayVal(items.toOwnedSlice(_.alloc) catch return JqError.OutOfMemory);
    }

    fn parseObject(*_): JqError!JqValue {
        if (_.depth >= MAX_JSON_DEPTH) return JqError.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 1; // '{'
        let entries = std.ArrayList(JqObjEntry).empty;
        errdefer {
            for (*en of entries.items) {
                _.alloc.free(en.key);
                en.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }

        _.skipWs();
        if (_.pos < _.src.len and _.src[_.pos] == '}') {
            _.pos += 1;
            return JqValue.objectVal(entries.toOwnedSlice(_.alloc) catch return JqError.OutOfMemory);
        }

        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len or _.src[_.pos] != '"') return JqError.InvalidJson;
            const key = try _.parseStringRaw();
            errdefer _.alloc.free(key);

            _.skipWs();
            if (_.pos >= _.src.len or _.src[_.pos] != ':') return JqError.InvalidJson;
            _.pos += 1;

            const val = try _.parseValue();
            entries.append(_.alloc, .{ .key = key, .value = val }) catch {
                val.freeContents(_.alloc);
                return JqError.OutOfMemory;
            };

            _.skipWs();
            if (_.pos >= _.src.len) return JqError.UnexpectedEof;
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            if (_.src[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            return JqError.InvalidJson;
        }
        return JqValue.objectVal(entries.toOwnedSlice(_.alloc) catch return JqError.OutOfMemory);
    }
}

pub fn parseJson(alloc: std.mem.Allocator, input: []const u8): JqError!JqValue {
    let parser = JsonParser{ .alloc = alloc, .src = input };
    let val = try parser.parseValue();
    errdefer val.deinit(alloc);
    parser.skipWs();
    if (parser.pos != input.len) return JqError.InvalidJson;
    return val;
}

fn writeEscapedString(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: []const u8): JqError!void {
    buf.append(alloc, '"') catch return JqError.OutOfMemory;
    for (c of s) {
        switch (c) {
            '"' => buf.appendSlice(alloc, "\\\"") catch return JqError.OutOfMemory,
            '\\' => buf.appendSlice(alloc, "\\\\") catch return JqError.OutOfMemory,
            0x08 => buf.appendSlice(alloc, "\\b") catch return JqError.OutOfMemory,
            0x0c => buf.appendSlice(alloc, "\\f") catch return JqError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return JqError.OutOfMemory,
            '\r' => buf.appendSlice(alloc, "\\r") catch return JqError.OutOfMemory,
            '\t' => buf.appendSlice(alloc, "\\t") catch return JqError.OutOfMemory,
            else => {
                if (c < 0x20) {
                    const hex = "0123456789abcdef";
                    buf.appendSlice(alloc, "\\u00") catch return JqError.OutOfMemory;
                    buf.append(alloc, hex[(c >> 4) & 0xF]) catch return JqError.OutOfMemory;
                    buf.append(alloc, hex[c & 0xF]) catch return JqError.OutOfMemory;
                } else {
                    buf.append(alloc, c) catch return JqError.OutOfMemory;
                }
            },
        }
    }
    buf.append(alloc, '"') catch return JqError.OutOfMemory;
}

fn writeValueToBuf(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: JqValue): JqError!void {
    switch (val.tag) {
        .Null => buf.appendSlice(alloc, "null") catch return JqError.OutOfMemory,
        .Bool => buf.appendSlice(alloc, if (val.bool_val) "true" else "false") catch return JqError.OutOfMemory,
        .Int => {
            let num_buf: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&num_buf, "{d}", .{val.int_val}) catch return JqError.OutOfMemory;
            buf.appendSlice(alloc, s) catch return JqError.OutOfMemory;
        },
        .Float => {
            if (std.math.isNan(val.float_val) or std.math.isInf(val.float_val)) {
                buf.appendSlice(alloc, "null") catch return JqError.OutOfMemory;
            } else {
                let num_buf: [64]u8 = undefined;
                const s = std.fmt.bufPrint(&num_buf, "{d}", .{val.float_val}) catch return JqError.OutOfMemory;
                buf.appendSlice(alloc, s) catch return JqError.OutOfMemory;
            }
        },
        .String => try writeEscapedString(alloc, buf, val.str_val),
        .Array => {
            buf.append(alloc, '[') catch return JqError.OutOfMemory;
            let i: usize = 0;
            for (item of val.arr_val) {
                if (i > 0) buf.append(alloc, ',') catch return JqError.OutOfMemory;
                try writeValueToBuf(alloc, buf, item);
                i += 1;
            }
            buf.append(alloc, ']') catch return JqError.OutOfMemory;
        },
        .Object => {
            buf.append(alloc, '{') catch return JqError.OutOfMemory;
            let i: usize = 0;
            for (entry of val.obj_val) {
                if (i > 0) buf.append(alloc, ',') catch return JqError.OutOfMemory;
                try writeEscapedString(alloc, buf, entry.key);
                buf.append(alloc, ':') catch return JqError.OutOfMemory;
                try writeValueToBuf(alloc, buf, entry.value);
                i += 1;
            }
            buf.append(alloc, '}') catch return JqError.OutOfMemory;
        },
    }
}

pub fn stringifyJson(alloc: std.mem.Allocator, val: JqValue): JqError![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try writeValueToBuf(alloc, &buf, val);
    return buf.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
}
