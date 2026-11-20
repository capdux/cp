use super::prelude::ModuleFeatures;
use super::Codegen;

impl Codegen {
    pub(crate) fn emit_early_preludes(
        &mut self,
        features: &ModuleFeatures,
        has_top_err_decl: bool,
        has_top_error_decl: bool,
    ) {
        // --- Tri-State Result, Built-in Error, String Concat & Pattern Helpers ---
        if features.needs_result_error {
            self.writeln("pub fn CpResult(comptime __T: type, comptime __E: type) type {");
            self.writeln(
                "    return union(enum) { ok: __T, err: __E, pub const __IS_CP_RESULT = true; };",
            );
            self.writeln("}");
            self.writeln("const __CpErr = struct {");
            self.writeln("    code: i32 = 0,");
            self.writeln("    msg: []const u8 = \"\",");
            self.writeln("    cause: ?*const anyopaque = null,");
            self.writeln("    cause_type_id: u64 = 0,");
            self.writeln("    pub const __TYPE_ID: u64 = 0xEE00_0000_0000_0001;");
            self.writeln("    pub fn init(__cp_code: i32, __cp_msg: []const u8) __CpErr { return __CpErr{ .code = __cp_code, .msg = __cp_msg }; }");
            self.writeln("    pub fn with_cause(__cp_self: __CpErr, __cp_cause: anytype) __CpErr { const __CpCType = @TypeOf(__cp_cause); var __cp_ptr: ?*const anyopaque = null; var __cp_tid: u64 = 0; if (comptime @typeInfo(__CpCType) == .pointer) { __cp_ptr = @ptrCast(__cp_cause); const __CpChild = @typeInfo(__CpCType).pointer.child; if (comptime @hasDecl(__CpChild, \"__TYPE_ID\")) { __cp_tid = __CpChild.__TYPE_ID; } } else if (comptime (@typeInfo(__CpCType) == .@\"struct\" or @typeInfo(__CpCType) == .@\"union\") and @hasField(__CpCType, \"ptr\")) { __cp_ptr = @ptrCast(__cp_cause.ptr); if (comptime @hasField(__CpCType, \"vtable\")) { __cp_tid = __cp_cause.vtable.__type_id; } } else if (comptime @typeInfo(__CpCType) == .optional) { if (__cp_cause) |__cp_c| return __cp_self.with_cause(__cp_c); } else { if (comptime @hasDecl(__CpCType, \"__TYPE_ID\")) { __cp_tid = __CpCType.__TYPE_ID; } __cp_ptr = @ptrCast(&__cp_cause); } return __CpErr{ .code = __cp_self.code, .msg = __cp_self.msg, .cause = __cp_ptr, .cause_type_id = __cp_tid }; }");
            self.writeln("    pub fn downcast(__cp_self: __CpErr, comptime __CpTarget: type) ?*const __CpTarget {");
            self.writeln("        if (__cp_self.cause) |__cp_c| {");
            self.writeln("            if (comptime @hasDecl(__CpTarget, \"__TYPE_ID\")) {");
            self.writeln("                if (__cp_self.cause_type_id != 0 and __cp_self.cause_type_id != __CpTarget.__TYPE_ID) {");
            self.writeln("                    return null;");
            self.writeln("                }");
            self.writeln("            }");
            self.writeln("            return @ptrCast(@alignCast(__cp_c));");
            self.writeln("        }");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("};");
            if !has_top_err_decl {
                self.writeln("pub const Err = __CpErr;");
            }
            if !has_top_error_decl {
                self.writeln("pub const Error = __CpErr;");
            }
            self.writeln("fn is_cp_result(comptime __T: type) bool {");
            self.writeln(
                "    return @typeInfo(__T) == .@\"union\" and @hasDecl(__T, \"__IS_CP_RESULT\");",
            );
            self.writeln("}");
        }
        if features.needs_opt_chaining {
            self.writeln("fn CpOptType(comptime __T: type) type { if (@typeInfo(__T) == .optional) return __T; return ?__T; }");
            self.writeln("fn CpUnwrapOpt(comptime __T: type) type { if (@typeInfo(__T) == .optional) return @typeInfo(__T).optional.child; return __T; }");
            self.writeln("fn cp_opt_is_null(v: anytype) bool { const __T = @TypeOf(v); if (comptime @typeInfo(__T) == .optional) return v == null; return false; }");
            self.writeln("fn cp_opt_unwrap(v: anytype) CpUnwrapOpt(@TypeOf(v)) { const __T = @TypeOf(v); if (comptime @typeInfo(__T) == .optional) { return v.?; } else { return v; } }");
        }
        if features.needs_cast_int {
            self.writeln("fn cp_cast_int(comptime Dest: type, val: anytype) Dest { const Src = @TypeOf(val); if (comptime Src == Dest) return val; if (comptime Src == comptime_int) { if (comptime @typeInfo(Dest).int.signedness == .unsigned and val < 0) return @as(Dest, @truncate(@as(u128, @bitCast(@as(i128, val))))) else return @as(Dest, @truncate(val)); } const src_bits = @typeInfo(Src).int.bits; const dest_bits = @typeInfo(Dest).int.bits; if (comptime @typeInfo(Src).int.signedness == @typeInfo(Dest).int.signedness) { if (comptime dest_bits >= src_bits) return @as(Dest, val) else return @truncate(val); } if (comptime @typeInfo(Src).int.signedness == .signed) { const s: i128 = if (comptime 128 >= src_bits) @as(i128, val) else @truncate(val); const u: u128 = @bitCast(s); if (comptime dest_bits >= 128) return @as(Dest, u) else return @truncate(u); } else { const u: u128 = if (comptime 128 >= src_bits) @as(u128, val) else @truncate(val); const s: i128 = @bitCast(u); if (comptime dest_bits >= 128) return @as(Dest, s) else return @truncate(s); } }");
        }

    }

    pub(crate) fn emit_runtime_preludes(&mut self, features: &ModuleFeatures) {
        if features.needs_regex {
            self.emit_regex_prelude();
        }

        if features.needs_collections || features.needs_actors {
            self.writeln("");
            self.writeln("const MessageContext = struct {");
            self.writeln("    pub fn hash(self: @This(), m: Message) u64 {");
            self.writeln("        var hasher = std.hash.Wyhash.init(0);");
            self.writeln("        switch (m) {");
            self.writeln("            .int => |i| std.hash.autoHash(&hasher, i),");
            self.writeln(
                "            .float => |f| std.hash.autoHash(&hasher, @as(u32, @bitCast(f))),",
            );
            self.writeln("            .string => |s| hasher.update(s),");
            self.writeln("            .boolean => |bl| std.hash.autoHash(&hasher, bl),");
            self.writeln("            .array => |arr| { for (arr) |item| std.hash.autoHash(&hasher, self.hash(item)); },");
            self.writeln("            .any => |a| std.hash.autoHash(&hasher, @intFromPtr(a)),");
            self.writeln("        }");
            self.writeln("        return hasher.final();");
            self.writeln("    }");
            self.writeln("    pub fn eql(self: @This(), a: Message, b: Message) bool {");
            self.writeln("        const Ta = std.meta.activeTag(a);");
            self.writeln("        const Tb = std.meta.activeTag(b);");
            self.writeln("        if (Ta != Tb) return false;");
            self.writeln("        return switch (a) {");
            self.writeln("            .int => |i| i == b.int,");
            self.writeln("            .float => |f| f == b.float,");
            self.writeln("            .string => |s| std.mem.eql(u8, s, b.string),");
            self.writeln("            .boolean => |bl| bl == b.boolean,");
            self.writeln("            .array => |arr| { if (arr.len != b.array.len) return false; for (arr, 0..) |item, idx| { if (!self.eql(item, b.array[idx])) return false; } return true; },");
            self.writeln("            .any => |ptr| ptr == b.any,");
            self.writeln("        };");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const Message = union(enum) {");
            self.writeln("    int: i32, float: f32, string: []const u8, boolean: bool, array: []Message, any: *anyopaque,");
            self.writeln("    pub fn to_string(self: Message) []const u8 { return switch (self) { .string => |s| s, else => \"\", }; }");
            self.writeln("    pub fn to_int(self: Message) i32 { return switch (self) { .int => |i| i, else => 0, }; }");
            self.writeln("    pub fn format(self: Message, comptime fmt: []const u8, options: std.fmt.FormatOptions, writer: anytype) !void {");
            self.writeln("        switch (self) {");
            self.writeln("            .int => |i| try writer.print(\"{d}\", .{i}),");
            self.writeln("            .float => |f| try writer.print(\"{d}\", .{f}),");
            self.writeln("            .string => |s| try writer.print(\"{s}\", .{s}),");
            self.writeln("            .boolean => |b| try writer.print(\"{}\", .{b}),");
            self.writeln("            .array => |arr| { try writer.print(\"[\", .{}); for (arr, 0..) |item, idx| { if (idx > 0) try writer.print(\", \", .{}); try item.format(fmt, options, writer); } try writer.print(\"]\", .{}); },");
            self.writeln("            .any => |a| try writer.print(\"any@{*}\", .{a}),");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("const MessageEntry = struct { key: Message, value: Message };");
            self.writeln("");
            self.writeln("const Map = struct {");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln("    _map: std.HashMap(Message, Message, MessageContext, std.hash_map.default_max_load_percentage),");
            self.writeln("    size: i32,");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*Map {");
            self.writeln("        const self = try allocator.create(Map);");
            self.writeln("        self.* = .{");
            self.writeln("            .allocator = allocator,");
            self.writeln("            ._map = std.HashMap(Message, Message, MessageContext, std.hash_map.default_max_load_percentage).init(allocator),");
            self.writeln("            .size = 0,");
            self.writeln("        };");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn init(self: *Map) void {");
            self.writeln("        _ = self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn release(self: *Map) void {");
            self.writeln("        self._map.deinit();");
            self.writeln("        self.allocator.destroy(self);");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn set(self: *Map, key: anytype, value: anytype) *Map {");
            self.writeln("        self._map.put(cp_message_from(key), cp_message_from(value)) catch unreachable;");
            self.writeln("        self.size = @intCast(self._map.count());");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn get(self: *Map, key: anytype) ?Message {");
            self.writeln("        return self._map.get(cp_message_from(key));");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn has(self: *Map, key: anytype) bool {");
            self.writeln("        return self._map.contains(cp_message_from(key));");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn delete(self: *Map, key: anytype) bool {");
            self.writeln("        const removed = self._map.remove(cp_message_from(key));");
            self.writeln("        self.size = @intCast(self._map.count());");
            self.writeln("        return removed;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn clear(self: *Map) void {");
            self.writeln("        self._map.clearRetainingCapacity();");
            self.writeln("        self.size = 0;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn put_all(self: *Map, other: *Map) void {");
            self.writeln("        var it = other._map.iterator();");
            self.writeln("        while (it.next()) |entry| {");
            self.writeln(
                "            self._map.put(entry.key_ptr.*, entry.value_ptr.*) catch unreachable;",
            );
            self.writeln("        }");
            self.writeln("        self.size = @intCast(self._map.count());");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn keys(self: *Map) []Message {");
            self.writeln("        const list = self.allocator.alloc(Message, self._map.count()) catch unreachable;");
            self.writeln("        var it = self._map.keyIterator();");
            self.writeln("        var i: usize = 0;");
            self.writeln("        while (it.next()) |k| {");
            self.writeln("            list[i] = k.*;");
            self.writeln("            i += 1;");
            self.writeln("        }");
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn values(self: *Map) []Message {");
            self.writeln("        const list = self.allocator.alloc(Message, self._map.count()) catch unreachable;");
            self.writeln("        var it = self._map.valueIterator();");
            self.writeln("        var i: usize = 0;");
            self.writeln("        while (it.next()) |v| {");
            self.writeln("            list[i] = v.*;");
            self.writeln("            i += 1;");
            self.writeln("        }");
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn entries(self: *Map) []MessageEntry {");
            self.writeln("        const list = self.allocator.alloc(MessageEntry, self._map.count()) catch unreachable;");
            self.writeln("        var it = self._map.iterator();");
            self.writeln("        var i: usize = 0;");
            self.writeln("        while (it.next()) |entry| {");
            self.writeln(
                "            list[i] = .{ .key = entry.key_ptr.*, .value = entry.value_ptr.* };",
            );
            self.writeln("            i += 1;");
            self.writeln("        }");
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const Dict = Map;");
            self.writeln("");
            self.writeln("const Set = struct {");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln("    _map: std.HashMap(Message, void, MessageContext, std.hash_map.default_max_load_percentage),");
            self.writeln("    size: i32,");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*Set {");
            self.writeln("        const self = try allocator.create(Set);");
            self.writeln("        self.* = .{");
            self.writeln("            .allocator = allocator,");
            self.writeln("            ._map = std.HashMap(Message, void, MessageContext, std.hash_map.default_max_load_percentage).init(allocator),");
            self.writeln("            .size = 0,");
            self.writeln("        };");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn init(self: *Set) void {");
            self.writeln("        _ = self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn release(self: *Set) void {");
            self.writeln("        self._map.deinit();");
            self.writeln("        self.allocator.destroy(self);");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn add(self: *Set, value: anytype) *Set {");
            self.writeln("        self._map.put(cp_message_from(value), {}) catch unreachable;");
            self.writeln("        self.size = @intCast(self._map.count());");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn has(self: *Set, value: anytype) bool {");
            self.writeln("        return self._map.contains(cp_message_from(value));");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn delete(self: *Set, value: anytype) bool {");
            self.writeln("        const removed = self._map.remove(cp_message_from(value));");
            self.writeln("        self.size = @intCast(self._map.count());");
            self.writeln("        return removed;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn clear(self: *Set) void {");
            self.writeln("        self._map.clearRetainingCapacity();");
            self.writeln("        self.size = 0;");
            self.writeln("    }");
            self.writeln("    ");
            self.writeln("    pub fn values(self: *Set) []Message {");
            self.writeln("        const list = self.allocator.alloc(Message, self._map.count()) catch unreachable;");
            self.writeln("        var it = self._map.keyIterator();");
            self.writeln("        var i: usize = 0;");
            self.writeln("        while (it.next()) |k| {");
            self.writeln("            list[i] = k.*;");
            self.writeln("            i += 1;");
            self.writeln("        }");
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const HashMap = Map;");
            self.writeln("const HashSet = Set;");
            self.writeln("");
            self.writeln("fn cp_msg_cmp(a: Message, b: Message) i32 {");
            self.writeln("    const tag_a = @intFromEnum(a);");
            self.writeln("    const tag_b = @intFromEnum(b);");
            self.writeln("    if (tag_a != tag_b) return if (tag_a < tag_b) -1 else 1;");
            self.writeln("    switch (a) {");
            self.writeln("        .int => |ia| { const ib = b.int; if (ia < ib) return -1; if (ia > ib) return 1; return 0; },");
            self.writeln("        .float => |fa| { const fb = b.float; if (fa < fb) return -1; if (fa > fb) return 1; return 0; },");
            self.writeln("        .string => |sa| { const sb = b.string; return switch (std.mem.order(u8, sa, sb)) { .lt => -1, .eq => 0, .gt => 1 }; },");
            self.writeln("        .boolean => |ba| { const bb = b.boolean; if (ba == bb) return 0; return if (!ba and bb) -1 else 1; },");
            self.writeln("        .array => |aa| { const ab = b.array; if (aa.len < ab.len) return -1; if (aa.len > ab.len) return 1; return 0; },");
            self.writeln("        .any => |pa| { const pb = b.any; const ia = @intFromPtr(pa); const ib = @intFromPtr(pb); if (ia < ib) return -1; if (ia > ib) return 1; return 0; },");
            self.writeln("    }");
            self.writeln("}");
            self.writeln("");
            self.writeln("const VecDeque = struct {");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln("    buf: []Message,");
            self.writeln("    head: usize,");
            self.writeln("    tail: usize,");
            self.writeln("    count: usize,");
            self.writeln("    capacity: usize,");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*VecDeque {");
            self.writeln("        const self = try allocator.create(VecDeque);");
            self.writeln("        const init_cap: usize = 8;");
            self.writeln("        const b = try allocator.alloc(Message, init_cap);");
            self.writeln("        self.* = .{");
            self.writeln("            .allocator = allocator,");
            self.writeln("            .buf = b,");
            self.writeln("            .head = 0,");
            self.writeln("            .tail = 0,");
            self.writeln("            .count = 0,");
            self.writeln("            .capacity = init_cap,");
            self.writeln("        };");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    pub fn init(self: *VecDeque) void { _ = self; }");
            self.writeln("    pub fn release(self: *VecDeque) void {");
            self.writeln("        self.allocator.free(self.buf);");
            self.writeln("        self.allocator.destroy(self);");
            self.writeln("    }");
            self.writeln("    fn ensure_capacity(self: *VecDeque) void {");
            self.writeln("        if (self.count == self.capacity) {");
            self.writeln("            const new_cap = self.capacity * 2;");
            self.writeln(
            "            const new_buf = self.allocator.alloc(Message, new_cap) catch unreachable;",
        );
            self.writeln("            var i: usize = 0;");
            self.writeln("            while (i < self.count) : (i += 1) {");
            self.writeln("                new_buf[i] = self.buf[(self.head + i) % self.capacity];");
            self.writeln("            }");
            self.writeln("            self.allocator.free(self.buf);");
            self.writeln("            self.buf = new_buf;");
            self.writeln("            self.head = 0;");
            self.writeln("            self.tail = self.count;");
            self.writeln("            self.capacity = new_cap;");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn push_back(self: *VecDeque, val: anytype) void {");
            self.writeln("        self.ensure_capacity();");
            self.writeln("        self.buf[self.tail] = cp_message_from(val);");
            self.writeln("        self.tail = (self.tail + 1) % self.capacity;");
            self.writeln("        self.count += 1;");
            self.writeln("    }");
            self.writeln("    pub fn push_front(self: *VecDeque, val: anytype) void {");
            self.writeln("        self.ensure_capacity();");
            self.writeln(
                "        self.head = if (self.head == 0) self.capacity - 1 else self.head - 1;",
            );
            self.writeln("        self.buf[self.head] = cp_message_from(val);");
            self.writeln("        self.count += 1;");
            self.writeln("    }");
            self.writeln("    pub fn pop_front(self: *VecDeque) ?Message {");
            self.writeln("        if (self.count == 0) return null;");
            self.writeln("        const val = self.buf[self.head];");
            self.writeln("        self.head = (self.head + 1) % self.capacity;");
            self.writeln("        self.count -= 1;");
            self.writeln("        return val;");
            self.writeln("    }");
            self.writeln("    pub fn pop_back(self: *VecDeque) ?Message {");
            self.writeln("        if (self.count == 0) return null;");
            self.writeln(
                "        self.tail = if (self.tail == 0) self.capacity - 1 else self.tail - 1;",
            );
            self.writeln("        const val = self.buf[self.tail];");
            self.writeln("        self.count -= 1;");
            self.writeln("        return val;");
            self.writeln("    }");
            self.writeln("    pub fn peek_front(self: *const VecDeque) ?Message {");
            self.writeln("        if (self.count == 0) return null;");
            self.writeln("        return self.buf[self.head];");
            self.writeln("    }");
            self.writeln("    pub fn peek_back(self: *const VecDeque) ?Message {");
            self.writeln("        if (self.count == 0) return null;");
            self.writeln(
                "        const idx = if (self.tail == 0) self.capacity - 1 else self.tail - 1;",
            );
            self.writeln("        return self.buf[idx];");
            self.writeln("    }");
            self.writeln("    pub fn get(self: *const VecDeque, index: anytype) ?Message {");
            self.writeln("        const m = cp_message_from(index);");
            self.writeln("        const idx_val: usize = switch (m) { .int => |i| if (i >= 0) @intCast(i) else return null, else => return null };");
            self.writeln("        if (idx_val >= self.count) return null;");
            self.writeln("        return self.buf[(self.head + idx_val) % self.capacity];");
            self.writeln("    }");
            self.writeln("    pub fn len(self: *const VecDeque) usize { return self.count; }");
            self.writeln(
                "    pub fn size(self: *const VecDeque) i32 { return @intCast(self.count); }",
            );
            self.writeln(
                "    pub fn is_empty(self: *const VecDeque) bool { return self.count == 0; }",
            );
            self.writeln("    pub fn clear(self: *VecDeque) void { self.head = 0; self.tail = 0; self.count = 0; }");
            self.writeln("    pub fn next(self: *VecDeque) ?Message { return self.pop_front(); }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const BinaryHeap = struct {");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln("    items: std.ArrayList(Message),");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*BinaryHeap {");
            self.writeln("        const self = try allocator.create(BinaryHeap);");
            self.writeln("        self.* = .{ .allocator = allocator, .items = .empty };");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    pub fn init(self: *BinaryHeap) void { _ = self; }");
            self.writeln("    pub fn release(self: *BinaryHeap) void {");
            self.writeln("        self.items.deinit(self.allocator);");
            self.writeln("        self.allocator.destroy(self);");
            self.writeln("    }");
            self.writeln("    pub fn push(self: *BinaryHeap, val: anytype) void {");
            self.writeln("        const m = cp_message_from(val);");
            self.writeln("        self.items.append(self.allocator, m) catch unreachable;");
            self.writeln("        var idx = self.items.items.len - 1;");
            self.writeln("        while (idx > 0) {");
            self.writeln("            const parent = (idx - 1) / 2;");
            self.writeln(
            "            if (cp_msg_cmp(self.items.items[idx], self.items.items[parent]) > 0) {",
        );
            self.writeln("                const tmp = self.items.items[idx];");
            self.writeln("                self.items.items[idx] = self.items.items[parent];");
            self.writeln("                self.items.items[parent] = tmp;");
            self.writeln("                idx = parent;");
            self.writeln("            } else { break; }");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn pop(self: *BinaryHeap) ?Message {");
            self.writeln("        if (self.items.items.len == 0) return null;");
            self.writeln("        const top = self.items.items[0];");
            self.writeln("        const last = self.items.pop();");
            self.writeln("        if (self.items.items.len > 0) {");
            self.writeln("            self.items.items[0] = last.?;");
            self.writeln("            var idx: usize = 0;");
            self.writeln("            const n = self.items.items.len;");
            self.writeln("            while (true) {");
            self.writeln("                var largest = idx;");
            self.writeln("                const left = 2 * idx + 1;");
            self.writeln("                const right = 2 * idx + 2;");
            self.writeln("                if (left < n and cp_msg_cmp(self.items.items[left], self.items.items[largest]) > 0) { largest = left; }");
            self.writeln("                if (right < n and cp_msg_cmp(self.items.items[right], self.items.items[largest]) > 0) { largest = right; }");
            self.writeln("                if (largest != idx) {");
            self.writeln("                    const tmp = self.items.items[idx];");
            self.writeln("                    self.items.items[idx] = self.items.items[largest];");
            self.writeln("                    self.items.items[largest] = tmp;");
            self.writeln("                    idx = largest;");
            self.writeln("                } else { break; }");
            self.writeln("            }");
            self.writeln("        }");
            self.writeln("        return top;");
            self.writeln("    }");
            self.writeln("    pub fn peek(self: *const BinaryHeap) ?Message {");
            self.writeln("        if (self.items.items.len == 0) return null;");
            self.writeln("        return self.items.items[0];");
            self.writeln("    }");
            self.writeln(
                "    pub fn len(self: *const BinaryHeap) usize { return self.items.items.len; }",
            );
            self.writeln(
                "    pub fn size(self: *const BinaryHeap) i32 { return @intCast(self.items.items.len); }",
            );
            self.writeln(
                "    pub fn is_empty(self: *const BinaryHeap) bool { return self.items.items.len == 0; }",
            );
            self.writeln(
                "    pub fn clear(self: *BinaryHeap) void { self.items.clearRetainingCapacity(); }",
            );
            self.writeln("    pub fn next(self: *BinaryHeap) ?Message { return self.pop(); }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const BTreeMap = struct {");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln("    entries: std.ArrayList(MessageEntry),");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*BTreeMap {");
            self.writeln("        const self = try allocator.create(BTreeMap);");
            self.writeln("        self.* = .{ .allocator = allocator, .entries = .empty };");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    pub fn init(self: *BTreeMap) void { _ = self; }");
            self.writeln("    pub fn release(self: *BTreeMap) void {");
            self.writeln("        self.entries.deinit(self.allocator);");
            self.writeln("        self.allocator.destroy(self);");
            self.writeln("    }");
            self.writeln("    fn binary_search(self: *BTreeMap, k: Message) struct { found: bool, index: usize } {");
            self.writeln("        var low: usize = 0;");
            self.writeln("        var high: usize = self.entries.items.len;");
            self.writeln("        while (low < high) {");
            self.writeln("            const mid = low + (high - low) / 2;");
            self.writeln("            const cmp_res = cp_msg_cmp(self.entries.items[mid].key, k);");
            self.writeln("            if (cmp_res == 0) return .{ .found = true, .index = mid };");
            self.writeln("            if (cmp_res < 0) { low = mid + 1; } else { high = mid; }");
            self.writeln("        }");
            self.writeln("        return .{ .found = false, .index = low };");
            self.writeln("    }");
            self.writeln(
                "    pub fn insert(self: *BTreeMap, key: anytype, val: anytype) ?Message {",
            );
            self.writeln("        const k = cp_message_from(key);");
            self.writeln("        const v = cp_message_from(val);");
            self.writeln("        const search = self.binary_search(k);");
            self.writeln("        if (search.found) {");
            self.writeln("            const old = self.entries.items[search.index].value;");
            self.writeln("            self.entries.items[search.index].value = v;");
            self.writeln("            return old;");
            self.writeln("        } else {");
            self.writeln("            self.entries.insert(self.allocator, search.index, .{ .key = k, .value = v }) catch unreachable;");
            self.writeln("            return null;");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn set(self: *BTreeMap, key: anytype, val: anytype) *BTreeMap {");
            self.writeln("        _ = self.insert(key, val);");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    pub fn get(self: *BTreeMap, key: anytype) ?Message {");
            self.writeln("        const k = cp_message_from(key);");
            self.writeln("        const search = self.binary_search(k);");
            self.writeln(
                "        if (search.found) return self.entries.items[search.index].value;",
            );
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("    pub fn has(self: *BTreeMap, key: anytype) bool {");
            self.writeln("        const k = cp_message_from(key);");
            self.writeln("        return self.binary_search(k).found;");
            self.writeln("    }");
            self.writeln("    pub fn remove(self: *BTreeMap, key: anytype) ?Message {");
            self.writeln("        const k = cp_message_from(key);");
            self.writeln("        const search = self.binary_search(k);");
            self.writeln("        if (search.found) {");
            self.writeln("            const old = self.entries.items[search.index].value;");
            self.writeln("            _ = self.entries.orderedRemove(search.index);");
            self.writeln("            return old;");
            self.writeln("        }");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("    pub fn delete(self: *BTreeMap, key: anytype) bool { return self.remove(key) != null; }");
            self.writeln("    pub fn min_key(self: *BTreeMap) ?Message {");
            self.writeln("        if (self.entries.items.len == 0) return null;");
            self.writeln("        return self.entries.items[0].key;");
            self.writeln("    }");
            self.writeln("    pub fn max_key(self: *BTreeMap) ?Message {");
            self.writeln("        if (self.entries.items.len == 0) return null;");
            self.writeln("        return self.entries.items[self.entries.items.len - 1].key;");
            self.writeln("    }");
            self.writeln(
                "    pub fn len(self: *BTreeMap) usize { return self.entries.items.len; }",
            );
            self.writeln(
                "    pub fn size(self: *BTreeMap) i32 { return @intCast(self.entries.items.len); }",
            );
            self.writeln(
                "    pub fn is_empty(self: *BTreeMap) bool { return self.entries.items.len == 0; }",
            );
            self.writeln(
                "    pub fn clear(self: *BTreeMap) void { self.entries.clearRetainingCapacity(); }",
            );
            self.writeln("    pub fn keys(self: *BTreeMap) []Message {");
            self.writeln("        const list = self.allocator.alloc(Message, self.entries.items.len) catch unreachable;");
            self.writeln(
                "        for (self.entries.items, 0..) |entry, i| { list[i] = entry.key; }",
            );
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("    pub fn values(self: *BTreeMap) []Message {");
            self.writeln("        const list = self.allocator.alloc(Message, self.entries.items.len) catch unreachable;");
            self.writeln(
                "        for (self.entries.items, 0..) |entry, i| { list[i] = entry.value; }",
            );
            self.writeln("        return list;");
            self.writeln("    }");
            self.writeln("    pub fn next(self: *BTreeMap) ?MessageEntry {");
            self.writeln("        if (self.entries.items.len == 0) return null;");
            self.writeln("        return self.entries.orderedRemove(0);");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const BTreeSet = struct {");
            self.writeln("    map: *BTreeMap,");
            self.writeln("");
            self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*BTreeSet {");
            self.writeln("        const self = try allocator.create(BTreeSet);");
            self.writeln("        self.map = try BTreeMap._create(allocator);");
            self.writeln("        return self;");
            self.writeln("    }");
            self.writeln("    pub fn init(self: *BTreeSet) void { _ = self; }");
            self.writeln("    pub fn release(self: *BTreeSet) void {");
            self.writeln("        const alloc = self.map.allocator;");
            self.writeln("        self.map.release();");
            self.writeln("        alloc.destroy(self);");
            self.writeln("    }");
            self.writeln("    pub fn add(self: *BTreeSet, val: anytype) bool {");
            self.writeln("        return self.map.insert(val, true) == null;");
            self.writeln("    }");
            self.writeln("    pub fn has(self: *BTreeSet, val: anytype) bool {");
            self.writeln("        return self.map.has(val);");
            self.writeln("    }");
            self.writeln("    pub fn remove(self: *BTreeSet, val: anytype) ?Message {");
            self.writeln("        if (self.map.remove(val) != null) return cp_message_from(true);");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("    pub fn delete(self: *BTreeSet, val: anytype) bool { return self.map.delete(val); }");
            self.writeln("    pub fn min(self: *BTreeSet) ?Message { return self.map.min_key(); }");
            self.writeln("    pub fn max(self: *BTreeSet) ?Message { return self.map.max_key(); }");
            self.writeln("    pub fn len(self: *BTreeSet) usize { return self.map.len(); }");
            self.writeln("    pub fn size(self: *BTreeSet) i32 { return self.map.size(); }");
            self.writeln(
                "    pub fn is_empty(self: *BTreeSet) bool { return self.map.is_empty(); }",
            );
            self.writeln("    pub fn clear(self: *BTreeSet) void { self.map.clear(); }");
            self.writeln(
                "    pub fn values(self: *BTreeSet) []Message { return self.map.keys(); }",
            );
            self.writeln("    pub fn next(self: *BTreeSet) ?Message {");
            self.writeln("        if (self.map.next()) |entry| { return entry.key; }");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
        }

        if features.needs_reflection {
            self.writeln("pub const FieldInfo = struct {");
            self.writeln("    name: []const u8,");
            self.writeln("    typeName: []const u8,");
            self.writeln("    meta: *Map,");
            self.writeln("};");
            self.writeln("");
            self.writeln("pub const MethodInfo = struct {");
            self.writeln("    name: []const u8,");
            self.writeln("    returnTypeName: []const u8,");
            self.writeln("    paramNames: []const []const u8,");
            self.writeln("    isAsync: bool,");
            self.writeln("    isStatic: bool,");
            self.writeln("    meta: *Map,");
            self.writeln("};");
            self.writeln("");
            self.writeln("pub const TypeInfo = struct {");
            self.writeln("    name: []const u8,");
            self.writeln("    isRef: bool,");
            self.writeln("    fields: []const FieldInfo,");
            self.writeln("    methods: []const MethodInfo,");
            self.writeln("    meta: *Map,");
            self.writeln(
                "    pub fn getField(self: *const TypeInfo, field_name: []const u8) ?FieldInfo {",
            );
            self.writeln("        for (self.fields) |f| {");
            self.writeln("            if (std.mem.eql(u8, f.name, field_name)) return f;");
            self.writeln("        }");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln(
                "    pub fn getMethod(self: *const TypeInfo, method_name: []const u8) ?MethodInfo {",
            );
            self.writeln("        for (self.methods) |m| {");
            self.writeln("            if (std.mem.eql(u8, m.name, method_name)) return m;");
            self.writeln("        }");
            self.writeln("        return null;");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
        }

        if features.needs_actors {
            self.writeln("const PID = struct { node_id: u32, local_id: u32 };");
        }

        if features.needs_json && !features.has_imported_json && !self.is_lib_mode {
            self.writeln("// --- CP Standard Native Built-in JSON & Zero-Copy DOM ---");
            self.writeln(include_str!("runtime/json.zig"));
        }

        if features.needs_sync {
            self.writeln("// --- Multithreading Synchronization Primitives (cap:sync) ---");
            self.writeln("const NativeMutex = struct {");
            self.writeln("    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),");
            self.writeln("    pub fn lock(self: *NativeMutex) void {");
            self.writeln("        var spin: usize = 0;");
            self.writeln("        while (true) {");
            self.writeln("            if (self.state.cmpxchgWeak(0, 1, .acquire, .monotonic) == null) return;");
            self.writeln("            if (spin < 100) { std.atomic.spinLoopHint(); spin += 1; } else { std.Thread.yield() catch {}; }");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln(
                "    pub fn unlock(self: *NativeMutex) void { _ = self.state.swap(0, .release); }",
            );
            self.writeln("    pub fn tryLock(self: *NativeMutex) bool { return self.state.cmpxchgWeak(0, 1, .acquire, .monotonic) == null; }");
            self.writeln("    pub fn isLocked(self: *NativeMutex) bool { return self.state.load(.monotonic) != 0; }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const NativeRwLock = struct {");
            self.writeln("    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),");
            self.writeln("    const WRITER_BIT: u32 = 1 << 31;");
            self.writeln("    const READER_MASK: u32 = ~WRITER_BIT;");
            self.writeln("    pub fn lockShared(self: *NativeRwLock) void {");
            self.writeln("        var spin: usize = 0;");
            self.writeln("        while (true) {");
            self.writeln("            const current = self.state.load(.monotonic);");
            self.writeln("            if ((current & WRITER_BIT) == 0) { if (self.state.cmpxchgWeak(current, current + 1, .acquire, .monotonic) == null) return; }");
            self.writeln("            if (spin < 100) { std.atomic.spinLoopHint(); spin += 1; } else { std.Thread.yield() catch {}; }");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn unlockShared(self: *NativeRwLock) void {");
            self.writeln("        while (true) {");
            self.writeln("            const current = self.state.load(.monotonic);");
            self.writeln("            const count = current & READER_MASK;");
            self.writeln("            if (count == 0) return;");
            self.writeln("            if (self.state.cmpxchgWeak(current, current - 1, .release, .monotonic) == null) return;");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn tryLockShared(self: *NativeRwLock) bool { const current = self.state.load(.monotonic); if ((current & WRITER_BIT) != 0) return false; return self.state.cmpxchgWeak(current, current + 1, .acquire, .monotonic) == null; }");
            self.writeln("    pub fn lockExclusive(self: *NativeRwLock) void {");
            self.writeln("        var spin: usize = 0;");
            self.writeln("        while (true) {");
            self.writeln("            if (self.state.cmpxchgWeak(0, WRITER_BIT, .acquire, .monotonic) == null) return;");
            self.writeln("            if (spin < 100) { std.atomic.spinLoopHint(); spin += 1; } else { std.Thread.yield() catch {}; }");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    pub fn unlockExclusive(self: *NativeRwLock) void { _ = self.state.cmpxchgStrong(WRITER_BIT, 0, .release, .monotonic); }");
            self.writeln("    pub fn tryLockExclusive(self: *NativeRwLock) bool { return self.state.cmpxchgWeak(0, WRITER_BIT, .acquire, .monotonic) == null; }");
            self.writeln("    pub fn isReadLocked(self: *NativeRwLock) bool { return (self.state.load(.monotonic) & READER_MASK) > 0; }");
            self.writeln("    pub fn isWriteLocked(self: *NativeRwLock) bool { return (self.state.load(.monotonic) & WRITER_BIT) != 0; }");
            self.writeln("};");
            self.writeln("");
            self.writeln("const CpBareMutex = struct {");
            self.writeln("    raw: NativeMutex = .{},");
            self.writeln("    pub fn lock(self: *CpBareMutex) void { self.raw.lock(); }");
            self.writeln("    pub fn unlock(self: *CpBareMutex) void { self.raw.unlock(); }");
            self.writeln(
                "    pub fn tryLock(self: *CpBareMutex) bool { return self.raw.tryLock(); }",
            );
            self.writeln(
                "    pub fn isLocked(self: *CpBareMutex) bool { return self.raw.isLocked(); }",
            );
            self.writeln("};");
            self.writeln("");
            self.writeln("const CpBareRwLock = struct {");
            self.writeln("    raw: NativeRwLock = .{},");
            self.writeln("    pub fn lock(self: *CpBareRwLock) void { self.raw.lockExclusive(); }");
            self.writeln(
                "    pub fn unlock(self: *CpBareRwLock) void { self.raw.unlockExclusive(); }",
            );
            self.writeln(
                "    pub fn lockShared(self: *CpBareRwLock) void { self.raw.lockShared(); }",
            );
            self.writeln(
                "    pub fn unlockShared(self: *CpBareRwLock) void { self.raw.unlockShared(); }",
            );
            self.writeln("    pub fn read(self: *CpBareRwLock) void { self.raw.lockShared(); }");
            self.writeln(
                "    pub fn write(self: *CpBareRwLock) void { self.raw.lockExclusive(); }",
            );
            self.writeln("    pub fn tryLock(self: *CpBareRwLock) bool { return self.raw.tryLockExclusive(); }");
            self.writeln("    pub fn tryLockShared(self: *CpBareRwLock) bool { return self.raw.tryLockShared(); }");
            self.writeln(
                "    pub fn tryRead(self: *CpBareRwLock) bool { return self.raw.tryLockShared(); }",
            );
            self.writeln("    pub fn tryWrite(self: *CpBareRwLock) bool { return self.raw.tryLockExclusive(); }");
            self.writeln("    pub fn isReadLocked(self: *CpBareRwLock) bool { return self.raw.isReadLocked(); }");
            self.writeln("    pub fn isWriteLocked(self: *CpBareRwLock) bool { return self.raw.isWriteLocked(); }");
            self.writeln("};");
            self.writeln("");
            self.writeln("fn LockGuard(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        mutex: *CpMutex(__T),");
            self.writeln("        data: *__T,");
            self.writeln("        pub fn release(self: Self) void { self.mutex.raw.unlock(); }");
            self.writeln("        pub fn unlock(self: Self) void { self.release(); }");
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn ReadGuard(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        rwlock: *CpRwLock(__T),");
            self.writeln("        data: *const __T,");
            self.writeln(
                "        pub fn release(self: Self) void { self.rwlock.raw.unlockShared(); }",
            );
            self.writeln("        pub fn unlock(self: Self) void { self.release(); }");
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn WriteGuard(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        rwlock: *CpRwLock(__T),");
            self.writeln("        data: *__T,");
            self.writeln(
                "        pub fn release(self: Self) void { self.rwlock.raw.unlockExclusive(); }",
            );
            self.writeln("        pub fn unlock(self: Self) void { self.release(); }");
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn CpMutex(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        raw: NativeMutex = .{},");
            self.writeln("        data: __T,");
            self.writeln("        pub fn init(val: __T) Self { return .{ .data = val }; }");
            self.writeln("        pub fn lock(self: *Self) LockGuard(__T) { self.raw.lock(); return .{ .mutex = self, .data = &self.data }; }");
            self.writeln("        pub fn tryLock(self: *Self) ?LockGuard(__T) { if (self.raw.tryLock()) { return LockGuard(__T){ .mutex = self, .data = &self.data }; } return null; }");
            self.writeln(
                "        pub fn isLocked(self: *Self) bool { return self.raw.isLocked(); }",
            );
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn CpRwLock(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        raw: NativeRwLock = .{},");
            self.writeln("        data: __T,");
            self.writeln("        pub fn init(val: __T) Self { return .{ .data = val }; }");
            self.writeln("        pub fn read(self: *Self) ReadGuard(__T) { self.raw.lockShared(); return .{ .rwlock = self, .data = &self.data }; }");
            self.writeln("        pub fn tryRead(self: *Self) ?ReadGuard(__T) { if (self.raw.tryLockShared()) { return ReadGuard(__T){ .rwlock = self, .data = &self.data }; } return null; }");
            self.writeln("        pub fn write(self: *Self) WriteGuard(__T) { self.raw.lockExclusive(); return .{ .rwlock = self, .data = &self.data }; }");
            self.writeln("        pub fn tryWrite(self: *Self) ?WriteGuard(__T) { if (self.raw.tryLockExclusive()) { return WriteGuard(__T){ .rwlock = self, .data = &self.data }; } return null; }");
            self.writeln(
                "        pub fn isReadLocked(self: *Self) bool { return self.raw.isReadLocked(); }",
            );
            self.writeln("        pub fn isWriteLocked(self: *Self) bool { return self.raw.isWriteLocked(); }");
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn CpAtomic(comptime __T: type) type {");
            self.writeln("    return struct {");
            self.writeln("        const Self = @This();");
            self.writeln("        raw: std.atomic.Value(__T),");
            self.writeln("        pub fn init(val: __T) Self { return .{ .raw = std.atomic.Value(__T).init(val) }; }");
            self.writeln(
                "        pub fn load(self: *const Self) __T { return self.raw.load(.seq_cst); }",
            );
            self.writeln(
                "        pub fn store(self: *Self, val: __T) void { self.raw.store(val, .seq_cst); }",
            );
            self.writeln("        pub fn swap(self: *Self, val: __T) __T { return self.raw.swap(val, .seq_cst); }");
            self.writeln("        pub fn cas(self: *Self, expected: __T, desired: __T) bool { return self.raw.cmpxchgStrong(expected, desired, .seq_cst, .seq_cst) == null; }");
            self.writeln("        pub fn compareAndSwap(self: *Self, expected: __T, desired: __T) bool { return self.cas(expected, desired); }");
            self.writeln("        pub fn add(self: *Self, delta: __T) __T { return self.raw.fetchAdd(delta, .seq_cst); }");
            self.writeln("        pub fn sub(self: *Self, delta: __T) __T { return self.raw.fetchSub(delta, .seq_cst); }");
            self.writeln("        pub fn inc(self: *Self) __T { return self.add(1); }");
            self.writeln("        pub fn dec(self: *Self) __T { return self.sub(1); }");
            self.writeln("        pub fn @\"and\"(self: *Self, mask: __T) __T { return self.raw.fetchAnd(mask, .seq_cst); }");
            self.writeln("        pub fn @\"or\"(self: *Self, mask: __T) __T { return self.raw.fetchOr(mask, .seq_cst); }");
            self.writeln("        pub fn @\"xor\"(self: *Self, mask: __T) __T { return self.raw.fetchXor(mask, .seq_cst); }");
            self.writeln("        pub fn testAndSet(self: *Self) bool { if (comptime __T == bool) { return self.raw.swap(true, .seq_cst); } else { return self.raw.swap(1, .seq_cst) != 0; } }");
            self.writeln("        pub fn clear(self: *Self) void { if (comptime __T == bool) { self.raw.store(false, .seq_cst); } else { self.raw.store(0, .seq_cst); } }");
            self.writeln("    };");
            self.writeln("}");
            self.writeln("");
            self.writeln("fn cp_make_mutex(val: anytype) CpMutex(@TypeOf(val)) { return CpMutex(@TypeOf(val)).init(val); }");
            self.writeln("fn cp_make_rwlock(val: anytype) CpRwLock(@TypeOf(val)) { return CpRwLock(@TypeOf(val)).init(val); }");
            self.writeln("fn CpAtomicConcreteType(comptime __T: type) type { if (__T == comptime_int) return i32; if (__T == comptime_float) return f32; return __T; }");
            self.writeln("fn cp_make_atomic(val: anytype) CpAtomic(CpAtomicConcreteType(@TypeOf(val))) { const __T = CpAtomicConcreteType(@TypeOf(val)); return CpAtomic(__T).init(@as(__T, val)); }");
            self.writeln("");
        }

        if features.needs_collections || features.needs_actors {
            self.writeln("fn CpIndexReturnType(comptime __T: type) type {");
            self.writeln("    if (@typeInfo(__T) == .pointer) {");
            self.writeln("        const Child = @typeInfo(__T).pointer.child;");
            self.writeln("        if (@typeInfo(Child) == .@\"struct\" and @hasDecl(Child, \"get\")) return ?Message;");
            self.writeln(
                "        if (@typeInfo(Child) == .array) return @typeInfo(Child).array.child;",
            );
            self.writeln(
                "        if (@typeInfo(Child) == .pointer and @typeInfo(Child).pointer.size == .slice) return @typeInfo(Child).pointer.child;",
            );
            self.writeln("    }");
            self.writeln("    if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice) return @typeInfo(__T).pointer.child;");
            self.writeln("    if (@typeInfo(__T) == .array) return @typeInfo(__T).array.child;");
            self.writeln("    return ?Message;");
            self.writeln("}");
            self.writeln("fn cp_index_get(container: anytype, key: anytype) CpIndexReturnType(@TypeOf(container)) {");
            self.writeln("    const __T = @TypeOf(container);");
            self.writeln("    if (comptime (__T == Message or __T == ?Message)) {");
            self.writeln(
                "        const val: ?Message = if (__T == ?Message) container else container;",
            );
            self.writeln("        if (val) |m| {");
            self.writeln("            switch (m) {");
            self.writeln("                .array => |arr| {");
            self.writeln("                    if (comptime (@typeInfo(@TypeOf(key)) == .int or @typeInfo(@TypeOf(key)) == .comptime_int)) {");
            self.writeln("                        const idx: usize = @intCast(key);");
            self.writeln("                        if (idx < arr.len) return arr[idx];");
            self.writeln("                    }");
            self.writeln("                    return null;");
            self.writeln("                },");
            self.writeln("                .any => |ptr| {");
            self.writeln("                    const map: *Map = @ptrCast(@alignCast(ptr));");
            self.writeln("                    return map.get(key);");
            self.writeln("                },");
            self.writeln("                else => return null,");
            self.writeln("            }");
            self.writeln("        } else {");
            self.writeln("            return null;");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("    if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .@\"struct\" and @hasDecl(@typeInfo(__T).pointer.child, \"get\"))) {");
            self.writeln("        return container.get(key);");
            self.writeln("    } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice)) {");
            self.writeln("        return container.*[@as(usize, @intCast(key))];");
            self.writeln("    } else {");
            self.writeln("        return container[@as(usize, @intCast(key))];");
            self.writeln("    }");
            self.writeln("}");
            self.writeln(
                "fn cp_index_assign(container: anytype, key: anytype, value: anytype) void {",
            );
            self.writeln("    const __T = @TypeOf(container);");
            self.writeln("    if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .@\"struct\" and @hasDecl(@typeInfo(__T).pointer.child, \"set\"))) {");
            self.writeln("        _ = container.set(key, value);");
            self.writeln("    } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice)) {");
            self.writeln("        @constCast(container.*)[@as(usize, @intCast(key))] = value;");
            self.writeln("    } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice)) {");
            self.writeln("        @constCast(container)[@as(usize, @intCast(key))] = value;");
            self.writeln("    } else {");
            self.writeln("        container[@as(usize, @intCast(key))] = value;");
            self.writeln("    }");
            self.writeln("}");
            self.writeln("fn cp_message_from(v: anytype) Message {");
            self.writeln("    const __T = @TypeOf(v);");
            self.writeln("    if (__T == Message) return v;");
            self.writeln(
                "    if (__T == []Message or __T == []const Message) return .{ .array = @constCast(v) };",
            );
            self.writeln(
                "    if (__T == i32 or __T == comptime_int) return .{ .int = @intCast(v) };
    if (__T == bool) return .{ .boolean = v };",
            );
            self.writeln(
                "    if (__T == f32 or __T == comptime_float) return .{ .float = @floatCast(v) };",
            );
            self.writeln(
                "    if (__T == []const u8 or __T == []u8) return .{ .string = v };
    if (@typeInfo(__T) == .pointer) {
        const Child = @typeInfo(__T).pointer.child;
        if (Child == u8) return .{ .string = v };
        if (@typeInfo(Child) == .array and @typeInfo(Child).array.child == u8) {
            const cp_str_slice: []const u8 = v;
            return .{ .string = cp_str_slice };
        }
        return .{ .any = @ptrCast(@constCast(v)) };
    }",
            );
            self.writeln("    return .{ .any = @ptrCast(@constCast(&v)) };");
            self.writeln("}");
        }

        if features.needs_string_helpers || features.needs_unpack {
            self.writeln("fn is_string_like(comptime __T: type) bool { if (__T == []const u8 or __T == []u8) return true; if (@typeInfo(__T) == .pointer) { const Child = @typeInfo(__T).pointer.child; if (Child == u8) return true; if (@typeInfo(Child) == .array and @typeInfo(Child).array.child == u8) return true; } return false; }");
            self.writeln("fn to_string_slice(v: anytype) []const u8 { const __T = @TypeOf(v); if (comptime (__T == []const u8 or __T == []u8)) { return v; } else { const cp_str_slice: []const u8 = v; return cp_str_slice; } }");
            self.writeln("fn CpValType(comptime __T: type) type { if (is_string_like(__T)) return []const u8; if (__T == comptime_int) return i32; if (__T == comptime_float) return f32; return __T; }");
            self.writeln("fn cp_val(v: anytype) CpValType(@TypeOf(v)) { const __T = @TypeOf(v); if (comptime is_string_like(__T)) { return to_string_slice(v); } else if (comptime __T == comptime_int) { return @as(i32, v); } else if (comptime __T == comptime_float) { return @as(f32, v); } else { return v; } }");
        }

        if features.needs_add {
            if features.needs_collections || features.needs_actors {
                self.writeln("fn CpAddReturn(comptime Ta: type, comptime Tb: type) type { if (Ta == Message and Tb == Message) return i32; if (Ta == Message and @typeInfo(Tb) == .int) return Tb; if (@typeInfo(Ta) == .int and Tb == Message) return Ta; if (is_string_like(Ta) or is_string_like(Tb)) return []const u8; if (Ta == comptime_int and Tb == comptime_int) return i32; if (Ta == comptime_int) return Tb; if (Tb == comptime_int) return Ta; if (Ta == comptime_float and Tb == comptime_float) return f32; if (Ta == comptime_float) return Tb; if (Tb == comptime_float) return Ta; return Ta; }");
                self.writeln("fn cp_add(a: anytype, b: anytype) CpAddReturn(@TypeOf(a), @TypeOf(b)) { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime (Ta == Message and Tb == Message)) { return a.to_int() + b.to_int(); } else if (comptime (Ta == Message and @typeInfo(Tb) == .int)) { return @as(Tb, @intCast(a.to_int())) + b; } else if (comptime (@typeInfo(Ta) == .int and Tb == Message)) { return a + @as(Ta, @intCast(b.to_int())); } else if (comptime (is_string_like(Ta) and is_string_like(Tb))) { return std.fmt.allocPrint(std.heap.page_allocator, \"{s}{s}\", .{ to_string_slice(a), to_string_slice(b) }) catch unreachable; } else if (comptime is_string_like(Ta)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{s}{any}\", .{ to_string_slice(a), b }) catch unreachable; } else if (comptime is_string_like(Tb)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{any}{s}\", .{ a, to_string_slice(b) }) catch unreachable; } else if (comptime (@typeInfo(Ta) == .int and @typeInfo(Tb) == .int)) { return a + @as(Ta, @intCast(b)); } else { return a + b; } }");
            } else {
                self.writeln("fn CpAddReturn(comptime Ta: type, comptime Tb: type) type { if (is_string_like(Ta) or is_string_like(Tb)) return []const u8; if (Ta == comptime_int and Tb == comptime_int) return i32; if (Ta == comptime_int) return Tb; if (Tb == comptime_int) return Ta; if (Ta == comptime_float and Tb == comptime_float) return f32; if (Ta == comptime_float) return Tb; if (Tb == comptime_float) return Ta; return Ta; }");
                self.writeln("fn cp_add(a: anytype, b: anytype) CpAddReturn(@TypeOf(a), @TypeOf(b)) { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime (is_string_like(Ta) and is_string_like(Tb))) { return std.fmt.allocPrint(std.heap.page_allocator, \"{s}{s}\", .{ to_string_slice(a), to_string_slice(b) }) catch unreachable; } else if (comptime is_string_like(Ta)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{s}{any}\", .{ to_string_slice(a), b }) catch unreachable; } else if (comptime is_string_like(Tb)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{any}{s}\", .{ a, to_string_slice(b) }) catch unreachable; } else if (comptime (@typeInfo(Ta) == .int and @typeInfo(Tb) == .int)) { return a + @as(Ta, @intCast(b)); } else { return a + b; } }");
            }
        }

        if features.needs_result_error {
            self.writeln("fn CpUnwrapOkType(comptime __T: type) type { if (is_cp_result(__T)) { const OkT = @FieldType(__T, \"ok\"); if (@typeInfo(OkT) == .optional) return @typeInfo(OkT).optional.child; return OkT; } if (@typeInfo(__T) == .error_union) { const Payload = @typeInfo(__T).error_union.payload; if (@typeInfo(Payload) == .optional) return @typeInfo(Payload).optional.child; return Payload; } if (@typeInfo(__T) == .optional) return @typeInfo(__T).optional.child; return __T; }");
            self.writeln("fn CpUnwrapErrType(comptime __T: type) type { if (is_cp_result(__T)) return @FieldType(__T, \"err\"); if (@typeInfo(__T) == .error_union) return @typeInfo(__T).error_union.error_set; return anyerror; }");
            self.writeln("fn cp_has_ok_val(v: anytype) bool { const __T = @TypeOf(v); if (comptime is_cp_result(__T)) { if (v != .ok) return false; const OkT = @FieldType(__T, \"ok\"); if (comptime @typeInfo(OkT) == .optional) { return v.ok != null; } else { return true; } } else if (comptime @typeInfo(__T) == .error_union) { const Payload = @typeInfo(__T).error_union.payload; if (comptime @typeInfo(Payload) == .optional) { if (v) |opt_val| return opt_val != null else |_| return false; } else { if (v) |_| return true else |_| return false; } } else if (comptime @typeInfo(__T) == .optional) { return v != null; } else { return true; } }");
            self.writeln("fn cp_unwrap_ok_val(v: anytype) CpUnwrapOkType(@TypeOf(v)) { const __T = @TypeOf(v); if (comptime is_cp_result(__T)) { const OkT = @FieldType(__T, \"ok\"); if (comptime @typeInfo(OkT) == .optional) { return v.ok.?; } else { return v.ok; } } else if (comptime @typeInfo(__T) == .error_union) { const payload = v catch unreachable; if (comptime @typeInfo(@TypeOf(payload)) == .optional) { return payload.?; } else { return payload; } } else if (comptime @typeInfo(__T) == .optional) { return v.?; } else { return v; } }");
            self.writeln("fn cp_unwrap_err_val(v: anytype) CpUnwrapErrType(@TypeOf(v)) { const __T = @TypeOf(v); if (comptime is_cp_result(__T)) return v.err; if (comptime @typeInfo(__T) == .error_union) { if (v) |_| unreachable else |e| return e; } return v; }");
            self.writeln("fn cp_is_null_val(v: anytype) bool { const __T = @TypeOf(v); if (comptime is_cp_result(__T)) { if (v != .ok) return false; const OkT = @FieldType(__T, \"ok\"); if (comptime @typeInfo(OkT) == .optional) { return v.ok == null; } else { return false; } } else if (comptime @typeInfo(__T) == .optional) { return v == null; } else { return false; } }");
            self.writeln("fn cp_convert_err(comptime __CpTargetErr: type, __cp_err_val: anytype) __CpTargetErr { const __CpSrcType = @TypeOf(__cp_err_val); if (comptime __CpSrcType == __CpTargetErr) { return __cp_err_val; } else if (comptime __CpTargetErr == __CpErr) { if (comptime @typeInfo(__CpSrcType) == .@\"union\") { return __CpErr{ .code = @intCast(@intFromEnum(std.meta.activeTag(__cp_err_val))), .msg = @tagName(__cp_err_val) }; } else if (comptime @typeInfo(__CpSrcType) == .@\"enum\") { return __CpErr{ .code = @intCast(@intFromEnum(__cp_err_val)), .msg = @tagName(__cp_err_val) }; } else if (comptime @typeInfo(__CpSrcType) == .error_set) { return __CpErr{ .code = 0, .msg = @errorName(__cp_err_val) }; } else if (comptime @typeInfo(__CpSrcType) == .@\"struct\") { var __err_code: i32 = 0; var __cp_m: []const u8 = \"\"; if (comptime @hasField(__CpSrcType, \"code\")) { __err_code = @as(i32, @intCast(__cp_err_val.code)); } if (comptime @hasField(__CpSrcType, \"msg\")) { __cp_m = to_string_slice(__cp_err_val.msg); } else if (comptime @hasField(__CpSrcType, \"message\")) { __cp_m = to_string_slice(__cp_err_val.message); } else if (comptime @hasDecl(__CpSrcType, \"message\")) { __cp_m = to_string_slice(__cp_err_val.message()); } else { __cp_m = @typeName(__CpSrcType); } return __CpErr.init(__err_code, __cp_m); } else if (comptime @typeInfo(__CpSrcType) == .pointer and @typeInfo(@typeInfo(__CpSrcType).pointer.child) == .@\"struct\") { const __CpChild = @typeInfo(__CpSrcType).pointer.child; var __err_code: i32 = 0; var __cp_m: []const u8 = \"\"; if (comptime @hasField(__CpChild, \"code\")) { __err_code = @as(i32, @intCast(__cp_err_val.code)); } if (comptime @hasField(__CpChild, \"msg\")) { __cp_m = to_string_slice(__cp_err_val.msg); } else if (comptime @hasField(__CpChild, \"message\")) { __cp_m = to_string_slice(__cp_err_val.message); } else if (comptime @hasDecl(__CpChild, \"message\")) { __cp_m = to_string_slice(__cp_err_val.message()); } else { __cp_m = @typeName(__CpChild); } return __CpErr.init(__err_code, __cp_m).with_cause(__cp_err_val); } else { return __CpErr{ .code = -1, .msg = \"error\" }; } } else if (comptime @typeInfo(__CpTargetErr) == .pointer and @typeInfo(__CpSrcType) != .pointer) { return @ptrCast(@constCast(&__cp_err_val)); } else if (comptime @typeInfo(__CpSrcType) == .pointer and @typeInfo(__CpTargetErr) != .pointer and @typeInfo(__CpSrcType).pointer.child == __CpTargetErr) { return __cp_err_val.*; } else if (comptime @typeInfo(__CpTargetErr) == .@\"union\" and @hasDecl(__CpTargetErr, \"__from\")) { return __CpTargetErr.__from(__cp_err_val); } else if (comptime @hasDecl(__CpTargetErr, \"from\")) { return __CpTargetErr.from(__cp_err_val); } else { return __cp_err_val; } }");
        }

        if features.needs_result_error {
            self.writeln("fn cp_match_tag(__cp_val: anytype, comptime __cp_tag: anytype) bool { const __T = @TypeOf(__cp_val); if (comptime @typeInfo(__T) == .@\"enum\") { return __cp_val == __cp_tag; } else if (comptime @typeInfo(__T) == .error_set) { return __cp_val == @field(anyerror, @tagName(__cp_tag)); } else if (comptime (__T == __CpErr or (@typeInfo(__T) == .@\"struct\" and @hasField(__T, \"msg\")))) { return std.mem.eql(u8, __cp_val.msg, @tagName(__cp_tag)); } else if (comptime @typeInfo(__T) == .@\"union\") { return std.meta.activeTag(__cp_val) == __cp_tag; } else { return false; } }");
        }

        if features.needs_not {
            self.writeln("inline fn cp_not(v: anytype) bool { const __T = @TypeOf(v); if (comptime __T == bool) { return !v; } else if (comptime @typeInfo(__T) == .optional) { return v == null; } else if (comptime is_string_like(__T)) { return to_string_slice(v).len == 0; } else if (comptime (@typeInfo(__T) == .int or __T == comptime_int)) { return v == 0; } else { return false; } }");
        }

        if features.needs_null_coalesce {
            if features.needs_collections || features.needs_actors {
                self.writeln("fn CpNullCoalesceReturn(comptime TOpt: type, comptime TFb: type) type { if (@typeInfo(TOpt) == .optional) { const Child = @typeInfo(TOpt).optional.child; if (Child == Message) return Message; if (is_string_like(Child)) return []const u8; return Child; } return CpValType(TFb); }");
                self.writeln("fn cp_null_coalesce(opt_val: anytype, fallback: anytype) CpNullCoalesceReturn(@TypeOf(opt_val), @TypeOf(fallback)) { const TOpt = @TypeOf(opt_val); const RType = CpNullCoalesceReturn(TOpt, @TypeOf(fallback)); if (comptime @typeInfo(TOpt) == .optional) { if (opt_val) |v| { if (comptime RType == Message) { return v; } else if (comptime is_string_like(RType)) { return to_string_slice(v); } else { return v; } } else { if (comptime RType == Message) { return cp_message_from(fallback); } else if (comptime is_string_like(RType)) { return to_string_slice(fallback); } else { return fallback; } } } return cp_val(fallback); }");
            } else {
                self.writeln("fn CpNullCoalesceReturn(comptime TOpt: type, comptime TFb: type) type { if (@typeInfo(TOpt) == .optional) { const Child = @typeInfo(TOpt).optional.child; if (is_string_like(Child)) return []const u8; return Child; } return CpValType(TFb); }");
                self.writeln("fn cp_null_coalesce(opt_val: anytype, fallback: anytype) CpNullCoalesceReturn(@TypeOf(opt_val), @TypeOf(fallback)) { const TOpt = @TypeOf(opt_val); const RType = CpNullCoalesceReturn(TOpt, @TypeOf(fallback)); if (comptime @typeInfo(TOpt) == .optional) { if (opt_val) |v| { if (comptime is_string_like(RType)) { return to_string_slice(v); } else { return v; } } else { if (comptime is_string_like(RType)) { return to_string_slice(fallback); } else { return fallback; } } } return cp_val(fallback); }");
            }
        }

        if features.needs_fmt {
            if features.needs_collections || features.needs_actors {
                self.writeln("fn cp_fmt(v: anytype) []const u8 { const __T = @TypeOf(v); if (comptime is_string_like(__T)) { return to_string_slice(v); } else if (comptime __T == bool) { return if (v) \"true\" else \"false\"; } else if (comptime (@typeInfo(__T) == .int or __T == comptime_int)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{v}) catch unreachable; } else if (comptime (@typeInfo(__T) == .float or __T == comptime_float)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{v}) catch unreachable; } else if (comptime @typeInfo(__T) == .@\"enum\") { return @tagName(v); } else if (comptime __T == Message) { return switch (v) { .string => |s| s, .int => |i| std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{i}) catch unreachable, .float => |f| std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{f}) catch unreachable, .boolean => |b| if (b) \"true\" else \"false\", else => std.fmt.allocPrint(std.heap.page_allocator, \"{any}\", .{v}) catch unreachable, }; } else if (comptime @typeInfo(__T) == .optional) { if (v) |unwrapped| { return cp_fmt(unwrapped); } else { return \"null\"; } } else { return std.fmt.allocPrint(std.heap.page_allocator, \"{any}\", .{v}) catch unreachable; } }");
            } else {
                self.writeln("fn cp_fmt(v: anytype) []const u8 { const __T = @TypeOf(v); if (comptime is_string_like(__T)) { return to_string_slice(v); } else if (comptime __T == bool) { return if (v) \"true\" else \"false\"; } else if (comptime (@typeInfo(__T) == .int or __T == comptime_int)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{v}) catch unreachable; } else if (comptime (@typeInfo(__T) == .float or __T == comptime_float)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{v}) catch unreachable; } else if (comptime @typeInfo(__T) == .@\"enum\") { return @tagName(v); } else if (comptime @typeInfo(__T) == .optional) { if (v) |unwrapped| { return cp_fmt(unwrapped); } else { return \"null\"; } } else { return std.fmt.allocPrint(std.heap.page_allocator, \"{any}\", .{v}) catch unreachable; } }");
            }
        }

        if features.needs_math_random {
            self.writeln("fn cp_math_random() f32 {");
            self.writeln("    return 0.5; // Stub");
            self.writeln("}");
        }

        if features.needs_equality {
            self.writeln("fn cp_eq(a: anytype, b: anytype) bool {");
            self.writeln("    const Ta = @TypeOf(a); const Tb = @TypeOf(b);");
            self.writeln(
                "    if (comptime @typeInfo(Ta) == .null and @typeInfo(Tb) == .null) return true;",
            );
            self.writeln("    if (comptime @typeInfo(Ta) == .null) { if (comptime @typeInfo(Tb) == .optional) return b == null; return false; }");
            self.writeln("    if (comptime @typeInfo(Tb) == .null) { if (comptime @typeInfo(Ta) == .optional) return a == null; return false; }");
            if features.needs_collections || features.needs_actors {
                self.writeln("    if (comptime @typeInfo(Ta) == .optional and @typeInfo(Ta).optional.child == Message) { if (a) |unwrapped_a| return cp_eq(unwrapped_a, b); return false; }");
                self.writeln("    if (comptime @typeInfo(Tb) == .optional and @typeInfo(Tb).optional.child == Message) { if (b) |unwrapped_b| return cp_eq(a, unwrapped_b); return false; }");
                self.writeln("    if (comptime Ta == Message and is_string_like(Tb)) { return switch (a) { .string => |s| std.mem.eql(u8, s, to_string_slice(b)), else => false, }; }");
                self.writeln("    if (comptime Tb == Message and is_string_like(Ta)) { return switch (b) { .string => |s| std.mem.eql(u8, to_string_slice(a), s), else => false, }; }");
                self.writeln("    if (comptime Ta == Message and (Tb == i32 or Tb == comptime_int)) { return switch (a) { .int => |i| i == b, else => false, }; }");
                self.writeln("    if (comptime Tb == Message and (Ta == i32 or Ta == comptime_int)) { return switch (b) { .int => |i| a == i, else => false, }; }");
                self.writeln("    if (comptime Ta == Message and (Tb == f32 or Tb == comptime_float)) { return switch (a) { .float => |f| f == b, else => false, }; }");
                self.writeln("    if (comptime Tb == Message and (Ta == f32 or Ta == comptime_float)) { return switch (b) { .float => |f| a == f, else => false, }; }");
                self.writeln("    if (comptime Ta == Message and Tb == bool) { return switch (a) { .boolean => |bl| bl == b, else => false, }; }");
                self.writeln("    if (comptime Tb == Message and Ta == bool) { return switch (b) { .boolean => |bl| a == bl, else => false, }; }");
                self.writeln("    if (comptime Ta == Message and Tb == Message) { return MessageContext.eql(.{}, a, b); }");
            }
            self.writeln("    if (comptime is_string_like(Ta) and is_string_like(Tb)) { return std.mem.eql(u8, to_string_slice(a), to_string_slice(b)); }");
            self.writeln("    if (comptime (@typeInfo(Ta) == .int or Ta == comptime_int) and (@typeInfo(Tb) == .int or Tb == comptime_int)) { return a == b; }");
            self.writeln("    if (comptime (@typeInfo(Ta) == .float or Ta == comptime_float) and (@typeInfo(Tb) == .float or Tb == comptime_float)) { return a == b; }");
            self.writeln("    if (comptime @typeInfo(Ta) == .optional and @typeInfo(Tb) == .optional) { if (a) |va| { if (b) |vb| return cp_eq(va, vb); return false; } else { return b == null; } }");
            self.writeln("    if (comptime @typeInfo(Ta) == .optional) { if (a) |va| return cp_eq(va, b); return false; }");
            self.writeln("    if (comptime @typeInfo(Tb) == .optional) { if (b) |vb| return cp_eq(a, vb); return false; }");
            self.writeln("    if (comptime Ta != Tb) return false;");
            self.writeln("    return a == b;");
            self.writeln("}");
            self.writeln("fn cp_neq(a: anytype, b: anytype) bool { return !cp_eq(a, b); }");
        }

        if features.needs_comparison {
            self.writeln("fn cp_lt(a: anytype, b: anytype) bool { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime is_string_like(Ta) and is_string_like(Tb)) { return std.mem.order(u8, to_string_slice(a), to_string_slice(b)) == .lt; } return a < b; }");
            self.writeln("fn cp_gt(a: anytype, b: anytype) bool { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime is_string_like(Ta) and is_string_like(Tb)) { return std.mem.order(u8, to_string_slice(a), to_string_slice(b)) == .gt; } return a > b; }");
            self.writeln("fn cp_lte(a: anytype, b: anytype) bool { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime is_string_like(Ta) and is_string_like(Tb)) { const ord = std.mem.order(u8, to_string_slice(a), to_string_slice(b)); return ord == .lt or ord == .eq; } return a <= b; }");
            self.writeln("fn cp_gte(a: anytype, b: anytype) bool { const Ta = @TypeOf(a); const Tb = @TypeOf(b); if (comptime is_string_like(Ta) and is_string_like(Tb)) { const ord = std.mem.order(u8, to_string_slice(a), to_string_slice(b)); return ord == .gt or ord == .eq; } return a >= b; }");
        }

        if features.needs_hash {
            self.writeln("fn cp_hash_val(__cp_val: anytype) usize { const __T = @TypeOf(__cp_val); if (comptime is_string_like(__T)) { const __cp_s = to_string_slice(__cp_val); var __cp_h: usize = 2166136261; for (__cp_s) |__cp_byte| { __cp_h = (__cp_h ^ __cp_byte) *% 16777619; } return __cp_h; } if (comptime @typeInfo(__T) == .int or @typeInfo(__T) == .float or @typeInfo(__T) == .bool) { var __cp_h: usize = 2166136261; const __cp_bytes = std.mem.asBytes(&__cp_val); for (__cp_bytes) |__cp_byte| { __cp_h = (__cp_h ^ __cp_byte) *% 16777619; } return __cp_h; } if (comptime @typeInfo(__T) == .pointer) { if (@hasDecl(@typeInfo(__T).pointer.child, \"hash\")) { return __cp_val.hash(); } } return 0; }");
        }

        if features.needs_array_helpers {
            self.writeln("fn cp_array_lit(comptime __Elem: type, __init_slice: []const __Elem) []const __Elem { const __buf = std.heap.page_allocator.alloc(__Elem, __init_slice.len) catch unreachable; if (__init_slice.len > 0) @memcpy(__buf, __init_slice); return __buf; }");
            self.writeln("fn CpArrayElemType(comptime __PtrT: type) type { const __S0 = @typeInfo(__PtrT).pointer.child; const __S1 = if (@typeInfo(__S0) == .pointer and @typeInfo(__S0).pointer.size == .one) @typeInfo(__S0).pointer.child else __S0; return @typeInfo(__S1).pointer.child; }");
            self.writeln("fn cp_array_push(__arr_ptr: anytype, __item: anytype) void { const __S0 = @TypeOf(__arr_ptr.*); const __target_ptr = if (comptime (@typeInfo(__S0) == .pointer and @typeInfo(__S0).pointer.size == .one)) __arr_ptr.* else __arr_ptr; const __SliceT = @TypeOf(__target_ptr.*); const __Elem = @typeInfo(__SliceT).pointer.child; const __old_slice = __target_ptr.*; const __new_slice = std.heap.page_allocator.alloc(__Elem, __old_slice.len + 1) catch unreachable; if (__old_slice.len > 0) @memcpy(__new_slice[0..__old_slice.len], __old_slice); __new_slice[__old_slice.len] = @as(__Elem, cp_val(__item)); __target_ptr.* = __new_slice; }");
            self.writeln("fn cp_array_pop(__arr_ptr: anytype) ?CpArrayElemType(@TypeOf(__arr_ptr)) { const __S0 = @TypeOf(__arr_ptr.*); const __target_ptr = if (comptime (@typeInfo(__S0) == .pointer and @typeInfo(__S0).pointer.size == .one)) __arr_ptr.* else __arr_ptr; const __s = __target_ptr.*; if (__s.len == 0) return null; const __last = __s[__s.len - 1]; __target_ptr.* = __s[0 .. __s.len - 1]; return __last; }");
            self.writeln("fn cp_container_clear(__cp_ptr: anytype) void { const __S0 = @TypeOf(__cp_ptr.*); if (comptime (@typeInfo(__S0) == .pointer and @typeInfo(__S0).pointer.size == .one and @typeInfo(@typeInfo(__S0).pointer.child) == .@\"struct\" and @hasDecl(@typeInfo(__S0).pointer.child, \"clear\"))) { __cp_ptr.*.clear(); } else { const __target_ptr = if (comptime (@typeInfo(__S0) == .pointer and @typeInfo(__S0).pointer.size == .one)) __cp_ptr.* else __cp_ptr; __target_ptr.* = __target_ptr.*[0..0]; } }");
        }

        if features.needs_iter_helpers {
            self.writeln("fn cp_iter_slice(__cp_val: anytype) switch (@typeInfo(@TypeOf(__cp_val))) { .pointer => |__cp_p| switch (__cp_p.size) { .slice => @TypeOf(__cp_val), .one => switch (@typeInfo(__cp_p.child)) { .array => []const @typeInfo(__cp_p.child).array.child, .pointer => |__cp_sub_p| if (__cp_sub_p.size == .slice) __cp_p.child else @TypeOf(__cp_val), else => @TypeOf(__cp_val), }, else => @TypeOf(__cp_val), }, .array => []const @typeInfo(@TypeOf(__cp_val)).array.child, else => @TypeOf(__cp_val), } { const __T = @TypeOf(__cp_val); switch (@typeInfo(__T)) { .pointer => |__cp_p| switch (__cp_p.size) { .slice => return __cp_val, .one => switch (@typeInfo(__cp_p.child)) { .array => return __cp_val[0..], .pointer => |__cp_sub_p| if (__cp_sub_p.size == .slice) return __cp_val.* else return __cp_val, else => return __cp_val, }, else => return __cp_val, }, .array => return __cp_val[0..], else => return __cp_val, } }");
            self.writeln("fn cp_mut_slice(__cp_ptr: anytype) switch (@typeInfo(@TypeOf(__cp_ptr.*))) { .array => []@typeInfo(@TypeOf(__cp_ptr.*)).array.child, .pointer => |__cp_p| switch (__cp_p.size) { .slice => []if (@typeInfo(__cp_p.child) == .pointer) @typeInfo(__cp_p.child).pointer.child else __cp_p.child, .one => switch (@typeInfo(__cp_p.child)) { .pointer => |__cp_sub_p| if (__cp_sub_p.size == .slice) []__cp_sub_p.child else @compileError(\"Cannot mutably iterate over non-collection\"), else => @compileError(\"Cannot mutably iterate over non-collection\"), }, else => @compileError(\"Cannot mutably iterate over non-collection\"), }, else => @compileError(\"Cannot mutably iterate over non-collection\"), } { const __ValT = @TypeOf(__cp_ptr.*); switch (@typeInfo(__ValT)) { .array => return __cp_ptr[0..], .pointer => |__cp_p| switch (__cp_p.size) { .slice => return @constCast(__cp_ptr.*), .one => return @constCast(__cp_ptr.*.*), else => unreachable, }, else => unreachable, } }");
        }
        if features.needs_index_helpers {
            self.writeln("fn cp_index_of(__container: anytype, __needle: anytype) i32 { const __cp_slice = cp_iter_slice(__container); const __SliceT = @TypeOf(__cp_slice); if (comptime is_string_like(__SliceT) and is_string_like(@TypeOf(__needle))) { if (std.mem.indexOf(u8, to_string_slice(__cp_slice), to_string_slice(__needle))) |__idx| return @intCast(__idx); return -1; } for (__cp_slice, 0..) |__item, __idx| { if (cp_eq(__item, __needle)) return @intCast(__idx); } return -1; }");
            self.writeln("fn cp_last_index_of(__container: anytype, __needle: anytype) i32 { const __cp_slice = cp_iter_slice(__container); const __SliceT = @TypeOf(__cp_slice); if (comptime is_string_like(__SliceT) and is_string_like(@TypeOf(__needle))) { if (std.mem.lastIndexOf(u8, to_string_slice(__cp_slice), to_string_slice(__needle))) |__idx| return @intCast(__idx); return -1; } var __idx: usize = __cp_slice.len; while (__idx > 0) { __idx -= 1; if (cp_eq(__cp_slice[__idx], __needle)) return @intCast(__idx); } return -1; }");
            self.writeln("fn cp_includes(__container: anytype, __needle: anytype) bool { return cp_index_of(__container, __needle) != -1; }");
        }
        if features.needs_actors {
            self.writeln("");
            self.writeln("pub const BaseContext = struct { pc: u32 = 0, _self: PID = undefined, _parent: ?*anyopaque = null, _parent_fn: ?*const fn(*anyopaque) anyerror!void = null, arena: ?*std.heap.ArenaAllocator = null, received_msg: ?Message = null, received_sender: ?PID = null, await_result: ?Message = null, };");

            self.writeln("");
            self.writeln("pub const ZigSharedTensor = struct {");
            self.writeln("    ref_count: std.atomic.Value(u32),");
            self.writeln("    data: []u8,");
            self.writeln("    allocator: std.mem.Allocator,");
            self.writeln(
                "    pub fn create(allocator: std.mem.Allocator, size: usize) !*ZigSharedTensor {",
            );
            self.writeln("        const ptr = try allocator.create(ZigSharedTensor);");
            self.writeln("        ptr.data = try allocator.alloc(u8, size);");
            self.writeln("        ptr.ref_count = std.atomic.Value(u32).init(1);");
            self.writeln("        ptr.allocator = allocator;");
            self.writeln("        return ptr;");
            self.writeln("    }");
            self.writeln("    pub fn retain(self: *ZigSharedTensor) void {");
            self.writeln("        _ = self.ref_count.fetchAdd(1, .monotonic);");
            self.writeln("    }");
            self.writeln("    pub fn release(self: *ZigSharedTensor) void {");
            self.writeln("        if (self.ref_count.fetchSub(1, .release) == 1) {");
            self.writeln("            _ = self.ref_count.load(.acquire);");
            self.writeln("            self.allocator.free(self.data);");
            self.writeln("            self.allocator.destroy(self);");
            self.writeln("        }");
            self.writeln("    }");
            self.writeln("};");
            self.writeln("");
            self.writeln("// --- Native Reactor Registry ---");
            self.writeln("threadlocal var poll_fds: [1024]std.posix.pollfd = undefined;");
            self.writeln("threadlocal var fd_to_ctx: [1024]*anyopaque = undefined;");
            self.writeln(
                "threadlocal var fd_to_run: [1024]*const fn(*anyopaque) anyerror!void = undefined;",
            );
            self.writeln("threadlocal var active_fds: usize = 0;");
            self.writeln("fn register_fd(fd: i32, run: *const fn(*anyopaque) anyerror!void, ctx: *anyopaque, events: i16) void {");
            self.writeln("    if (active_fds < 1024) {");
            self.writeln(
                "        poll_fds[active_fds] = .{ .fd = fd, .events = events, .revents = 0 };",
            );
            self.writeln("        fd_to_ctx[active_fds] = ctx;");
            self.writeln("        fd_to_run[active_fds] = run;");
            self.writeln("        active_fds += 1;");
            self.writeln("    }");
            self.writeln("}");
        }
        if features.needs_shift {
            self.writeln("fn cp_shl(val: anytype, shift: anytype) @TypeOf(val) {");
            self.writeln("    const __T = @TypeOf(val);");
            self.writeln("    if (comptime @typeInfo(__T) == .comptime_int) return val << shift;");
            self.writeln("    if (comptime @typeInfo(__T) == .int) {");
            self.writeln("        const ShiftType = std.math.Log2Int(__T);");
            self.writeln("        const u_shift: usize = if (@typeInfo(@TypeOf(shift)) == .int and @typeInfo(@TypeOf(shift)).int.signedness == .signed) @intCast(@max(0, shift)) else @intCast(shift);");
            self.writeln("        return val << @as(ShiftType, @truncate(u_shift));");
            self.writeln("    }");
            self.writeln("    return val << shift;");
            self.writeln("}");
            self.writeln("fn cp_shr(val: anytype, shift: anytype) @TypeOf(val) {");
            self.writeln("    const __T = @TypeOf(val);");
            self.writeln("    if (comptime @typeInfo(__T) == .comptime_int) return val >> shift;");
            self.writeln("    if (comptime @typeInfo(__T) == .int) {");
            self.writeln("        const ShiftType = std.math.Log2Int(__T);");
            self.writeln("        const u_shift: usize = if (@typeInfo(@TypeOf(shift)) == .int and @typeInfo(@TypeOf(shift)).int.signedness == .signed) @intCast(@max(0, shift)) else @intCast(shift);");
            self.writeln("        return val >> @as(ShiftType, @truncate(u_shift));");
            self.writeln("    }");
            self.writeln("    return val >> shift;");
            self.writeln("}");
        }
        if features.needs_actors {
            self.writeln("");
            self.writeln("// --- Scheduler ---");
            self.writeln(
                "const _Task = struct { run: *const fn(*anyopaque) anyerror!void, ctx: *anyopaque };",
            );
            self.writeln("const Spinlock = struct { state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0), pub fn acquire(self: *Spinlock) void { while (self.state.cmpxchgWeak(0, 1, .acquire, .monotonic) != null) { std.atomic.spinLoopHint(); } } pub fn release(self: *Spinlock) void { self.state.store(0, .release); } };");
            self.writeln("const GlobalTaskQueue = struct { lock: Spinlock = .{}, buffer: [2048]_Task = undefined, head: usize = 0, tail: usize = 0, count: usize = 0, pub fn push(self: *GlobalTaskQueue, item: _Task) void { self.lock.acquire(); defer self.lock.release(); if (self.count >= 2048) return; self.buffer[self.tail] = item; self.tail = (self.tail + 1) % 2048; self.count += 1; } pub fn pop(self: *GlobalTaskQueue) ?_Task { self.lock.acquire(); defer self.lock.release(); if (self.count == 0) return null; const item = self.buffer[self.head]; self.head = (self.head + 1) % 2048; self.count -= 1; return item; } pub fn len(self: *GlobalTaskQueue) usize { self.lock.acquire(); defer self.lock.release(); return self.count; } };");
            self.writeln("var global_task_queue: GlobalTaskQueue = .{};");
            self.writeln("var global_runtime_shutdown: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);");
            self.writeln(
                "var global_active_tasks: std.atomic.Value(usize) = std.atomic.Value(usize).init(0);",
            );
            self.writeln("var global_running_workers: std.atomic.Value(usize) = std.atomic.Value(usize).init(0);");
            self.writeln("threadlocal var task_queue: [1000]_Task = undefined; threadlocal var queue_len: usize = 0;");
            self.writeln(
                "threadlocal var current_node_id: u32 = 0; threadlocal var next_local_id: u32 = 1;",
            );
            self.writeln("fn schedule(run: *const fn(*anyopaque) anyerror!void, ctx: *anyopaque) void { if (current_node_id == 0) { global_task_queue.push(.{ .run = run, .ctx = ctx }); } else { if (queue_len < 1000) { task_queue[queue_len] = .{ .run = run, .ctx = ctx }; queue_len += 1; } } }");
            self.writeln("const _WorkerPoolRunner = struct { pub fn run() void { while (!global_runtime_shutdown.load(.acquire)) { if (global_task_queue.pop()) |task| { _ = global_running_workers.fetchAdd(1, .acquire); task.run(task.ctx) catch {}; _ = global_running_workers.fetchSub(1, .release); } else { const ts = std.os.linux.timespec{ .sec = 0, .nsec = 100_000 }; _ = std.os.linux.nanosleep(&ts, null); } } } };");
            self.writeln("");
            self.writeln("// --- Cross-Thread Channels & Mailboxes ---");
            self.writeln(
                "const CrossThreadMessage = struct { target: PID, sender: PID, msg: Message };",
            );
            self.writeln("const MailboxItem = struct { sender: PID, msg: Message };");
            self.writeln("const MailboxRingBuffer = struct { lock: Spinlock = .{}, buffer: [256]CrossThreadMessage = undefined, head: usize = 0, tail: usize = 0, count: usize = 0, pub fn push(self: *MailboxRingBuffer, item: CrossThreadMessage) bool { self.lock.acquire(); defer self.lock.release(); if (self.count >= 256) return false; self.buffer[self.tail] = item; self.tail = (self.tail + 1) % 256; self.count += 1; return true; } pub fn pop(self: *MailboxRingBuffer) ?CrossThreadMessage { self.lock.acquire(); defer self.lock.release(); if (self.count == 0) return null; const item = self.buffer[self.head]; self.head = (self.head + 1) % 256; self.count -= 1; return item; } };");
            self.writeln("var global_worker_mailboxes: [128]MailboxRingBuffer = @splat(.{});");
            self.writeln(
                "var global_next_worker_id: std.atomic.Value(u32) = std.atomic.Value(u32).init(1);",
            );
            self.writeln("const LocalMailbox = struct { items: [16]MailboxItem = undefined, head: usize = 0, tail: usize = 0, count: usize = 0, pub fn push(self: *LocalMailbox, item: MailboxItem) bool { if (self.count >= 16) return false; self.items[self.tail] = item; self.tail = (self.tail + 1) % 16; self.count += 1; return true; } pub fn pop(self: *LocalMailbox) ?MailboxItem { if (self.count == 0) return null; const item = self.items[self.head]; self.head = (self.head + 1) % 16; self.count -= 1; return item; } };");
            self.writeln("threadlocal var local_mailboxes: [1024]LocalMailbox = @splat(.{});");
            self.writeln("threadlocal var waiting_actors: [1024]?*anyopaque = @splat(null);");
            self.writeln("threadlocal var waiting_actor_fns: [1024]?*const fn(*anyopaque) anyerror!void = @splat(null);");
            self.writeln("threadlocal var waiting_count: usize = 0;");
            self.writeln("fn deliver_local(target_local_id: u32, sender_pid: PID, msg: Message) void { const slot = target_local_id % 1024; _ = local_mailboxes[slot].push(.{ .sender = sender_pid, .msg = msg }); if (waiting_actors[slot]) |w_ctx| { const w_fn = waiting_actor_fns[slot].?; waiting_actors[slot] = null; waiting_actor_fns[slot] = null; if (waiting_count > 0) waiting_count -= 1; schedule(w_fn, w_ctx); } }");
            self.writeln("fn drain_cross_thread_mailbox() void { const slot = current_node_id % 128; while (global_worker_mailboxes[slot].pop()) |item| { deliver_local(item.target.local_id, item.sender, item.msg); } }");
            self.writeln("fn cp_send(sender_pid: PID, target: PID, msg: Message) bool { if (target.node_id == current_node_id) { deliver_local(target.local_id, sender_pid, msg); return true; } else { const slot = target.node_id % 128; return global_worker_mailboxes[slot].push(.{ .target = target, .sender = sender_pid, .msg = msg }); } }");
            self.writeln("");
            self.writeln("pub fn print(v: anytype) void { const __T = @TypeOf(v); if (comptime is_string_like(__T)) { std.debug.print(\"{s}\\n\", .{ to_string_slice(v) }); } else if (comptime __T == Message) { std.debug.print(\"{s}\\n\", .{ cp_fmt(v) }); } else if (comptime __T == ?Message) { if (v) |m| { std.debug.print(\"{s}\\n\", .{ cp_fmt(m) }); } else { std.debug.print(\"null\\n\", .{}); } } else { std.debug.print(\"{any}\\n\", .{ v }); } }");
            self.writeln("pub const CpTask = struct { base: BaseContext = .{}, lock: Spinlock = .{}, state: u8 = 0, waiter_ctx: ?*anyopaque = null, waiter_fn: ?*const fn(*anyopaque) anyerror!void = null, join_barrier: ?*JoinBarrier = null, join_idx: usize = 0, arena: ?*std.heap.ArenaAllocator = null, };");
            if !self.enum_defs.contains_key("Task") {
                self.writeln("pub const Task = CpTask;");
            }
            self.writeln("pub const JoinBarrier = struct { lock: Spinlock = .{}, remaining: usize, total: usize, results: []Message, waiter_ctx: ?*anyopaque = null, waiter_fn: ?*const fn(*anyopaque) anyerror!void = null, pub fn init(allocator: std.mem.Allocator, total: usize) !*JoinBarrier { const b = try allocator.create(JoinBarrier); b.* = .{ .remaining = total + 1, .total = total, .results = try allocator.alloc(Message, total), .waiter_ctx = null, .waiter_fn = null }; return b; } pub fn on_task_done(self: *JoinBarrier, idx: usize, msg: Message) void { self.lock.acquire(); self.results[idx] = msg; self.remaining -= 1; const done = (self.remaining == 0); const w_ctx_opt = self.waiter_ctx; const w_fn_opt = self.waiter_fn; if (done and w_ctx_opt != null) { self.waiter_ctx = null; self.waiter_fn = null; } self.lock.release(); if (done) { if (w_ctx_opt) |w_ctx| { const w_fn = w_fn_opt.?; const base_waiter: *BaseContext = @ptrCast(@alignCast(w_ctx)); const arr_res = std.heap.page_allocator.alloc(Message, self.total) catch unreachable; @memcpy(arr_res, self.results); base_waiter.await_result = Message{ .array = arr_res }; schedule(w_fn, w_ctx); } } } pub fn finish_setup_and_should_suspend(self: *JoinBarrier) bool { self.lock.acquire(); self.remaining -= 1; const done = (self.remaining == 0); if (done) { const base_waiter: *BaseContext = @ptrCast(@alignCast(self.waiter_ctx.?)); const arr_res = std.heap.page_allocator.alloc(Message, self.total) catch unreachable; @memcpy(arr_res, self.results); base_waiter.await_result = Message{ .array = arr_res }; self.waiter_ctx = null; self.waiter_fn = null; } self.lock.release(); return !done; } };");
            self.writeln("pub fn __cp_task_bridge(p: *anyopaque) anyerror!void { const t: *CpTask = @ptrCast(@alignCast(p)); t.lock.acquire(); t.state = 2; const jb = t.join_barrier; const j_idx = t.join_idx; const w_ctx_opt = t.waiter_ctx; const w_fn_opt = t.waiter_fn; t.waiter_ctx = null; t.waiter_fn = null; const res = t.base.await_result; t.lock.release(); if (jb) |b| { b.on_task_done(j_idx, res orelse Message{ .string = \"\" }); } if (w_ctx_opt) |w_ctx| { const w_fn = w_fn_opt.?; const base_waiter: *BaseContext = @ptrCast(@alignCast(w_ctx)); base_waiter.await_result = res; schedule(w_fn, w_ctx); } _ = global_active_tasks.fetchSub(1, .release); }");
            self.writeln("pub fn cp_msg_to_val(m: Message) []const u8 { return cp_fmt(m); }");
        }
        if features.needs_unpack {
            if !features.needs_collections && !features.needs_actors {
                self.writeln("const Message = anyopaque;");
                self.writeln("fn cp_msg_to_val(_: anytype) []const u8 { return \"\"; }");
            }
            self.writeln("pub fn CpUnpackElemType(comptime __T: type, comptime idx: usize) type { if (__T == ?Message or __T == Message) return []const u8; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice) return @typeInfo(__T).pointer.child; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice) return @typeInfo(@typeInfo(__T).pointer.child).pointer.child; if (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array) return @typeInfo(@typeInfo(__T).pointer.child).array.child; if (@typeInfo(__T) == .array) return @typeInfo(__T).array.child; const ti = @typeInfo(__T); if (ti == .@\"struct\") return ti.@\"struct\".field_types[idx]; return @FieldType(__T, std.fmt.comptimePrint(\"{d}\", .{idx})); }");
            self.writeln("pub fn cp_unpack_elem(v: anytype, comptime idx: usize) CpUnpackElemType(@TypeOf(v), idx) { const __T = @TypeOf(v); if (comptime __T == ?Message) { if (v) |m| { if (m == .array and idx < m.array.len) { return cp_msg_to_val(m.array[idx]); } } return \"\"; } else if (comptime __T == Message) { if (v == .array and idx < v.array.len) { return cp_msg_to_val(v.array[idx]); } return \"\"; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice)) { return v[idx]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice)) { return v.*[idx]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array)) { return v[idx]; } else if (comptime (@typeInfo(__T) == .array)) { return v[idx]; } else { const ti = @typeInfo(__T); if (ti == .@\"struct\") return @field(v, ti.@\"struct\".field_names[idx]); return @field(v, std.fmt.comptimePrint(\"{d}\", .{idx})); } }");
            self.writeln("pub fn CpUnpackTailType(comptime __T: type, comptime offset_from_end: usize) type { if (__T == ?Message or __T == Message) return []const u8; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice) return @typeInfo(__T).pointer.child; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice) return @typeInfo(@typeInfo(__T).pointer.child).pointer.child; if (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array) return @typeInfo(@typeInfo(__T).pointer.child).array.child; if (@typeInfo(__T) == .array) return @typeInfo(__T).array.child; const ti = @typeInfo(__T); if (ti == .@\"struct\") { const n = ti.@\"struct\".field_names.len; return ti.@\"struct\".field_types[n - offset_from_end]; } return @FieldType(__T, std.fmt.comptimePrint(\"{d}\", .{1})); }");
            self.writeln("pub fn cp_unpack_tail(v: anytype, comptime offset_from_end: usize) CpUnpackTailType(@TypeOf(v), offset_from_end) { const __T = @TypeOf(v); if (comptime __T == ?Message) { if (v) |m| { if (m == .array and m.array.len >= offset_from_end) { return cp_msg_to_val(m.array[m.array.len - offset_from_end]); } } return \"\"; } else if (comptime __T == Message) { if (v == .array and v.array.len >= offset_from_end) { return cp_msg_to_val(v.array[v.array.len - offset_from_end]); } return \"\"; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice)) { return v[v.len - offset_from_end]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice)) { return v.*[v.*.len - offset_from_end]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array)) { return v[v.len - offset_from_end]; } else if (comptime (@typeInfo(__T) == .array)) { return v[v.len - offset_from_end]; } else { const ti = @typeInfo(__T); if (ti == .@\"struct\") { const n = ti.@\"struct\".field_names.len; return @field(v, ti.@\"struct\".field_names[n - offset_from_end]); } return @field(v, std.fmt.comptimePrint(\"{d}\", .{0})); } }");
            self.writeln("pub fn CpUnpackSliceType(comptime __T: type, comptime start_idx: usize) type { if (__T == ?Message or __T == Message) return []const Message; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice) return __T; if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice) return @typeInfo(__T).pointer.child; if (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array) return []const @typeInfo(@typeInfo(__T).pointer.child).array.child; if (@typeInfo(__T) == .array) return []const @typeInfo(__T).array.child; const ti = @typeInfo(__T); if (ti == .@\"struct\") { const types = ti.@\"struct\".field_types; if (types.len == 0) return []const i32; const Elem = if (start_idx < types.len) types[start_idx] else types[0]; return []const CpValType(Elem); } return []const i32; }");
            self.writeln("pub fn cp_unpack_slice(v: anytype, comptime start_idx: usize, comptime suffix_len: usize) CpUnpackSliceType(@TypeOf(v), start_idx) { const __T = @TypeOf(v); if (comptime __T == ?Message) { if (v) |m| { if (m == .array and m.array.len >= start_idx + suffix_len) { return m.array[start_idx .. m.array.len - suffix_len]; } } return &[_]Message{}; } else if (comptime __T == Message) { if (v == .array and v.array.len >= start_idx + suffix_len) { return v.array[start_idx .. v.array.len - suffix_len]; } return &[_]Message{}; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice)) { return v[start_idx .. v.len - suffix_len]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and @typeInfo(@typeInfo(__T).pointer.child) == .pointer and @typeInfo(@typeInfo(__T).pointer.child).pointer.size == .slice)) { return v.*[start_idx .. v.*.len - suffix_len]; } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .array)) { return v[start_idx .. v.len - suffix_len]; } else if (comptime (@typeInfo(__T) == .array)) { return v[start_idx .. v.len - suffix_len]; } else { const ti = @typeInfo(__T); if (ti == .@\"struct\") { const names = ti.@\"struct\".field_names; const types = ti.@\"struct\".field_types; const end = names.len - suffix_len; const count = if (end > start_idx) end - start_idx else 0; const Elem = if (start_idx < types.len) CpValType(types[start_idx]) else i32; const buf = std.heap.page_allocator.alloc(Elem, count) catch unreachable; inline for (start_idx..end, 0..) |f_idx, out_idx| { buf[out_idx] = cp_val(@field(v, names[f_idx])); } return buf; } return &[_]i32{}; } }");
        }
        if features.needs_string_helpers || features.needs_fmt {
            self.writeln("pub fn cp_string_from(val: anytype) []const u8 { const __T = @TypeOf(val); if (comptime __T == []const u8) { return val; } else if (comptime (__T == [*:0]const u8 or __T == [*:0]u8)) { return std.mem.span(val); } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .slice and @typeInfo(__T).pointer.child == u8)) { return val; } else if (comptime (@typeInfo(__T) == .@\"enum\")) { return @tagName(val); } else if (comptime __T == bool) { return if (val) \"true\" else \"false\"; } else if (comptime (@typeInfo(__T) == .int or __T == comptime_int)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{}\", .{val}) catch \"0\"; } else if (comptime (@typeInfo(__T) == .float or __T == comptime_float)) { return std.fmt.allocPrint(std.heap.page_allocator, \"{d}\", .{val}) catch \"0.0\"; } else if (comptime (@typeInfo(__T) == .@\"struct\" and @hasDecl(__T, \"toString\"))) { return val.toString(); } else if (comptime (@typeInfo(__T) == .pointer and @typeInfo(@typeInfo(__T).pointer.child) == .@\"struct\" and @hasDecl(@typeInfo(__T).pointer.child, \"toString\"))) { return val.toString(); } else { return @typeName(__T); } }");
            self.writeln("");
        }

    }

    pub(crate) fn emit_regex_prelude(&mut self) {
        self.writeln("pub const regex_t = extern struct { __space: [64]usize = undefined, };");
        self.writeln("pub const regmatch_t = extern struct { rm_so: i32, rm_eo: i32, };");
        self.writeln(
            "extern \"c\" fn regcomp(preg: *regex_t, regex: [*:0]const u8, cflags: i32) i32;",
        );
        self.writeln("extern \"c\" fn regexec(preg: *const regex_t, string: [*:0]const u8, nmatch: usize, pmatch: [*]regmatch_t, eflags: i32) i32;");
        self.writeln("extern \"c\" fn regfree(preg: *regex_t) void;");
        self.writeln("pub const RegExp = struct {");
        self.writeln("    preg: regex_t, source: []const u8, lit_prefix: []const u8 = \"\", is_pure_literal: bool = false,");
        self.writeln("    pub fn _create(allocator: std.mem.Allocator) !*RegExp { const p = try allocator.create(RegExp); p.* = undefined; p.lit_prefix = \"\"; p.is_pure_literal = false; return p; }");
        self.writeln("    fn simdIndexOfByte(haystack: []const u8, needle: u8) ?usize {");
        self.writeln("        var i: usize = 0;");
        self.writeln("        const v_needle: @Vector(16, u8) = @splat(needle);");
        self.writeln("        while (i + 16 <= haystack.len) : (i += 16) {");
        self.writeln("            const chunk: @Vector(16, u8) = haystack[i..][0..16].*;");
        self.writeln("            const mask: u16 = @bitCast(chunk == v_needle);");
        self.writeln("            if (mask != 0) return i + @ctz(mask);");
        self.writeln("        }");
        self.writeln(
            "        while (i < haystack.len) : (i += 1) { if (haystack[i] == needle) return i; }",
        );
        self.writeln("        return null;");
        self.writeln("    }");
        self.writeln("    fn simdContainsPrefix(haystack: []const u8, needle: []const u8) bool {");
        self.writeln("        if (needle.len == 0) return true;");
        self.writeln("        if (haystack.len < needle.len) return false;");
        self.writeln("        const first = needle[0];");
        self.writeln("        var pos: usize = 0;");
        self.writeln("        while (pos + needle.len <= haystack.len) {");
        self.writeln(
            "            const rel = simdIndexOfByte(haystack[pos..], first) orelse return false;",
        );
        self.writeln("            const idx = pos + rel;");
        self.writeln("            if (idx + needle.len > haystack.len) return false;");
        self.writeln("            if (std.mem.eql(u8, haystack[idx .. idx + needle.len], needle)) return true;");
        self.writeln("            pos = idx + 1;");
        self.writeln("        }");
        self.writeln("        return false;");
        self.writeln("    }");
        self.writeln("    fn extractLitPrefix(pattern: []const u8, flags: []const u8, is_pure: *bool) []const u8 {");
        self.writeln("        is_pure.* = false;");
        self.writeln("        for (flags) |fc| { if (fc == 'i' or fc == 'm') return \"\"; }");
        self.writeln("        for (pattern) |c| { if (c == '|') return \"\"; }");
        self.writeln("        var end: usize = 0;");
        self.writeln("        while (end < pattern.len) : (end += 1) {");
        self.writeln("            const c = pattern[end];");
        self.writeln("            if (c == '.' or c == '^' or c == '$' or c == '*' or c == '+' or c == '?' or c == '(' or c == ')' or c == '[' or c == ']' or c == '{' or c == '}' or c == '\\\\') {");
        self.writeln("                if ((c == '*' or c == '?') and end > 0) end -= 1;");
        self.writeln("                return pattern[0..end];");
        self.writeln("            }");
        self.writeln("        }");
        self.writeln("        is_pure.* = (pattern.len > 0);");
        self.writeln("        return pattern;");
        self.writeln("    }");
        self.writeln(
            "    pub fn init(self: *RegExp, pattern: []const u8, flags: []const u8) void {",
        );
        self.writeln("        self.source = pattern;");
        self.writeln("        var pure: bool = false;");
        self.writeln("        self.lit_prefix = extractLitPrefix(pattern, flags, &pure);");
        self.writeln("        self.is_pure_literal = pure;");
        self.writeln("        var cflags: i32 = 1; // REG_EXTENDED");
        self.writeln("        for (flags) |fc| { if (fc == 'i') { cflags |= 2; } else if (fc == 'm') { cflags |= (if (comptime @import(\"builtin\").os.tag == .linux) @as(i32, 4) else @as(i32, 8)); } }");
        self.writeln("        var stack_buf: [2048]u8 = undefined;");
        self.writeln("        if (pattern.len < 2048) {");
        self.writeln(
            "            @memcpy(stack_buf[0..pattern.len], pattern); stack_buf[pattern.len] = 0;",
        );
        self.writeln("            if (regcomp(&self.preg, @ptrCast(&stack_buf), cflags) != 0) { @panic(\"Invalid RegExp\"); }");
        self.writeln("        } else {");
        self.writeln("            const c_pattern = std.heap.smp_allocator.allocSentinel(u8, pattern.len, 0) catch unreachable; defer std.heap.smp_allocator.free(c_pattern);");
        self.writeln("            @memcpy(c_pattern[0..pattern.len], pattern);");
        self.writeln("            if (regcomp(&self.preg, c_pattern.ptr, cflags) != 0) { @panic(\"Invalid RegExp\"); }");
        self.writeln("        }");
        self.writeln("    }");
        self.writeln("    pub fn @\"test\"(self: *RegExp, str: []const u8) bool {");
        self.writeln("        if (self.lit_prefix.len > 0) { if (!simdContainsPrefix(str, self.lit_prefix)) return false; if (self.is_pure_literal) return true; }");
        self.writeln("        var pmatch: [1]regmatch_t = undefined;");
        self.writeln("        var stack_buf: [2048]u8 = undefined;");
        self.writeln("        if (str.len < 2048) {");
        self.writeln("            @memcpy(stack_buf[0..str.len], str); stack_buf[str.len] = 0;");
        self.writeln(
            "            return regexec(&self.preg, @ptrCast(&stack_buf), 1, &pmatch, 0) == 0;",
        );
        self.writeln("        }");
        self.writeln("        const c_str = std.heap.smp_allocator.allocSentinel(u8, str.len, 0) catch unreachable; defer std.heap.smp_allocator.free(c_str);");
        self.writeln("        @memcpy(c_str[0..str.len], str);");
        self.writeln("        return regexec(&self.preg, c_str.ptr, 1, &pmatch, 0) == 0;");
        self.writeln("    }");
        self.writeln("    pub fn exec(self: *RegExp, str: []const u8) []const u8 {");
        self.writeln("        if (self.lit_prefix.len > 0) { if (!simdContainsPrefix(str, self.lit_prefix)) return \"\"; if (self.is_pure_literal) return self.lit_prefix; }");
        self.writeln("        var pmatch: [1]regmatch_t = undefined;");
        self.writeln("        var stack_buf: [2048]u8 = undefined;");
        self.writeln("        if (str.len < 2048) {");
        self.writeln("            @memcpy(stack_buf[0..str.len], str); stack_buf[str.len] = 0;");
        self.writeln("            if (regexec(&self.preg, @ptrCast(&stack_buf), 1, &pmatch, 0) == 0) { const start: usize = @intCast(pmatch[0].rm_so); const end: usize = @intCast(pmatch[0].rm_eo); return str[start..end]; }");
        self.writeln("            return \"\";");
        self.writeln("        }");
        self.writeln("        const c_str = std.heap.smp_allocator.allocSentinel(u8, str.len, 0) catch unreachable; defer std.heap.smp_allocator.free(c_str);");
        self.writeln("        @memcpy(c_str[0..str.len], str);");
        self.writeln("        if (regexec(&self.preg, c_str.ptr, 1, &pmatch, 0) == 0) { const start: usize = @intCast(pmatch[0].rm_so); const end: usize = @intCast(pmatch[0].rm_eo); return str[start..end]; }");
        self.writeln("        return \"\";");
        self.writeln("    }");
        self.writeln("    pub fn release(self: *RegExp) void { regfree(&self.preg); }");
        self.writeln("};");
    }
}
