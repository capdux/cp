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

    pub fn deinit(self: *JmesValue, alloc: std.mem.Allocator) {
        switch (self.*) {
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
        self.* = .Null;
    }

    pub fn clone(self: JmesValue, alloc: std.mem.Allocator): JmesError!JmesValue {
        switch (self) {
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

    pub fn asNumber(self: JmesValue): f64? {
        switch (self) {
            .Int => |v| return @as(f64, @floatFromInt(v)),
            .Float => |v| return v,
            else => return null,
        }
    }

    pub fn equals(self: JmesValue, other: JmesValue): bool {
        switch (self) {
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

    pub fn isTruthy(self: JmesValue): bool {
        switch (self) {
            .Null => return false,
            .Bool => |b| return b,
            .Int, .Float => return true,
            .String => |s| return s.len > 0,
            .Array => |arr| return arr.len > 0,
            .Object => |entries| return entries.len > 0,
        }
    }

    pub fn typeName(self: JmesValue): []const u8 {
        switch (self) {
            .Null => return "null",
            .Bool => return "boolean",
            .Int, .Float => return "number",
            .String => return "string",
            .Array => return "array",
            .Object => return "object",
        }
    }

    pub fn getField(self: JmesValue, key: []const u8): JmesValue? {
        switch (self) {
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

    fn skipWs(self: *JsonParser) {
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                self.pos += 1;
            } else {
                break;
            }
        }
    }

    fn parseValue(self: *JsonParser, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.skipWs();
        if (self.pos >= self.input.len) return JmesError.InvalidJson;
        const c = self.input[self.pos];
        if (c == 'n') {
            if (self.pos + 4 <= self.input.len and std.mem.eql(u8, self.input[self.pos .. self.pos + 4], "null")) {
                self.pos += 4;
                return .Null;
            }
            return JmesError.InvalidJson;
        }
        if (c == 't') {
            if (self.pos + 4 <= self.input.len and std.mem.eql(u8, self.input[self.pos .. self.pos + 4], "true")) {
                self.pos += 4;
                return JmesValue{ .Bool = true };
            }
            return JmesError.InvalidJson;
        }
        if (c == 'f') {
            if (self.pos + 5 <= self.input.len and std.mem.eql(u8, self.input[self.pos .. self.pos + 5], "false")) {
                self.pos += 5;
                return JmesValue{ .Bool = false };
            }
            return JmesError.InvalidJson;
        }
        if (c == '"') {
            const s = self.parseString()?;
            return JmesValue{ .String = s };
        }
        if (c == '[') {
            return self.parseArray(depth + 1);
        }
        if (c == '{') {
            return self.parseObject(depth + 1);
        }
        if (c == '-' or (c >= '0' and c <= '9')) {
            return self.parseNumber();
        }
        return JmesError.InvalidJson;
    }

    fn parseString(self: *JsonParser): JmesError![]u8 {
        if (self.pos >= self.input.len or self.input[self.pos] != '"') return JmesError.InvalidJson;
        self.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(self.alloc);
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == '"') {
                self.pos += 1;
                return out.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\') {
                self.pos += 1;
                if (self.pos >= self.input.len) return JmesError.InvalidJson;
                const esc = self.input[self.pos];
                self.pos += 1;
                switch (esc) {
                    '"' => out.append(self.alloc, '"') catch return JmesError.OutOfMemory,
                    '\\' => out.append(self.alloc, '\\') catch return JmesError.OutOfMemory,
                    '/' => out.append(self.alloc, '/') catch return JmesError.OutOfMemory,
                    'b' => out.append(self.alloc, 0x08) catch return JmesError.OutOfMemory,
                    'f' => out.append(self.alloc, 0x0C) catch return JmesError.OutOfMemory,
                    'n' => out.append(self.alloc, '\n') catch return JmesError.OutOfMemory,
                    'r' => out.append(self.alloc, '\r') catch return JmesError.OutOfMemory,
                    't' => out.append(self.alloc, '\t') catch return JmesError.OutOfMemory,
                    'u' => {
                        if (self.pos + 4 > self.input.len) return JmesError.InvalidJson;
                        const hex = self.input[self.pos .. self.pos + 4];
                        self.pos += 4;
                        const cp = std.fmt.parseInt(u21, hex, 16) catch return JmesError.InvalidJson;
                        let buf: [4]u8 = undefined;
                        const n = std.unicode.utf8Encode(cp, &buf) catch return JmesError.InvalidJson;
                        out.appendSlice(self.alloc, buf[0..n]) catch return JmesError.OutOfMemory;
                    },
                    else => return JmesError.InvalidJson,
                }
            } else {
                out.append(self.alloc, c) catch return JmesError.OutOfMemory;
                self.pos += 1;
            }
        }
        return JmesError.InvalidJson;
    }

    fn parseNumber(self: *JsonParser): JmesError!JmesValue {
        const start = self.pos;
        let is_float = false;
        if (self.input[self.pos] == '-') self.pos += 1;
        if (self.pos >= self.input.len) return JmesError.InvalidJson;
        if (self.input[self.pos] == '0') {
            self.pos += 1;
        } else if (self.input[self.pos] >= '1' and self.input[self.pos] <= '9') {
            while (self.pos < self.input.len and self.input[self.pos] >= '0' and self.input[self.pos] <= '9') {
                self.pos += 1;
            }
        } else {
            return JmesError.InvalidJson;
        }
        if (self.pos < self.input.len and self.input[self.pos] == '.') {
            is_float = true;
            self.pos += 1;
            if (self.pos >= self.input.len or self.input[self.pos] < '0' or self.input[self.pos] > '9') {
                return JmesError.InvalidJson;
            }
            while (self.pos < self.input.len and self.input[self.pos] >= '0' and self.input[self.pos] <= '9') {
                self.pos += 1;
            }
        }
        if (self.pos < self.input.len and (self.input[self.pos] == 'e' or self.input[self.pos] == 'E')) {
            is_float = true;
            self.pos += 1;
            if (self.pos < self.input.len and (self.input[self.pos] == '+' or self.input[self.pos] == '-')) {
                self.pos += 1;
            }
            if (self.pos >= self.input.len or self.input[self.pos] < '0' or self.input[self.pos] > '9') {
                return JmesError.InvalidJson;
            }
            while (self.pos < self.input.len and self.input[self.pos] >= '0' and self.input[self.pos] <= '9') {
                self.pos += 1;
            }
        }
        const slice = self.input[start..self.pos];
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

    fn parseArray(self: *JsonParser, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '['
        self.skipWs();
        let items = std.ArrayList(JmesValue).empty;
        errdefer {
            for (*it of items.items) {
                it.deinit(self.alloc);
            }
            items.deinit(self.alloc);
        }
        if (self.pos < self.input.len and self.input[self.pos] == ']') {
            self.pos += 1;
            const slice = items.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        }
        while (true) {
            const v = self.parseValue(depth)?;
            items.append(self.alloc, v) catch |ce| {
                let mv = v;
                mv.deinit(self.alloc);
                * = ce;
                return JmesError.OutOfMemory;
            };
            self.skipWs();
            if (self.pos >= self.input.len) return JmesError.InvalidJson;
            if (self.input[self.pos] == ']') {
                self.pos += 1;
                break;
            }
            if (self.input[self.pos] != ',') return JmesError.InvalidJson;
            self.pos += 1;
        }
        const slice = items.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
        return JmesValue{ .Array = slice };
    }

    fn parseObject(self: *JsonParser, depth: usize): JmesError!JmesValue {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '{'
        self.skipWs();
        let entries = std.ArrayList(JmesEntry).empty;
        errdefer {
            for (*e of entries.items) {
                self.alloc.free(e.key);
                e.value.deinit(self.alloc);
            }
            entries.deinit(self.alloc);
        }
        if (self.pos < self.input.len and self.input[self.pos] == '}') {
            self.pos += 1;
            const slice = entries.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Object = slice };
        }
        while (true) {
            self.skipWs();
            const key = self.parseString()?;
            errdefer self.alloc.free(key);
            self.skipWs();
            if (self.pos >= self.input.len or self.input[self.pos] != ':') return JmesError.InvalidJson;
            self.pos += 1;
            const val = self.parseValue(depth)?;
            entries.append(self.alloc, JmesEntry{ .key = key, .value = val }) catch |ce| {
                let mv = val;
                mv.deinit(self.alloc);
                * = ce;
                return JmesError.OutOfMemory;
            };
            self.skipWs();
            if (self.pos >= self.input.len) return JmesError.InvalidJson;
            if (self.input[self.pos] == '}') {
                self.pos += 1;
                break;
            }
            if (self.input[self.pos] != ',') return JmesError.InvalidJson;
            self.pos += 1;
        }
        const slice = entries.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
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
