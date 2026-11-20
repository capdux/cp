// Source: cp/lib/cel/types.cp (CEL Value System, Environment & Errors)
const std = @import("std");

pub const CelError = error{
    SyntaxError,
    MaxDepthExceeded,
    CostLimitExceeded,
    TypeMismatch,
    DivisionByZero,
    IntegerOverflow,
    IndexOutOfBounds,
    KeyNotFound,
    UndeclaredIdentifier,
    UnknownFunction,
    InvalidArgument,
    OutOfMemory,
};

pub struct CelMapEntry {
    key: []const u8,
    value: CelValue,
}

pub const CelValue = union(enum) {
    null_val: void,
    bool_val: bool,
    int_val: i64,
    uint_val: u64,
    double_val: f64,
    string_val: []const u8,
    list_val: []CelValue,
    map_val: []CelMapEntry,

    pub fn typeName(self: *const CelValue): []const u8 {
        return switch (self.*) {
            .null_val => "null_type",
            .bool_val => "bool",
            .int_val => "int",
            .uint_val => "uint",
            .double_val => "double",
            .string_val => "string",
            .list_val => "list",
            .map_val => "map",
        };
    }

    pub fn deinit(*self: CelValue, alloc: std.mem.Allocator) {
        switch (self.*) {
            .null_val, .bool_val, .int_val, .uint_val, .double_val => {},
            .string_val => |s| {
                alloc.free(s);
            },
            .list_val => |items| {
                let i: usize = 0;
                while (i < items.len) {
                    items[i].deinit(alloc);
                    i += 1;
                }
                alloc.free(items);
            },
            .map_val => |entries| {
                let i: usize = 0;
                while (i < entries.len) {
                    alloc.free(entries[i].key);
                    entries[i].value.deinit(alloc);
                    i += 1;
                }
                alloc.free(entries);
            },
        }
        self.* = .null_val;
    }

    pub fn clone(self: *const CelValue, alloc: std.mem.Allocator): CelError!CelValue {
        return switch (self.*) {
            .null_val => .null_val,
            .bool_val => |b| CelValue{ .bool_val = b },
            .int_val => |i| CelValue{ .int_val = i },
            .uint_val => |u| CelValue{ .uint_val = u },
            .double_val => |d| CelValue{ .double_val = d },
            .string_val => |s| blk: {
                const dup = alloc.dupe(u8, s) catch return error.OutOfMemory;
                break :blk CelValue{ .string_val = dup };
            },
            .list_val => |items| blk: {
                var out = std.ArrayList(CelValue).empty;
                errdefer {
                    for (*it of out.items) {
                        it.deinit(alloc);
                    }
                    out.deinit(alloc);
                }
                for (it of items) {
                    const cloned_item = try it.clone(alloc);
                    out.append(alloc, cloned_item) catch return error.OutOfMemory;
                }
                const slice = out.toOwnedSlice(alloc) catch return error.OutOfMemory;
                break :blk CelValue{ .list_val = slice };
            },
            .map_val => |entries| blk: {
                var out = std.ArrayList(CelMapEntry).empty;
                errdefer {
                    for (*e of out.items) {
                        alloc.free(e.key);
                        e.value.deinit(alloc);
                    }
                    out.deinit(alloc);
                }
                for (e of entries) {
                    const kdup = alloc.dupe(u8, e.key) catch return error.OutOfMemory;
                    errdefer alloc.free(kdup);
                    const vdup = try e.value.clone(alloc);
                    out.append(alloc, CelMapEntry{ .key = kdup, .value = vdup }) catch return error.OutOfMemory;
                }
                const slice = out.toOwnedSlice(alloc) catch return error.OutOfMemory;
                break :blk CelValue{ .map_val = slice };
            },
        };
    }

    pub fn equals(self: *const CelValue, other: *const CelValue): bool {
        switch (self.*) {
            .null_val => return other.* == .null_val,
            .bool_val => |a| {
                return switch (other.*) {
                    .bool_val => |b| a == b,
                    else => false,
                };
            },
            .int_val => |a| {
                return switch (other.*) {
                    .int_val => |b| a == b,
                    .uint_val => |b| a >= 0 and @as(u64, @intCast(a)) == b,
                    .double_val => |b| @as(f64, @floatFromInt(a)) == b,
                    else => false,
                };
            },
            .uint_val => |a| {
                return switch (other.*) {
                    .uint_val => |b| a == b,
                    .int_val => |b| b >= 0 and a == @as(u64, @intCast(b)),
                    .double_val => |b| @as(f64, @floatFromInt(a)) == b,
                    else => false,
                };
            },
            .double_val => |a| {
                return switch (other.*) {
                    .double_val => |b| a == b,
                    .int_val => |b| a == @as(f64, @floatFromInt(b)),
                    .uint_val => |b| a == @as(f64, @floatFromInt(b)),
                    else => false,
                };
            },
            .string_val => |a| {
                return switch (other.*) {
                    .string_val => |b| std.mem.eql(u8, a, b),
                    else => false,
                };
            },
            .list_val => |a| {
                return switch (other.*) {
                    .list_val => |b| blk: {
                        if (a.len != b.len) {
                            break :blk false;
                        }
                        var i: usize = 0;
                        while (i < a.len) {
                            if (!a[i].equals(&b[i])) {
                                break :blk false;
                            }
                            i += 1;
                        }
                        break :blk true;
                    },
                    else => false,
                };
            },
            .map_val => |a| {
                return switch (other.*) {
                    .map_val => |b| blk: {
                        if (a.len != b.len) {
                            break :blk false;
                        }
                        for (ea of a) {
                            var found = false;
                            for (eb of b) {
                                if (std.mem.eql(u8, ea.key, eb.key)) {
                                    if (!ea.value.equals(&eb.value)) {
                                        break :blk false;
                                    }
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                break :blk false;
                            }
                        }
                        break :blk true;
                    },
                    else => false,
                };
            },
        }
    }

    pub fn mapGet(self: *const CelValue, key: []const u8): (*const CelValue)? {
        switch (self.*) {
            .map_val => |entries| {
                let i: usize = 0;
                while (i < entries.len) {
                    if (std.mem.eql(u8, entries[i].key, key)) {
                        return &entries[i].value;
                    }
                    i += 1;
                }
                return null;
            },
            else => return null,
        }
    }
};

pub fn celString(alloc: std.mem.Allocator, s: []const u8): CelError!CelValue {
    const dup = alloc.dupe(u8, s) catch return error.OutOfMemory;
    return CelValue{ .string_val = dup };
}

pub fn celInt(v: i64): CelValue {
    return CelValue{ .int_val = v };
}

pub fn celUint(v: u64): CelValue {
    return CelValue{ .uint_val = v };
}

pub fn celDouble(v: f64): CelValue {
    return CelValue{ .double_val = v };
}

pub fn celBool(v: bool): CelValue {
    return CelValue{ .bool_val = v };
}

pub fn celNull(): CelValue {
    return .null_val;
}

pub struct CelEnv {
    bindings: std.ArrayList(CelMapEntry),

    pub fn init(): CelEnv {
        return CelEnv{
            .bindings = std.ArrayList(CelMapEntry).empty,
        };
    }

    pub fn deinit(*_: *CelEnv, alloc: std.mem.Allocator)  {
        for (*entry of _.bindings.items) {
            alloc.free(entry.key);
            entry.value.deinit(alloc);
        }
        _.bindings.deinit(alloc);
    }

    pub fn bind(*_: *CelEnv, alloc: std.mem.Allocator, name: []const u8, val: CelValue): CelError!void {
        const cloned = try val.clone(alloc);
        errdefer {
            let tmp = cloned;
            tmp.deinit(alloc);
        }
        try _.bindOwned(alloc, name, cloned);
    }

    pub fn bindOwned(*_: *CelEnv, alloc: std.mem.Allocator, name: []const u8, val: CelValue): CelError!void {
        for (*entry of _.bindings.items) {
            if (std.mem.eql(u8, entry.key, name)) {
                entry.value.deinit(alloc);
                entry.value = val;
                return;
            }
        }
        const kdup = alloc.dupe(u8, name) catch return error.OutOfMemory;
        errdefer alloc.free(kdup);
        _.bindings.append(alloc, CelMapEntry{
            .key = kdup,
            .value = val,
        }) catch return error.OutOfMemory;
    }

    pub fn get(_: *const CelEnv, name: []const u8): (*const CelValue)? {
        let i: usize = 0;
        while (i < _.bindings.items.len) {
            if (std.mem.eql(u8, _.bindings.items[i].key, name)) {
                return &_.bindings.items[i].value;
            }
            i += 1;
        }
        return null;
    }
}
