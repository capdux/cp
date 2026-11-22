// Source: cp/lib/amqp/amqp.cp (AMQP 0-9-1 Binary Wire Framing, FieldTable & Stream Decoder)
// Pure CP implementation of AMQP 0-9-1 big-endian framing, Method/ContentHeader/Heartbeat
// helpers, recursive FieldTable/AmqpValue codec with 32-level depth guard, and incremental
// half-packet / sticky-packet stream decoder.

const std = @import("std");

pub const PROTOCOL_HEADER: []const u8 = "AMQP\x00\x00\x09\x01";
pub const FRAME_END: u8 = 0xCE;
pub const AMQP_MAX_DEPTH: usize = 32;
pub const DEFAULT_MAX_FRAME_SIZE: usize = 131072;

pub const PROP_FLAG_CONTENT_TYPE: u16 = 0x8000;
pub const PROP_FLAG_HEADERS: u16 = 0x2000;

pub err AmqpError {
    InvalidFrameEnd,
    InvalidFrameType,
    UnexpectedEof,
    FrameTooLarge,
    InvalidFieldType,
    StringTooLong,
    MaxDepthExceeded,
    InvalidChannel,
    OutOfMemory,
}

pub enum AmqpFrameType: u8 {
    Method = 1,
    Header = 2,
    Body = 3,
    Heartbeat = 8,
}

pub fn frameTypeFromU8(raw: u8): AmqpFrameType! {
    return switch (raw) {
        1 => .Method,
        2 => .Header,
        3 => .Body,
        8 => .Heartbeat,
        else => err.InvalidFrameType,
    };
}

pub fn frameTypeToU8(ft: AmqpFrameType): u8 {
    return switch (ft) {
        .Method => 1,
        .Header => 2,
        .Body => 3,
        .Heartbeat => 8,
    };
}

const empty_fields_storage: [0]AmqpField = [];

pub struct AmqpField {
    name: []const u8 = "",
    value: AmqpValue = .void_val,

    pub static fn init(name: []const u8, value: AmqpValue): AmqpField {
        return AmqpField{ name: name, value: value };
    }
}

pub fn deinitFieldTable(alloc: std.mem.Allocator, fields: []AmqpField) {
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        if (fields[i].name.len > 0) {
            alloc.free(fields[i].name);
        }
        fields[i].value.deinit(alloc);
    }
    if (fields.len > 0) {
        alloc.free(fields);
    }
}

pub fn getField(fields: []const AmqpField, name: []const u8): AmqpValue? {
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        if (std.mem.eql(u8, fields[i].name, name)) {
            return fields[i].value;
        }
    }
    return null;
}

pub fn eqlFieldTable(a: []const AmqpField, b: []const AmqpField): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!std.mem.eql(u8, a[i].name, b[i].name)) return false;
        if (!a[i].value.eql(b[i].value)) return false;
    }
    return true;
}

pub enum AmqpValue {
    boolean: bool,
    i32_val: i32,
    i64_val: i64,
    f64_val: f64,
    long_str: []const u8,
    field_array: []AmqpValue,
    field_table: []AmqpField,
    void_val,

    pub fn deinit(alloc: std.mem.Allocator) {
        switch (_) {
            .long_str s => {
                if (s.len > 0) {
                    alloc.free(s);
                }
            },
            .field_array items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                if (items.len > 0) {
                    alloc.free(items);
                }
            },
            .field_table fields => {
                deinitFieldTable(alloc, fields);
            },
            else => {},
        }
    }

    pub fn get(field_name: []const u8): AmqpValue? {
        return switch (_) {
            .field_table fields => getField(fields, field_name),
            else => null,
        };
    }

    pub fn eql(other: AmqpValue): bool {
        return switch (_) {
            .boolean a => switch (other) {
                .boolean b => a == b,
                else => false,
            },
            .i32_val a => switch (other) {
                .i32_val b => a == b,
                else => false,
            },
            .i64_val a => switch (other) {
                .i64_val b => a == b,
                else => false,
            },
            .f64_val a => switch (other) {
                .f64_val b => {
                    if (std.math.isNan(a) && std.math.isNan(b)) return true;
                    return a == b;
                },
                else => false,
            },
            .long_str a => switch (other) {
                .long_str b => std.mem.eql(u8, a, b),
                else => false,
            },
            .field_array a => switch (other) {
                .field_array b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
            .field_table a => switch (other) {
                .field_table b => eqlFieldTable(a, b),
                else => false,
            },
            .void_val => switch (other) {
                .void_val => true,
                else => false,
            },
        };
    }
}

// ============================================================================
// Big-Endian Wire Helpers
// ============================================================================

fn writeU16BE(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), v: u16): void! {
    let b: [2]u8 = [
        ((v >> 8) & 0xFF) as u8,
        (v & 0xFF) as u8,
    ];
    list.appendSlice(alloc, &b) catch return err.OutOfMemory;
}

fn writeU32BE(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), v: u32): void! {
    let b: [4]u8 = [
        ((v >> 24) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        (v & 0xFF) as u8,
    ];
    list.appendSlice(alloc, &b) catch return err.OutOfMemory;
}

fn writeU64BE(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), v: u64): void! {
    let b: [8]u8 = [
        ((v >> 56) & 0xFF) as u8,
        ((v >> 48) & 0xFF) as u8,
        ((v >> 40) & 0xFF) as u8,
        ((v >> 32) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        (v & 0xFF) as u8,
    ];
    list.appendSlice(alloc, &b) catch return err.OutOfMemory;
}

fn writeU32BEAt(buf: []u8, offset: usize, v: u32) {
    buf[offset] = ((v >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((v >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((v >> 8) & 0xFF) as u8;
    buf[offset + 3] = (v & 0xFF) as u8;
}

fn readU16BE(data: []const u8, offset: usize): u16! {
    if (offset + 2 > data.len) return err.UnexpectedEof;
    return ((data[offset] as u16) << 8) | (data[offset + 1] as u16);
}

fn readU32BE(data: []const u8, offset: usize): u32! {
    if (offset + 4 > data.len) return err.UnexpectedEof;
    return ((data[offset] as u32) << 24) |
        ((data[offset + 1] as u32) << 16) |
        ((data[offset + 2] as u32) << 8) |
        (data[offset + 3] as u32);
}

fn readU64BE(data: []const u8, offset: usize): u64! {
    if (offset + 8 > data.len) return err.UnexpectedEof;
    return ((data[offset] as u64) << 56) |
        ((data[offset + 1] as u64) << 48) |
        ((data[offset + 2] as u64) << 40) |
        ((data[offset + 3] as u64) << 32) |
        ((data[offset + 4] as u64) << 24) |
        ((data[offset + 5] as u64) << 16) |
        ((data[offset + 6] as u64) << 8) |
        (data[offset + 7] as u64);
}

fn writeShortStr(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), s: []const u8): void! {
    if (s.len > 255) return err.StringTooLong;
    list.append(alloc, s.len as u8) catch return err.OutOfMemory;
    if (s.len > 0) {
        list.appendSlice(alloc, s) catch return err.OutOfMemory;
    }
}

fn readShortStr(alloc: std.mem.Allocator, data: []const u8, offset: *usize): ([]const u8)! {
    if (offset.* + 1 > data.len) return err.UnexpectedEof;
    const len = data[offset.*] as usize;
    offset.* += 1;
    if (offset.* + len > data.len) return err.UnexpectedEof;
    const slice = data[offset.* .. offset.* + len];
    offset.* += len;
    if (len == 0) {
        return "";
    }
    return alloc.dupe(u8, slice) catch return err.OutOfMemory;
}

// ============================================================================
// FieldTable & AmqpValue Codec
// ============================================================================

fn encodeValueInto(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), val: AmqpValue, depth: usize): void! {
    if (depth > AMQP_MAX_DEPTH) return err.MaxDepthExceeded;
    switch (val) {
        .boolean b => {
            list.append(alloc, 't') catch return err.OutOfMemory;
            list.append(alloc, if (b) 1 else 0) catch return err.OutOfMemory;
        },
        .i32_val v => {
            list.append(alloc, 'I') catch return err.OutOfMemory;
            const u: u32 = @bitCast(v);
            writeU32BE(alloc, list, u)?;
        },
        .i64_val v => {
            list.append(alloc, 'l') catch return err.OutOfMemory;
            const u: u64 = @bitCast(v);
            writeU64BE(alloc, list, u)?;
        },
        .f64_val v => {
            list.append(alloc, 'd') catch return err.OutOfMemory;
            const u: u64 = @bitCast(v);
            writeU64BE(alloc, list, u)?;
        },
        .long_str s => {
            if (s.len > 0xFFFFFFFF) return err.StringTooLong;
            list.append(alloc, 'S') catch return err.OutOfMemory;
            writeU32BE(alloc, list, s.len as u32)?;
            if (s.len > 0) {
                list.appendSlice(alloc, s) catch return err.OutOfMemory;
            }
        },
        .field_array items => {
            list.append(alloc, 'A') catch return err.OutOfMemory;
            const size_pos = list.items.len;
            writeU32BE(alloc, list, 0)?;
            const body_start = list.items.len;
            let i: usize = 0;
            while (i < items.len) : (i += 1) {
                encodeValueInto(alloc, list, items[i], depth + 1)?;
            }
            const array_bytes = list.items.len - body_start;
            if (array_bytes > 0xFFFFFFFF) return err.FrameTooLarge;
            writeU32BEAt(list.items, size_pos, array_bytes as u32);
        },
        .field_table fields => {
            list.append(alloc, 'F') catch return err.OutOfMemory;
            encodeFieldTableInto(alloc, list, fields, depth + 1)?;
        },
        .void_val => {
            list.append(alloc, 'V') catch return err.OutOfMemory;
        },
    }
}

fn encodeFieldTableInto(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), fields: []const AmqpField, depth: usize): void! {
    if (depth > AMQP_MAX_DEPTH) return err.MaxDepthExceeded;
    const size_pos = list.items.len;
    writeU32BE(alloc, list, 0)?;
    const body_start = list.items.len;
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        writeShortStr(alloc, list, fields[i].name)?;
        encodeValueInto(alloc, list, fields[i].value, depth)?;
    }
    const table_bytes = list.items.len - body_start;
    if (table_bytes > 0xFFFFFFFF) return err.FrameTooLarge;
    writeU32BEAt(list.items, size_pos, table_bytes as u32);
}

pub fn encodeFieldTable(alloc: std.mem.Allocator, fields: []const AmqpField): ([]u8)! {
    let list: std.ArrayListUnmanaged(u8) = .{};
    errdefer list.deinit(alloc);
    encodeFieldTableInto(alloc, &list, fields, 0)?;
    return list.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

fn decodeValueAt(alloc: std.mem.Allocator, data: []const u8, offset: *usize, depth: usize, max_depth: usize): AmqpValue! {
    if (depth > max_depth) return err.MaxDepthExceeded;
    if (offset.* + 1 > data.len) return err.UnexpectedEof;
    const tag = data[offset.*];
    offset.* += 1;

    switch (tag) {
        't' => {
            if (offset.* + 1 > data.len) return err.UnexpectedEof;
            const b = data[offset.*] != 0;
            offset.* += 1;
            return AmqpValue{ boolean: b };
        },
        'I' => {
            const u = readU32BE(data, offset.*)?;
            offset.* += 4;
            const v: i32 = @bitCast(u);
            return AmqpValue{ i32_val: v };
        },
        'l' => {
            const u = readU64BE(data, offset.*)?;
            offset.* += 8;
            const v: i64 = @bitCast(u);
            return AmqpValue{ i64_val: v };
        },
        'd' => {
            const u = readU64BE(data, offset.*)?;
            offset.* += 8;
            const v: f64 = @bitCast(u);
            return AmqpValue{ f64_val: v };
        },
        'S' => {
            const slen = (readU32BE(data, offset.*)?) as usize;
            offset.* += 4;
            if (offset.* + slen > data.len) return err.UnexpectedEof;
            const slice = data[offset.* .. offset.* + slen];
            offset.* += slen;
            if (slen == 0) {
                return AmqpValue{ long_str: "" };
            }
            const dup = alloc.dupe(u8, slice) catch return err.OutOfMemory;
            return AmqpValue{ long_str: dup };
        },
        'A' => {
            const arr_len = (readU32BE(data, offset.*)?) as usize;
            offset.* += 4;
            if (offset.* + arr_len > data.len) return err.UnexpectedEof;
            const end_pos = offset.* + arr_len;
            let items: std.ArrayListUnmanaged(AmqpValue) = .{};
            errdefer {
                let k: usize = 0;
                while (k < items.items.len) : (k += 1) {
                    items.items[k].deinit(alloc);
                }
                items.deinit(alloc);
            }
            while (offset.* < end_pos) {
                let v = decodeValueAt(alloc, data, offset, depth + 1, max_depth)?;
                items.append(alloc, v) catch {
                    v.deinit(alloc);
                    return err.OutOfMemory;
                };
            }
            const owned = items.toOwnedSlice(alloc) catch return err.OutOfMemory;
            return AmqpValue{ field_array: owned };
        },
        'F' => {
            const tbl = decodeFieldTableAt(alloc, data, offset, depth + 1, max_depth)?;
            return AmqpValue{ field_table: tbl };
        },
        'V' => {
            return .void_val;
        },
        else => return err.InvalidFieldType,
    }
}

fn decodeFieldTableAt(alloc: std.mem.Allocator, data: []const u8, offset: *usize, depth: usize, max_depth: usize): ([]AmqpField)! {
    if (depth > max_depth) return err.MaxDepthExceeded;
    const table_len = (readU32BE(data, offset.*)?) as usize;
    offset.* += 4;
    if (offset.* + table_len > data.len) return err.UnexpectedEof;
    const end_pos = offset.* + table_len;

    let fields: std.ArrayListUnmanaged(AmqpField) = .{};
    errdefer {
        let k: usize = 0;
        while (k < fields.items.len) : (k += 1) {
            if (fields.items[k].name.len > 0) {
                alloc.free(fields.items[k].name);
            }
            fields.items[k].value.deinit(alloc);
        }
        fields.deinit(alloc);
    }

    while (offset.* < end_pos) {
        let name = readShortStr(alloc, data, offset)?;
        let val = decodeValueAt(alloc, data, offset, depth, max_depth) catch |e| {
            if (name.len > 0) alloc.free(name);
            return e;
        };
        fields.append(alloc, AmqpField{ name: name, value: val }) catch {
            if (name.len > 0) alloc.free(name);
            val.deinit(alloc);
            return err.OutOfMemory;
        };
    }

    return fields.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

pub fn decodeFieldTableWithMaxDepth(alloc: std.mem.Allocator, bytes: []const u8, max_depth: usize): ([]AmqpField)! {
    let offset: usize = 0;
    const fields = decodeFieldTableAt(alloc, bytes, &offset, 0, max_depth)?;
    if (offset != bytes.len) {
        deinitFieldTable(alloc, fields);
        return err.UnexpectedEof;
    }
    return fields;
}

pub fn decodeFieldTable(alloc: std.mem.Allocator, bytes: []const u8): ([]AmqpField)! {
    return decodeFieldTableWithMaxDepth(alloc, bytes, AMQP_MAX_DEPTH);
}

// ============================================================================
// Frame Encoding & Decoding
// ============================================================================

pub struct AmqpFrame {
    frame_type: AmqpFrameType = .Method,
    channel: u16 = 0,
    payload: []const u8 = "",

    pub fn deinit(alloc: std.mem.Allocator) {
        if (_.payload.len > 0) {
            alloc.free(_.payload);
        }
    }
}

pub fn encodeFrame(alloc: std.mem.Allocator, frame_type: AmqpFrameType, channel: u16, payload: []const u8): ([]u8)! {
    if (frame_type == .Heartbeat && channel != 0) {
        return err.InvalidChannel;
    }
    if (payload.len > 0xFFFFFFFF) {
        return err.FrameTooLarge;
    }
    const total_len = 8 + payload.len;
    let out = alloc.alloc(u8, total_len) catch return err.OutOfMemory;
    out[0] = frameTypeToU8(frame_type);
    out[1] = ((channel >> 8) & 0xFF) as u8;
    out[2] = (channel & 0xFF) as u8;
    const psize = payload.len as u32;
    out[3] = ((psize >> 24) & 0xFF) as u8;
    out[4] = ((psize >> 16) & 0xFF) as u8;
    out[5] = ((psize >> 8) & 0xFF) as u8;
    out[6] = (psize & 0xFF) as u8;
    if (payload.len > 0) {
        @memcpy(out[7 .. 7 + payload.len], payload);
    }
    out[7 + payload.len] = FRAME_END;
    return out;
}

pub struct AmqpMethodPayload {
    class_id: u16 = 0,
    method_id: u16 = 0,
    args: []const u8 = "",
}

pub fn encodeMethodFrame(alloc: std.mem.Allocator, channel: u16, class_id: u16, method_id: u16, args: []const u8): ([]u8)! {
    const payload_len = 4 + args.len;
    let payload = alloc.alloc(u8, payload_len) catch return err.OutOfMemory;
    defer alloc.free(payload);

    payload[0] = ((class_id >> 8) & 0xFF) as u8;
    payload[1] = (class_id & 0xFF) as u8;
    payload[2] = ((method_id >> 8) & 0xFF) as u8;
    payload[3] = (method_id & 0xFF) as u8;
    if (args.len > 0) {
        @memcpy(payload[4 .. 4 + args.len], args);
    }
    return encodeFrame(alloc, .Method, channel, payload);
}

pub fn decodeMethodPayload(payload: []const u8): AmqpMethodPayload! {
    if (payload.len < 4) return err.UnexpectedEof;
    const class_id = readU16BE(payload, 0)?;
    const method_id = readU16BE(payload, 2)?;
    return AmqpMethodPayload{
        class_id: class_id,
        method_id: method_id,
        args: payload[4..],
    };
}

pub struct AmqpContentHeader {
    class_id: u16 = 0,
    weight: u16 = 0,
    body_size: u64 = 0,
    content_type: ([]const u8)? = null,
    headers: []AmqpField = &empty_fields_storage,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (ct := _.content_type) {
            if (ct.len > 0) {
                alloc.free(ct);
            }
        }
        deinitFieldTable(alloc, _.headers);
    }
}

pub fn encodeContentHeader(
    alloc: std.mem.Allocator,
    channel: u16,
    class_id: u16,
    body_size: u64,
    content_type: ([]const u8)?,
    headers: []const AmqpField,
): ([]u8)! {
    let list: std.ArrayListUnmanaged(u8) = .{};
    defer list.deinit(alloc);

    writeU16BE(alloc, &list, class_id)?;
    writeU16BE(alloc, &list, 0)?;
    writeU64BE(alloc, &list, body_size)?;

    let flags: u16 = 0;
    if (content_type != null) {
        flags |= PROP_FLAG_CONTENT_TYPE;
    }
    if (headers.len > 0) {
        flags |= PROP_FLAG_HEADERS;
    }
    writeU16BE(alloc, &list, flags)?;

    if (ct := content_type) {
        writeShortStr(alloc, &list, ct)?;
    }
    if (headers.len > 0) {
        encodeFieldTableInto(alloc, &list, headers, 0)?;
    }

    return encodeFrame(alloc, .Header, channel, list.items);
}

pub fn decodeContentHeader(alloc: std.mem.Allocator, payload: []const u8): AmqpContentHeader! {
    if (payload.len < 14) return err.UnexpectedEof;
    const class_id = readU16BE(payload, 0)?;
    const weight = readU16BE(payload, 2)?;
    const body_size = readU64BE(payload, 4)?;
    const flags = readU16BE(payload, 12)?;

    let offset: usize = 14;
    let content_type: ([]const u8)? = null;
    errdefer {
        if (ct := content_type) {
            if (ct.len > 0) alloc.free(ct);
        }
    }

    if ((flags & PROP_FLAG_CONTENT_TYPE) != 0) {
        content_type = readShortStr(alloc, payload, &offset)?;
    }

    let headers: []AmqpField = &empty_fields_storage;
    if ((flags & PROP_FLAG_HEADERS) != 0) {
        headers = decodeFieldTableAt(alloc, payload, &offset, 0, AMQP_MAX_DEPTH)?;
    }

    return AmqpContentHeader{
        class_id: class_id,
        weight: weight,
        body_size: body_size,
        content_type: content_type,
        headers: headers,
    };
}

pub fn encodeHeartbeat(alloc: std.mem.Allocator): ([]u8)! {
    return encodeFrame(alloc, .Heartbeat, 0, "");
}

// ============================================================================
// Incremental Stream Decoder (Half-Packet & Sticky-Packet Framing)
// ============================================================================

pub struct AmqpStreamDecoder {
    alloc: std.mem.Allocator,
    max_frame_size: usize = DEFAULT_MAX_FRAME_SIZE,
    buf: std.ArrayListUnmanaged(u8) = .{},

    pub static fn init(alloc: std.mem.Allocator, max_frame_size: usize): AmqpStreamDecoder {
        return AmqpStreamDecoder{
            alloc: alloc,
            max_frame_size: max_frame_size,
            buf: .{},
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn feed(*_, chunk: []const u8): void! {
        if (chunk.len == 0) return;
        _.buf.appendSlice(_.alloc, chunk) catch return err.OutOfMemory;
    }

    pub fn nextFrame(*_): (AmqpFrame?)! {
        if (_.buf.items.len < 7) {
            return null;
        }
        const ft = frameTypeFromU8(_.buf.items[0])?;
        const channel = readU16BE(_.buf.items, 1)?;
        if (ft == .Heartbeat && channel != 0) {
            return err.InvalidChannel;
        }
        const payload_size = (readU32BE(_.buf.items, 3)?) as usize;
        if (payload_size > _.max_frame_size) {
            return err.FrameTooLarge;
        }
        const total_len = 8 + payload_size;
        if (_.buf.items.len < total_len) {
            return null;
        }
        if (_.buf.items[7 + payload_size] != FRAME_END) {
            return err.InvalidFrameEnd;
        }

        let payload_copy: []const u8 = "";
        if (payload_size > 0) {
            payload_copy = _.alloc.dupe(u8, _.buf.items[7 .. 7 + payload_size]) catch return err.OutOfMemory;
        }

        const remaining = _.buf.items.len - total_len;
        if (remaining > 0) {
            std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[total_len.._.buf.items.len]);
        }
        _.buf.items.len = remaining;

        return AmqpFrame{
            frame_type: ft,
            channel: channel,
            payload: payload_copy,
        };
    }
}
