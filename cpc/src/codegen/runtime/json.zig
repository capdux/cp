pub const JsonTag = enum(u8) {
    Null = 0,
    Bool = 1,
    Number = 2,
    String = 3,
    Array = 4,
    Object = 5,
};

pub const JsonField = struct {
    key: []const u8 = "",
    value: JsonValue = .{},
};

pub const JsonValue = struct {
    tag: JsonTag = .Null,
    bool_val: bool = false,
    num_val: f64 = 0,
    str_val: []const u8 = "",
    arr_val: []JsonValue = &.{},
    obj_val: []JsonField = &.{},

    pub fn isNull(self: *const JsonValue) bool {
        return self.tag == .Null;
    }
    pub fn isBool(self: *const JsonValue) bool {
        return self.tag == .Bool;
    }
    pub fn isNumber(self: *const JsonValue) bool {
        return self.tag == .Number;
    }
    pub fn isString(self: *const JsonValue) bool {
        return self.tag == .String;
    }
    pub fn isArray(self: *const JsonValue) bool {
        return self.tag == .Array;
    }
    pub fn isObject(self: *const JsonValue) bool {
        return self.tag == .Object;
    }
    pub fn asBool(self: *const JsonValue) ?bool {
        if (self.tag == .Bool) return self.bool_val;
        return null;
    }
    pub fn asNumber(self: *const JsonValue) ?f64 {
        if (self.tag == .Number) return self.num_val;
        return null;
    }
    pub fn asString(self: *const JsonValue) ?[]const u8 {
        if (self.tag == .String) return self.str_val;
        return null;
    }
    pub fn asArray(self: *const JsonValue) ?[]JsonValue {
        if (self.tag == .Array) return self.arr_val;
        return null;
    }
    pub fn asObject(self: *const JsonValue) ?[]JsonField {
        if (self.tag == .Object) return self.obj_val;
        return null;
    }
    pub fn len(self: *const JsonValue) usize {
        return switch (self.tag) {
            .Array => self.arr_val.len,
            .Object => self.obj_val.len,
            .String => self.str_val.len,
            else => 0,
        };
    }
    pub fn get(self: *const JsonValue, key: []const u8) ?JsonValue {
        if (self.tag != .Object) return null;
        for (self.obj_val) |field| {
            if (std.mem.eql(u8, field.key, key)) return field.value;
        }
        return null;
    }
    pub fn getIndex(self: *const JsonValue, idx: usize) ?JsonValue {
        if (self.tag != .Array or idx >= self.arr_val.len) return null;
        return self.arr_val[idx];
    }
};

pub const FastJsonScanner = struct {
    pub fn indexOfQuoteOrEscape(input: []const u8) ?usize {
        var i: usize = 0;
        const n = input.len;
        const t1: @Vector(16, u8) = @splat('"');
        const t2: @Vector(16, u8) = @splat('\\');
        while (i + 16 <= n) {
            const chunk: @Vector(16, u8) = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const mask: u16 = m1 | m2;
            if (mask != 0) {
                const bit_idx = @ctz(mask);
                return i + bit_idx;
            }
            i += 16;
        }
        while (i < n) : (i += 1) {
            const c = input[i];
            if (c == '"' or c == '\\') return i;
        }
        return null;
    }
};

pub const JsonParser = struct {
    src: []const u8,
    pos: usize = 0,
    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator, src: []const u8) JsonParser {
        return JsonParser{ .src = src, .pos = 0, .allocator = allocator };
    }

    pub fn skipWhitespace(self: *JsonParser) void {
        while (self.pos < self.src.len) {
            const c = self.src[self.pos];
            if (c == ' ' or c == '\t' or c == '\r' or c == '\n') {
                self.pos += 1;
            } else break;
        }
    }

    pub fn parseString(self: *JsonParser) ![]const u8 {
        self.skipWhitespace();
        if (self.pos >= self.src.len or self.src[self.pos] != '"') return error.InvalidToken;
        self.pos += 1;
        const start = self.pos;
        const remaining = self.src[start..];
        if (FastJsonScanner.indexOfQuoteOrEscape(remaining)) |offset| {
            if (remaining[offset] == '"') {
                self.pos = start + offset + 1;
                return self.src[start .. start + offset];
            }
        }
        while (self.pos < self.src.len) {
            const c = self.src[self.pos];
            if (c == '\\') {
                self.pos += 2;
            } else if (c == '"') {
                const s = self.src[start..self.pos];
                self.pos += 1;
                return s;
            } else {
                self.pos += 1;
            }
        }
        return error.UnexpectedEof;
    }

    pub fn parseNumber(self: *JsonParser) !f64 {
        self.skipWhitespace();
        const start = self.pos;
        if (self.pos < self.src.len and (self.src[self.pos] == '-' or self.src[self.pos] == '+')) {
            self.pos += 1;
        }
        while (self.pos < self.src.len) {
            const c = self.src[self.pos];
            if ((c >= '0' and c <= '9') or c == '.' or c == 'e' or c == 'E' or c == '+' or c == '-') {
                self.pos += 1;
            } else break;
        }
        return std.fmt.parseFloat(f64, self.src[start..self.pos]) catch error.InvalidToken;
    }

    pub fn parseValue(self: *JsonParser) anyerror!JsonValue {
        self.skipWhitespace();
        if (self.pos >= self.src.len) return error.UnexpectedEof;
        const ch = self.src[self.pos];
        if (ch == 'n' and self.pos + 4 <= self.src.len and std.mem.eql(u8, self.src[self.pos..self.pos+4], "null")) {
            self.pos += 4;
            return JsonValue{ .tag = .Null };
        }
        if (ch == 't' and self.pos + 4 <= self.src.len and std.mem.eql(u8, self.src[self.pos..self.pos+4], "true")) {
            self.pos += 4;
            return JsonValue{ .tag = .Bool, .bool_val = true };
        }
        if (ch == 'f' and self.pos + 5 <= self.src.len and std.mem.eql(u8, self.src[self.pos..self.pos+5], "false")) {
            self.pos += 5;
            return JsonValue{ .tag = .Bool, .bool_val = false };
        }
        if (ch == '"') return JsonValue{ .tag = .String, .str_val = try self.parseString() };
        if (ch == '-' or (ch >= '0' and ch <= '9')) return JsonValue{ .tag = .Number, .num_val = try self.parseNumber() };
        if (ch == '[') return self.parseArray();
        if (ch == '{') return self.parseObject();
        return error.InvalidToken;
    }

    pub fn parseArray(self: *JsonParser) anyerror!JsonValue {
        self.pos += 1;
        self.skipWhitespace();
        var list: std.ArrayList(JsonValue) = .empty;
        if (self.pos < self.src.len and self.src[self.pos] == ']') {
            self.pos += 1;
            return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(self.allocator) };
        }
        while (true) {
            const val = try self.parseValue();
            try list.append(self.allocator, val);
            self.skipWhitespace();
            if (self.pos >= self.src.len) return error.UnexpectedEof;
            const sep = self.src[self.pos];
            if (sep == ']') {
                self.pos += 1;
                break;
            } else if (sep == ',') {
                self.pos += 1;
            } else return error.ExpectedComma;
        }
        return JsonValue{ .tag = .Array, .arr_val = try list.toOwnedSlice(self.allocator) };
    }

    pub fn parseObject(self: *JsonParser) anyerror!JsonValue {
        self.pos += 1;
        self.skipWhitespace();
        var list: std.ArrayList(JsonField) = .empty;
        if (self.pos < self.src.len and self.src[self.pos] == '}') {
            self.pos += 1;
            return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(self.allocator) };
        }
        while (true) {
            const key = try self.parseString();
            self.skipWhitespace();
            if (self.pos >= self.src.len or self.src[self.pos] != ':') return error.ExpectedColon;
            self.pos += 1;
            const val = try self.parseValue();
            try list.append(self.allocator, JsonField{ .key = key, .value = val });
            self.skipWhitespace();
            if (self.pos >= self.src.len) return error.UnexpectedEof;
            const sep = self.src[self.pos];
            if (sep == '}') {
                self.pos += 1;
                break;
            } else if (sep == ',') {
                self.pos += 1;
            } else return error.ExpectedComma;
        }
        return JsonValue{ .tag = .Object, .obj_val = try list.toOwnedSlice(self.allocator) };
    }
};

pub const JSON = struct {
    pub fn parse(allocator: std.mem.Allocator, src: []const u8) anyerror!JsonValue {
        var p = JsonParser.init(allocator, src);
        return p.parseValue();
    }
    pub fn stringify(obj: anytype) []const u8 {
        return std.fmt.allocPrint(std.heap.smp_allocator, "{f}", .{std.json.fmt(obj, .{})}) catch unreachable;
    }
};
