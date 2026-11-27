// Source: cp/lib/jmespath/value.cp (JMESPath JSON value model & codec)
const std = @import("std");

pub const MAX_DEPTH: usize = 64;

pub const JmesError = error{
    OutOfMemory,
    InvalidJson,
    MaxDepthExceeded,
    InvalidSyntax,
    UnexpectedToken,
    InvalidSliceStep,
    InvalidFunction,
    InvalidArgumentType,
    InvalidArity,
};

pub const JmesEntry = struct {
    key: []const u8,
    value: JmesValue,
};

pub const JmesValue = union(enum) {
    Null: void,
    Bool: bool,
    Int: i64,
    Float: f64,
    String: []const u8,
    Array: []JmesValue,
    Object: []JmesEntry,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Null, .Bool, .Int, .Float => {},
            .String => |s| {
                alloc.free(s);
            },
            .Array => |arr| {
                let i: usize = 0;
                while (i < arr.len) : (i += 1) {
                    arr[i].deinit(alloc);
                }
                alloc.free(arr);
            },
            .Object => |entries| {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    alloc.free(entries[i].key);
                    entries[i].value.deinit(alloc);
                }
                alloc.free(entries);
            },
        }
        _.* = .Null;
    }

    pub fn clone(alloc: std.mem.Allocator): JmesError!JmesValue {
        switch (_) {
            .Null => return .Null,
            .Bool => |b| return JmesValue{ .Bool = b },
            .Int => |v| return JmesValue{ .Int = v },
            .Float => |v| return JmesValue{ .Float = v },
            .String => |s| {
                const copy = alloc.dupe(u8, s) catch return JmesError.OutOfMemory;
                return JmesValue{ .String = copy };
            },
            .Array => |arr| {
                let out = std.ArrayList(JmesValue).empty;
                errdefer {
                    for (*item of out.items) {
                        item.deinit(alloc);
                    }
                    out.deinit(alloc);
                }
                for (item of arr) {
                    const c = item.clone(alloc)?;
                    out.append(alloc, c) catch return JmesError.OutOfMemory;
                }
                const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                return JmesValue{ .Array = slice };
            },
            .Object => |entries| {
                let out = std.ArrayList(JmesEntry).empty;
                errdefer {
                    for (*entry of out.items) {
                        alloc.free(entry.key);
                        entry.value.deinit(alloc);
                    }
                    out.deinit(alloc);
                }
                for (entry of entries) {
                    const k = alloc.dupe(u8, entry.key) catch return JmesError.OutOfMemory;
                    const v = entry.value.clone(alloc) catch |ce| {
                        alloc.free(k);
                        return ce;
                    };
                    out.append(alloc, JmesEntry{ .key = k, .value = v }) catch |ce| {
                        alloc.free(k);
                        let mv = v;
                        mv.deinit(alloc);
                        * = ce;
                        return JmesError.OutOfMemory;
                    };
                }
                const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                return JmesValue{ .Object = slice };
            },
        }
    }

    pub fn asNumber(): f64? {
        switch (_) {
            .Int => |v| return @as(f64, @floatFromInt(v)),
            .Float => |v| return v,
            else => return null,
        }
    }

    pub fn equals(other: JmesValue): bool {
        switch (_) {
            .Null => return other == .Null,
            .Bool => |b| {
                switch (other) {
                    .Bool => |ob| return b == ob,
                    else => return false,
                }
            },
            .Int => |v| {
                switch (other) {
                    .Int => |ov| return v == ov,
                    .Float => |ofv| return @as(f64, @floatFromInt(v)) == ofv,
                    else => return false,
                }
            },
            .Float => |v| {
                switch (other) {
                    .Int => |ov| return v == @as(f64, @floatFromInt(ov)),
                    .Float => |ofv| return v == ofv,
                    else => return false,
                }
            },
            .String => |s| {
                switch (other) {
                    .String => |os| return std.mem.eql(u8, s, os),
                    else => return false,
                }
            },
            .Array => |arr| {
                switch (other) {
                    .Array => |oarr| {
                        if (arr.len != oarr.len) return false;
                        let i: usize = 0;
                        while (i < arr.len) : (i += 1) {
                            if (!arr[i].equals(oarr[i])) return false;
                        }
                        return true;
                    },
                    else => return false,
                }
            },
            .Object => |entries| {
                switch (other) {
                    .Object => |oentries| {
                        if (entries.len != oentries.len) return false;
                        for (entry of entries) {
                            const found = other.getField(entry.key) orelse return false;
                            if (!entry.value.equals(found)) return false;
                        }
                        return true;
                    },
                    else => return false,
                }
            },
        }
    }

    pub fn isTruthy(): bool {
        switch (_) {
            .Null => return false,
            .Bool => |b| return b,
            .Int, .Float => return true,
            .String => |s| return s.len > 0,
            .Array => |arr| return arr.len > 0,
            .Object => |entries| return entries.len > 0,
        }
    }

    pub fn typeName(): []const u8 {
        switch (_) {
            .Null => return "null",
            .Bool => return "boolean",
            .Int, .Float => return "number",
            .String => return "string",
            .Array => return "array",
            .Object => return "object",
        }
    }

    pub fn getField(key: []const u8): JmesValue? {
        switch (_) {
            .Object => |entries| {
                for (entry of entries) {
                    if (std.mem.eql(u8, entry.key, key)) {
                        return entry.value;
                    }
                }
                return null;
            },
            else => return null,
        }
    }
};

const JsonParser = struct {
    input: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,

    fn skipWs(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn parseValue(*_, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.skipWs();
        if (_.pos >= _.input.len) return JmesError.InvalidJson;
        const c = _.input[_.pos];
        if (c == 'n') {
            if (_.pos + 4 <= _.input.len and std.mem.eql(u8, _.input[_.pos .. _.pos + 4], "null")) {
                _.pos += 4;
                return .Null;
            }
            return JmesError.InvalidJson;
        }
        if (c == 't') {
            if (_.pos + 4 <= _.input.len and std.mem.eql(u8, _.input[_.pos .. _.pos + 4], "true")) {
                _.pos += 4;
                return JmesValue{ .Bool = true };
            }
            return JmesError.InvalidJson;
        }
        if (c == 'f') {
            if (_.pos + 5 <= _.input.len and std.mem.eql(u8, _.input[_.pos .. _.pos + 5], "false")) {
                _.pos += 5;
                return JmesValue{ .Bool = false };
            }
            return JmesError.InvalidJson;
        }
        if (c == '"') {
            const s = _.parseString()?;
            return JmesValue{ .String = s };
        }
        if (c == '[') {
            return _.parseArray(depth + 1);
        }
        if (c == '{') {
            return _.parseObject(depth + 1);
        }
        if (c == '-' or (c >= '0' and c <= '9')) {
            return _.parseNumber();
        }
        return JmesError.InvalidJson;
    }

    fn parseString(*_): JmesError![]u8 {
        if (_.pos >= _.input.len or _.input[_.pos] != '"') return JmesError.InvalidJson;
        _.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '"') {
                _.pos += 1;
                return out.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.input.len) return JmesError.InvalidJson;
                const esc = _.input[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => out.append(_.alloc, '"') catch return JmesError.OutOfMemory,
                    '\\' => out.append(_.alloc, '\\') catch return JmesError.OutOfMemory,
                    '/' => out.append(_.alloc, '/') catch return JmesError.OutOfMemory,
                    'b' => out.append(_.alloc, 0x08) catch return JmesError.OutOfMemory,
                    'f' => out.append(_.alloc, 0x0C) catch return JmesError.OutOfMemory,
                    'n' => out.append(_.alloc, '\n') catch return JmesError.OutOfMemory,
                    'r' => out.append(_.alloc, '\r') catch return JmesError.OutOfMemory,
                    't' => out.append(_.alloc, '\t') catch return JmesError.OutOfMemory,
                    'u' => {
                        if (_.pos + 4 > _.input.len) return JmesError.InvalidJson;
                        const hex = _.input[_.pos .. _.pos + 4];
                        _.pos += 4;
                        const cp = std.fmt.parseInt(u21, hex, 16) catch return JmesError.InvalidJson;
                        let buf: [4]u8 = undefined;
                        const n = std.unicode.utf8Encode(cp, &buf) catch return JmesError.InvalidJson;
                        out.appendSlice(_.alloc, buf[0..n]) catch return JmesError.OutOfMemory;
                    },
                    else => return JmesError.InvalidJson,
                }
            } else {
                out.append(_.alloc, c) catch return JmesError.OutOfMemory;
                _.pos += 1;
            }
        }
        return JmesError.InvalidJson;
    }

    fn parseNumber(*_): JmesError!JmesValue {
        const start = _.pos;
        let is_float = false;
        if (_.input[_.pos] == '-') _.pos += 1;
        if (_.pos >= _.input.len) return JmesError.InvalidJson;
        if (_.input[_.pos] == '0') {
            _.pos += 1;
        } else if (_.input[_.pos] >= '1' and _.input[_.pos] <= '9') {
            while (_.pos < _.input.len and _.input[_.pos] >= '0' and _.input[_.pos] <= '9') {
                _.pos += 1;
            }
        } else {
            return JmesError.InvalidJson;
        }
        if (_.pos < _.input.len and _.input[_.pos] == '.') {
            is_float = true;
            _.pos += 1;
            if (_.pos >= _.input.len or _.input[_.pos] < '0' or _.input[_.pos] > '9') {
                return JmesError.InvalidJson;
            }
            while (_.pos < _.input.len and _.input[_.pos] >= '0' and _.input[_.pos] <= '9') {
                _.pos += 1;
            }
        }
        if (_.pos < _.input.len and (_.input[_.pos] == 'e' or _.input[_.pos] == 'E')) {
            is_float = true;
            _.pos += 1;
            if (_.pos < _.input.len and (_.input[_.pos] == '+' or _.input[_.pos] == '-')) {
                _.pos += 1;
            }
            if (_.pos >= _.input.len or _.input[_.pos] < '0' or _.input[_.pos] > '9') {
                return JmesError.InvalidJson;
            }
            while (_.pos < _.input.len and _.input[_.pos] >= '0' and _.input[_.pos] <= '9') {
                _.pos += 1;
            }
        }
        const slice = _.input[start.._.pos];
        if (is_float) {
            const f = std.fmt.parseFloat(f64, slice) catch return JmesError.InvalidJson;
            return JmesValue{ .Float = f };
        } else {
            const iv = std.fmt.parseInt(i64, slice, 10) catch {
                const f = std.fmt.parseFloat(f64, slice) catch return JmesError.InvalidJson;
                return JmesValue{ .Float = f };
            };
            return JmesValue{ .Int = iv };
        }
    }

    fn parseArray(*_, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '['
        _.skipWs();
        let items = std.ArrayList(JmesValue).empty;
        errdefer {
            for (*it of items.items) {
                it.deinit(_.alloc);
            }
            items.deinit(_.alloc);
        }
        if (_.pos < _.input.len and _.input[_.pos] == ']') {
            _.pos += 1;
            const slice = items.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        }
        while (true) {
            const v = _.parseValue(depth)?;
            items.append(_.alloc, v) catch |ce| {
                let mv = v;
                mv.deinit(_.alloc);
                * = ce;
                return JmesError.OutOfMemory;
            };
            _.skipWs();
            if (_.pos >= _.input.len) return JmesError.InvalidJson;
            if (_.input[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            if (_.input[_.pos] != ',') return JmesError.InvalidJson;
            _.pos += 1;
        }
        const slice = items.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
        return JmesValue{ .Array = slice };
    }

    fn parseObject(*_, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '{'
        _.skipWs();
        let entries = std.ArrayList(JmesEntry).empty;
        errdefer {
            for (*e of entries.items) {
                _.alloc.free(e.key);
                e.value.deinit(_.alloc);
            }
            entries.deinit(_.alloc);
        }
        if (_.pos < _.input.len and _.input[_.pos] == '}') {
            _.pos += 1;
            const slice = entries.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Object = slice };
        }
        while (true) {
            _.skipWs();
            const key = _.parseString()?;
            errdefer _.alloc.free(key);
            _.skipWs();
            if (_.pos >= _.input.len or _.input[_.pos] != ':') return JmesError.InvalidJson;
            _.pos += 1;
            const val = _.parseValue(depth)?;
            entries.append(_.alloc, JmesEntry{ .key = key, .value = val }) catch |ce| {
                let mv = val;
                mv.deinit(_.alloc);
                * = ce;
                return JmesError.OutOfMemory;
            };
            _.skipWs();
            if (_.pos >= _.input.len) return JmesError.InvalidJson;
            if (_.input[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            if (_.input[_.pos] != ',') return JmesError.InvalidJson;
            _.pos += 1;
        }
        const slice = entries.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
        return JmesValue{ .Object = slice };
    }
};

pub fn parseJson(alloc: std.mem.Allocator, input: []const u8): JmesError!JmesValue {
    let parser = JsonParser{
        .input = input,
        .pos = 0,
        .alloc = alloc,
    };
    let val = parser.parseValue(0)?;
    parser.skipWs();
    if (parser.pos != parser.input.len) {
        val.deinit(alloc);
        return JmesError.InvalidJson;
    }
    return val;
}

fn appendEscapedString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): JmesError!void {
    out.append(alloc, '"') catch return JmesError.OutOfMemory;
    for (c of s) {
        switch (c) {
            '"' => out.appendSlice(alloc, "\\\"") catch return JmesError.OutOfMemory,
            '\\' => out.appendSlice(alloc, "\\\\") catch return JmesError.OutOfMemory,
            0x08 => out.appendSlice(alloc, "\\b") catch return JmesError.OutOfMemory,
            0x0C => out.appendSlice(alloc, "\\f") catch return JmesError.OutOfMemory,
            '\n' => out.appendSlice(alloc, "\\n") catch return JmesError.OutOfMemory,
            '\r' => out.appendSlice(alloc, "\\r") catch return JmesError.OutOfMemory,
            '\t' => out.appendSlice(alloc, "\\t") catch return JmesError.OutOfMemory,
            else => {
                if (c < 0x20) {
                    let buf: [6]u8 = undefined;
                    const formatted = std.fmt.bufPrint(&buf, "\\u{x:0>4}", .{c}) catch return JmesError.OutOfMemory;
                    out.appendSlice(alloc, formatted) catch return JmesError.OutOfMemory;
                } else {
                    out.append(alloc, c) catch return JmesError.OutOfMemory;
                }
            },
        }
    }
    out.append(alloc, '"') catch return JmesError.OutOfMemory;
}

fn stringifyInto(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: JmesValue): JmesError!void {
    switch (val) {
        .Null => out.appendSlice(alloc, "null") catch return JmesError.OutOfMemory,
        .Bool => |b| {
            if (b) {
                out.appendSlice(alloc, "true") catch return JmesError.OutOfMemory;
            } else {
                out.appendSlice(alloc, "false") catch return JmesError.OutOfMemory;
            }
        },
        .Int => |v| {
            let buf: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "{d}", .{v}) catch return JmesError.OutOfMemory;
            out.appendSlice(alloc, s) catch return JmesError.OutOfMemory;
        },
        .Float => |v| {
            let buf: [64]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "{d}", .{v}) catch return JmesError.OutOfMemory;
            out.appendSlice(alloc, s) catch return JmesError.OutOfMemory;
        },
        .String => |s| appendEscapedString(alloc, out, s)?,
        .Array => |arr| {
            out.append(alloc, '[') catch return JmesError.OutOfMemory;
            let i: usize = 0;
            while (i < arr.len) : (i += 1) {
                if (i > 0) out.append(alloc, ',') catch return JmesError.OutOfMemory;
                stringifyInto(alloc, out, arr[i])?;
            }
            out.append(alloc, ']') catch return JmesError.OutOfMemory;
        },
        .Object => |entries| {
            out.append(alloc, '{') catch return JmesError.OutOfMemory;
            let i: usize = 0;
            while (i < entries.len) : (i += 1) {
                if (i > 0) out.append(alloc, ',') catch return JmesError.OutOfMemory;
                appendEscapedString(alloc, out, entries[i].key)?;
                out.append(alloc, ':') catch return JmesError.OutOfMemory;
                stringifyInto(alloc, out, entries[i].value)?;
            }
            out.append(alloc, '}') catch return JmesError.OutOfMemory;
        },
    }
}

pub fn stringifyJson(alloc: std.mem.Allocator, val: JmesValue): JmesError![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);
    stringifyInto(alloc, &out, val)?;
    return out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
}
