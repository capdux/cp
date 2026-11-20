// Source: cp/lib/msgpack_rpc/codec.cp (Pure CP MessagePack-RPC Wire Codec & Stream Decoder)
const std = @import("std");

pub const MAX_RPC_DEPTH: usize = 32;
pub const DEFAULT_MAX_MSG_BYTES: usize = 16 * 1024 * 1024;

pub const RpcError = error{
    Incomplete,
    UnexpectedEof,
    InvalidFormat,
    InvalidMsgType,
    InvalidFrame,
    MaxDepthExceeded,
    MessageTooLarge,
    OutOfMemory,
};

pub const RpcMapEntry = struct {
    key: RpcValue,
    value: RpcValue,
};

pub const RpcValue = union(enum) {
    nil: void,
    bool_val: bool,
    int_val: i64,
    uint_val: u64,
    float_val: f64,
    str_val: []const u8,
    bin_val: []const u8,
    array_val: []RpcValue,
    map_val: []RpcMapEntry,

    pub fn deinit(self: *const RpcValue, alloc: std.mem.Allocator): void {
        switch (self.*) {
            .nil, .bool_val, .int_val, .uint_val, .float_val => {},
            .str_val, .bin_val => |s| alloc.free(s),
            .array_val => |arr| {
                for (item of arr) {
                    item.deinit(alloc);
                }
                alloc.free(arr);
            },
            .map_val => |entries| {
                for (e of entries) {
                    e.key.deinit(alloc);
                    e.value.deinit(alloc);
                }
                alloc.free(entries);
            },
        }
    }

    pub fn equals(self: RpcValue, other: RpcValue): bool {
        if (@intFromEnum(self) != @intFromEnum(other)) { return false; }
        switch (self) {
            .nil => return true,
            .bool_val => |v| return v == other.bool_val,
            .int_val => |v| return v == other.int_val,
            .uint_val => |v| return v == other.uint_val,
            .float_val => |v| return v == other.float_val,
            .str_val => |v| return std.mem.eql(u8, v, other.str_val),
            .bin_val => |v| return std.mem.eql(u8, v, other.bin_val),
            .array_val => |arr| {
                if (arr.len != other.array_val.len) { return false; }
                for (item, i of arr) {
                    if (!item.equals(other.array_val[i])) { return false; }
                }
                return true;
            },
            .map_val => |entries| {
                if (entries.len != other.map_val.len) { return false; }
                for (e, i of entries) {
                    if (!e.key.equals(other.map_val[i].key) or !e.value.equals(other.map_val[i].value)) {
                        return false;
                    }
                }
                return true;
            },
        }
    }
};

pub const RpcRequest = struct {
    msgid: u32,
    method: []const u8,
    params: []RpcValue,
};

pub const RpcResponse = struct {
    msgid: u32,
    err: RpcValue,
    result: RpcValue,
};

pub const RpcNotification = struct {
    method: []const u8,
    params: []RpcValue,
};

pub const RpcMessage = union(enum) {
    request: RpcRequest,
    response: RpcResponse,
    notification: RpcNotification,

    pub fn deinit(self: *const RpcMessage, alloc: std.mem.Allocator): void {
        switch (self.*) {
            .request => |r| {
                alloc.free(r.method);
                for (p of r.params) { p.deinit(alloc); }
                alloc.free(r.params);
            },
            .response => |r| {
                r.err.deinit(alloc);
                r.result.deinit(alloc);
            },
            .notification => |n| {
                alloc.free(n.method);
                for (p of n.params) { p.deinit(alloc); }
                alloc.free(n.params);
            },
        }
    }
};

fn appendInt(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, comptime T: type, v: T): !void {
    let tmp: [@sizeOf(T)]u8 = undefined;
    std.mem.writeInt(T, &tmp, v, .big);
    try buf.appendSlice(alloc, &tmp);
}

pub fn encodeRpcValue(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: RpcValue): RpcError!void {
    switch (val) {
        .nil => try buf.append(alloc, 0xC0),
        .bool_val => |b| try buf.append(alloc, if (b) 0xC3 else 0xC2),
        .uint_val => |u| {
            if (u <= 0x7F) {
                try buf.append(alloc, @intCast(u));
            } else if (u <= 0xFF) {
                try buf.append(alloc, 0xCC);
                try buf.append(alloc, @intCast(u));
            } else if (u <= 0xFFFF) {
                try buf.append(alloc, 0xCD);
                try appendInt(buf, alloc, u16, @intCast(u));
            } else if (u <= 0xFFFF_FFFF) {
                try buf.append(alloc, 0xCE);
                try appendInt(buf, alloc, u32, @intCast(u));
            } else {
                try buf.append(alloc, 0xCF);
                try appendInt(buf, alloc, u64, u);
            }
        },
        .int_val => |iv| {
            if (iv >= 0) {
                try encodeRpcValue(buf, alloc, RpcValue{ .uint_val = @intCast(iv) });
            } else if (iv >= -32) {
                const b: i8 = @intCast(iv);
                try buf.append(alloc, @bitCast(b));
            } else if (iv >= -128) {
                try buf.append(alloc, 0xD0);
                const b: i8 = @intCast(iv);
                try buf.append(alloc, @bitCast(b));
            } else if (iv >= -32768) {
                try buf.append(alloc, 0xD1);
                try appendInt(buf, alloc, i16, @intCast(iv));
            } else if (iv >= -2147483648) {
                try buf.append(alloc, 0xD2);
                try appendInt(buf, alloc, i32, @intCast(iv));
            } else {
                try buf.append(alloc, 0xD3);
                try appendInt(buf, alloc, i64, iv);
            }
        },
        .float_val => |fv| {
            try buf.append(alloc, 0xCB);
            try appendInt(buf, alloc, u64, @bitCast(fv));
        },
        .str_val => |s| {
            if (s.len <= 31) {
                try buf.append(alloc, @as(u8, 0xA0) | @as(u8, @intCast(s.len)));
            } else if (s.len <= 0xFF) {
                try buf.append(alloc, 0xD9);
                try buf.append(alloc, @intCast(s.len));
            } else if (s.len <= 0xFFFF) {
                try buf.append(alloc, 0xDA);
                try appendInt(buf, alloc, u16, @intCast(s.len));
            } else {
                try buf.append(alloc, 0xDB);
                try appendInt(buf, alloc, u32, @intCast(s.len));
            }
            try buf.appendSlice(alloc, s);
        },
        .bin_val => |b| {
            if (b.len <= 0xFF) {
                try buf.append(alloc, 0xC4);
                try buf.append(alloc, @intCast(b.len));
            } else if (b.len <= 0xFFFF) {
                try buf.append(alloc, 0xC5);
                try appendInt(buf, alloc, u16, @intCast(b.len));
            } else {
                try buf.append(alloc, 0xC6);
                try appendInt(buf, alloc, u32, @intCast(b.len));
            }
            try buf.appendSlice(alloc, b);
        },
        .array_val => |arr| {
            if (arr.len <= 15) {
                try buf.append(alloc, @as(u8, 0x90) | @as(u8, @intCast(arr.len)));
            } else if (arr.len <= 0xFFFF) {
                try buf.append(alloc, 0xDC);
                try appendInt(buf, alloc, u16, @intCast(arr.len));
            } else {
                try buf.append(alloc, 0xDD);
                try appendInt(buf, alloc, u32, @intCast(arr.len));
            }
            for (item of arr) {
                try encodeRpcValue(buf, alloc, item);
            }
        },
        .map_val => |entries| {
            if (entries.len <= 15) {
                try buf.append(alloc, @as(u8, 0x80) | @as(u8, @intCast(entries.len)));
            } else if (entries.len <= 0xFFFF) {
                try buf.append(alloc, 0xDE);
                try appendInt(buf, alloc, u16, @intCast(entries.len));
            } else {
                try buf.append(alloc, 0xDF);
                try appendInt(buf, alloc, u32, @intCast(entries.len));
            }
            for (e of entries) {
                try encodeRpcValue(buf, alloc, e.key);
                try encodeRpcValue(buf, alloc, e.value);
            }
        },
    }
}

pub fn encodeRpcMessage(alloc: std.mem.Allocator, msg: RpcMessage): RpcError![]u8 {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    switch (msg) {
        .request => |r| {
            try out.append(alloc, 0x94); // 4-element fixarray
            try encodeRpcValue(&out, alloc, RpcValue{ .uint_val = 0 });
            try encodeRpcValue(&out, alloc, RpcValue{ .uint_val = r.msgid });
            try encodeRpcValue(&out, alloc, RpcValue{ .str_val = r.method });
            try encodeRpcValue(&out, alloc, RpcValue{ .array_val = r.params });
        },
        .response => |r| {
            try out.append(alloc, 0x94); // 4-element fixarray
            try encodeRpcValue(&out, alloc, RpcValue{ .uint_val = 1 });
            try encodeRpcValue(&out, alloc, RpcValue{ .uint_val = r.msgid });
            try encodeRpcValue(&out, alloc, r.err);
            try encodeRpcValue(&out, alloc, r.result);
        },
        .notification => |n| {
            try out.append(alloc, 0x93); // 3-element fixarray
            try encodeRpcValue(&out, alloc, RpcValue{ .uint_val = 2 });
            try encodeRpcValue(&out, alloc, RpcValue{ .str_val = n.method });
            try encodeRpcValue(&out, alloc, RpcValue{ .array_val = n.params });
        },
    }
    return out.toOwnedSlice(alloc);
}

const Reader = struct {
    data: []const u8,
    pos: usize = 0,

    fn readByte(self: *Reader): RpcError!u8 {
        if (self.pos >= self.data.len) { return RpcError.Incomplete; }
        const b = self.data[self.pos];
        self.pos += 1;
        return b;
    }

    fn readSlice(self: *Reader, n: usize): RpcError![]const u8 {
        if (self.pos + n > self.data.len) { return RpcError.Incomplete; }
        const s = self.data[self.pos .. self.pos + n];
        self.pos += n;
        return s;
    }

    fn readInt(self: *Reader, comptime T: type): RpcError!T {
        const s = try self.readSlice(@sizeOf(T));
        return std.mem.readInt(T, s[0..@sizeOf(T)], .big);
    }
};

fn decodeRpcValueAt(alloc: std.mem.Allocator, r: *Reader, depth: usize): RpcError!RpcValue {
    if (depth > MAX_RPC_DEPTH) {
        return RpcError.MaxDepthExceeded;
    }
    const b = try r.readByte();
    if (b <= 0x7F) {
        return RpcValue{ .uint_val = b };
    }
    if (b >= 0xE0) {
        const sv: i8 = @bitCast(b);
        return RpcValue{ .int_val = sv };
    }
    if (b >= 0xA0 and b <= 0xBF) {
        const n = @as(usize, b & 0x1F);
        const s = try r.readSlice(n);
        return RpcValue{ .str_val = try alloc.dupe(u8, s) };
    }
    if (b >= 0x90 and b <= 0x9F) {
        return decodeArray(alloc, r, @as(usize, b & 0x0F), depth + 1);
    }
    if (b >= 0x80 and b <= 0x8F) {
        return decodeMap(alloc, r, @as(usize, b & 0x0F), depth + 1);
    }

    switch (b) {
        0xC0 => return RpcValue{ .nil = {} },
        0xC2 => return RpcValue{ .bool_val = false },
        0xC3 => return RpcValue{ .bool_val = true },
        0xC4 => {
            const n = @as(usize, try r.readByte());
            return RpcValue{ .bin_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xC5 => {
            const n = @as(usize, try r.readInt(u16));
            return RpcValue{ .bin_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xC6 => {
            const n = @as(usize, try r.readInt(u32));
            if (n > DEFAULT_MAX_MSG_BYTES) { return RpcError.MessageTooLarge; }
            return RpcValue{ .bin_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xCB => {
            const bits = try r.readInt(u64);
            return RpcValue{ .float_val = @bitCast(bits) };
        },
        0xCC => return RpcValue{ .uint_val = try r.readByte() },
        0xCD => return RpcValue{ .uint_val = try r.readInt(u16) },
        0xCE => return RpcValue{ .uint_val = try r.readInt(u32) },
        0xCF => return RpcValue{ .uint_val = try r.readInt(u64) },
        0xD0 => {
            const sv: i8 = @bitCast(try r.readByte());
            return RpcValue{ .int_val = sv };
        },
        0xD1 => return RpcValue{ .int_val = try r.readInt(i16) },
        0xD2 => return RpcValue{ .int_val = try r.readInt(i32) },
        0xD3 => return RpcValue{ .int_val = try r.readInt(i64) },
        0xD9 => {
            const n = @as(usize, try r.readByte());
            return RpcValue{ .str_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xDA => {
            const n = @as(usize, try r.readInt(u16));
            return RpcValue{ .str_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xDB => {
            const n = @as(usize, try r.readInt(u32));
            if (n > DEFAULT_MAX_MSG_BYTES) { return RpcError.MessageTooLarge; }
            return RpcValue{ .str_val = try alloc.dupe(u8, try r.readSlice(n)) };
        },
        0xDC => return decodeArray(alloc, r, @as(usize, try r.readInt(u16)), depth + 1),
        0xDD => {
            const n = @as(usize, try r.readInt(u32));
            if (n > 65536) { return RpcError.MessageTooLarge; }
            return decodeArray(alloc, r, n, depth + 1);
        },
        0xDE => return decodeMap(alloc, r, @as(usize, try r.readInt(u16)), depth + 1),
        0xDF => {
            const n = @as(usize, try r.readInt(u32));
            if (n > 65536) { return RpcError.MessageTooLarge; }
            return decodeMap(alloc, r, n, depth + 1);
        },
        else => return RpcError.InvalidFormat,
    }
}

fn decodeArray(alloc: std.mem.Allocator, r: *Reader, count: usize, depth: usize): RpcError!RpcValue {
    let list: std.ArrayList(RpcValue) = .empty;
    errdefer {
        for (item of list.items) { item.deinit(alloc); }
        list.deinit(alloc);
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        const v = try decodeRpcValueAt(alloc, r, depth);
        try list.append(alloc, v);
    }
    return RpcValue{ .array_val = try list.toOwnedSlice(alloc) };
}

fn decodeMap(alloc: std.mem.Allocator, r: *Reader, count: usize, depth: usize): RpcError!RpcValue {
    let list: std.ArrayList(RpcMapEntry) = .empty;
    errdefer {
        for (e of list.items) {
            e.key.deinit(alloc);
            e.value.deinit(alloc);
        }
        list.deinit(alloc);
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        let k = try decodeRpcValueAt(alloc, r, depth);
        errdefer k.deinit(alloc);
        const v = try decodeRpcValueAt(alloc, r, depth);
        try list.append(alloc, RpcMapEntry{ .key = k, .value = v });
    }
    return RpcValue{ .map_val = try list.toOwnedSlice(alloc) };
}

pub const DecodedRpcMessage = struct {
    message: RpcMessage,
    bytes_consumed: usize,
};

fn extractMsgId(v: RpcValue): RpcError!u32 {
    switch (v) {
        .uint_val => |u| {
            if (u > 0xFFFF_FFFF) { return RpcError.InvalidFrame; }
            return @intCast(u);
        },
        .int_val => |iv| {
            if (iv < 0 or iv > 0xFFFF_FFFF) { return RpcError.InvalidFrame; }
            return @intCast(iv);
        },
        else => return RpcError.InvalidFrame,
    }
}

pub fn decodeRpcMessage(alloc: std.mem.Allocator, bytes: []const u8): RpcError!DecodedRpcMessage {
    let r = Reader{ .data = bytes };
    let root = try decodeRpcValueAt(alloc, &r, 0);
    let owns_root = true;
    errdefer {
        if (owns_root) { root.deinit(alloc); }
    }

    switch (root) {
        .array_val => |arr| {
            if (arr.len < 3) { return RpcError.InvalidFrame; }
            const kind_val = arr[0];
            let kind: u64 = 99;
            switch (kind_val) {
                .uint_val => |u| kind = u,
                .int_val => |iv| {
                    if (iv < 0) { return RpcError.InvalidMsgType; }
                    kind = @intCast(iv);
                },
                else => return RpcError.InvalidMsgType,
            }

            if (kind == 0) {
                if (arr.len != 4) { return RpcError.InvalidFrame; }
                const msgid = try extractMsgId(arr[1]);
                const method = switch (arr[2]) {
                    .str_val => |s| s,
                    else => return RpcError.InvalidFrame,
                };
                const params = switch (arr[3]) {
                    .array_val => |p| p,
                    else => return RpcError.InvalidFrame,
                };
                owns_root = false;
                alloc.free(arr);
                return DecodedRpcMessage{
                    .message = RpcMessage{ .request = RpcRequest{
                        .msgid = msgid,
                        .method = method,
                        .params = params,
                    } },
                    .bytes_consumed = r.pos,
                };
            } else if (kind == 1) {
                if (arr.len != 4) { return RpcError.InvalidFrame; }
                const msgid = try extractMsgId(arr[1]);
                const err_val = arr[2];
                const res_val = arr[3];
                owns_root = false;
                alloc.free(arr);
                return DecodedRpcMessage{
                    .message = RpcMessage{ .response = RpcResponse{
                        .msgid = msgid,
                        .err = err_val,
                        .result = res_val,
                    } },
                    .bytes_consumed = r.pos,
                };
            } else if (kind == 2) {
                if (arr.len != 3) { return RpcError.InvalidFrame; }
                const method = switch (arr[1]) {
                    .str_val => |s| s,
                    else => return RpcError.InvalidFrame,
                };
                const params = switch (arr[2]) {
                    .array_val => |p| p,
                    else => return RpcError.InvalidFrame,
                };
                owns_root = false;
                alloc.free(arr);
                return DecodedRpcMessage{
                    .message = RpcMessage{ .notification = RpcNotification{
                        .method = method,
                        .params = params,
                    } },
                    .bytes_consumed = r.pos,
                };
            } else {
                return RpcError.InvalidMsgType;
            }
        },
        else => return RpcError.InvalidFrame,
    }
}

pub const RpcStreamDecoder = struct {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    max_msg_bytes: usize,

    pub fn init(alloc: std.mem.Allocator, max_msg_bytes: usize): RpcStreamDecoder {
        return RpcStreamDecoder{
            .alloc = alloc,
            .buf = .empty,
            .max_msg_bytes = if (max_msg_bytes == 0) DEFAULT_MAX_MSG_BYTES else max_msg_bytes,
        };
    }

    pub fn deinit(self: *RpcStreamDecoder): void {
        self.buf.deinit(self.alloc);
    }

    pub fn feed(self: *RpcStreamDecoder, chunk: []const u8): RpcError!void {
        if (self.buf.items.len + chunk.len > self.max_msg_bytes) {
            return RpcError.MessageTooLarge;
        }
        try self.buf.appendSlice(self.alloc, chunk);
    }

    pub fn next(self: *RpcStreamDecoder): RpcError!(RpcMessage?) {
        if (self.buf.items.len == 0) { return null; }
        const decoded = decodeRpcMessage(self.alloc, self.buf.items) catch |e| {
            if (e == RpcError.Incomplete) { return null; }
            return e;
        };
        const consumed = decoded.bytes_consumed;
        const remaining = self.buf.items.len - consumed;
        if (remaining > 0) {
            std.mem.copyForwards(u8, self.buf.items[0..remaining], self.buf.items[consumed..]);
        }
        self.buf.items.len = remaining;
        return decoded.message;
    }
};
