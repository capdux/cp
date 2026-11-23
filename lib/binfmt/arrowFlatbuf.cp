// Source: cp/lib/arrow_ipc/flatbuf.cp (Lightweight deterministic Flatbuffer codec for Arrow IPC)

const std = @import("std");
import { ArrowError, ArrowType, ArrowField, ArrowSchema } from "./arrowTypes.cp";

pub struct FbNode {
    pub length: i64,
    pub null_count: i64,
}

pub struct FbBuffer {
    pub offset: i64,
    pub length: i64,
}

pub struct FbBlock {
    pub offset: i64,
    pub meta_len: i32,
    pub body_len: i64,
}

pub enum FbMessageHeader {
    Schema,
    RecordBatch,
}

pub struct FbDecodedMessage {
    pub header_type: FbMessageHeader,
    pub body_length: i64,
    pub schema: ArrowSchema?,
    pub row_count: i64,
    pub nodes: []FbNode,
    pub buffers: []FbBuffer,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (*s := _.schema) {
            s.deinit(alloc);
        }
        if (_.nodes.len > 0) {
            alloc.free(_.nodes);
        }
        if (_.buffers.len > 0) {
            alloc.free(_.buffers);
        }
        _.nodes = &.{};
        _.buffers = &.{};
    }
}

pub struct FbDecodedFooter {
    pub schema: ArrowSchema,
    pub record_batches: []FbBlock,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.schema.deinit(alloc);
        alloc.free(_.record_batches);
        _.record_batches = &.{};
    }
}

struct EncodedType {
    type_id: u8,
    off: u32,
}

struct FbBuilder {
    buf: std.ArrayList(u8),

    fn init(): FbBuilder {
        return FbBuilder{ .buf = std.ArrayList(u8).empty };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    fn offset(): u32 {
        return @intCast(_.buf.items.len);
    }

    fn pushBytesRev(*_, alloc: std.mem.Allocator, data: []const u8): ArrowError!void {
        let i: usize = data.len;
        while (i > 0) {
            i -= 1;
            _.buf.append(alloc, data[i]) catch return ArrowError.OutOfMemory;
        }
    }

    fn pushU8(*_, alloc: std.mem.Allocator, v: u8): ArrowError!void {
        _.buf.append(alloc, v) catch return ArrowError.OutOfMemory;
    }

    fn pushU16(*_, alloc: std.mem.Allocator, v: u16): ArrowError!void {
        let b: [2]u8 = undefined;
        std.mem.writeInt(u16, &b, v, .little);
        try _.pushBytesRev(alloc, &b);
    }

    fn pushI16(*_, alloc: std.mem.Allocator, v: i16): ArrowError!void {
        let b: [2]u8 = undefined;
        std.mem.writeInt(i16, &b, v, .little);
        try _.pushBytesRev(alloc, &b);
    }

    fn pushU32(*_, alloc: std.mem.Allocator, v: u32): ArrowError!void {
        let b: [4]u8 = undefined;
        std.mem.writeInt(u32, &b, v, .little);
        try _.pushBytesRev(alloc, &b);
    }

    fn pushI32(*_, alloc: std.mem.Allocator, v: i32): ArrowError!void {
        let b: [4]u8 = undefined;
        std.mem.writeInt(i32, &b, v, .little);
        try _.pushBytesRev(alloc, &b);
    }

    fn pushI64(*_, alloc: std.mem.Allocator, v: i64): ArrowError!void {
        let b: [8]u8 = undefined;
        std.mem.writeInt(i64, &b, v, .little);
        try _.pushBytesRev(alloc, &b);
    }

    fn pushOffset(*_, alloc: std.mem.Allocator, target: u32): ArrowError!void {
        const cur = _.offset() + 4;
        try _.pushU32(alloc, cur - target);
    }

    fn createString(*_, alloc: std.mem.Allocator, s: []const u8): ArrowError!u32 {
        try _.pushU8(alloc, 0);
        try _.pushBytesRev(alloc, s);
        try _.pushU32(alloc, @intCast(s.len));
        return _.offset();
    }

    fn createOffsetVector(*_, alloc: std.mem.Allocator, targets: []const u32): ArrowError!u32 {
        let i: usize = targets.len;
        while (i > 0) {
            i -= 1;
            try _.pushOffset(alloc, targets[i]);
        }
        try _.pushU32(alloc, @intCast(targets.len));
        return _.offset();
    }

    fn finish(*_, alloc: std.mem.Allocator, root_table: u32): ArrowError![]u8 {
        try _.pushOffset(alloc, root_table);
        const out = _.buf.toOwnedSlice(alloc) catch return ArrowError.OutOfMemory;
        std.mem.reverse(u8, out);
        return out;
    }
}

fn writeTable(
    b: *FbBuilder,
    alloc: std.mem.Allocator,
    num_fields: usize,
    field_offsets_from_end: []const u16,
    table_body_end: u32,
    table_body_start: u32,
): ArrowError!u32 {
    const table_start = table_body_start + 4;
    const obj_size: u16 = @intCast(table_start - table_body_end);
    const vtable_size: u16 = @intCast(4 + 2 * num_fields);
    try b.pushI32(alloc, @intCast(vtable_size));
    const final_table_off = b.offset();

    let i: usize = num_fields;
    while (i > 0) {
        i -= 1;
        try b.pushU16(alloc, field_offsets_from_end[i]);
    }
    try b.pushU16(alloc, obj_size);
    try b.pushU16(alloc, vtable_size);
    return final_table_off;
}

fn encodeTypeTable(b: *FbBuilder, alloc: std.mem.Allocator, t: ArrowType): ArrowError!EncodedType {
    if (t == .Int32 or t == .Int64) {
        const body_end = b.offset();
        const bit_width: i32 = if (t == .Int32) 32 else 64;
        try b.pushU8(alloc, 1);
        const f1_off = b.offset();
        try b.pushI32(alloc, bit_width);
        const f0_off = b.offset();
        const body_start = b.offset();
        const table_start = body_start + 4;
        const slots: [2]u16 = [
            @intCast(table_start - f0_off),
            @intCast(table_start - f1_off),
        ];
        const tbl = try writeTable(b, alloc, 2, &slots, body_end, body_start);
        return EncodedType{ .type_id = 2, .off = tbl };
    }
    if (t == .Float64) {
        const body_end = b.offset();
        try b.pushI16(alloc, 2);
        const f0_off = b.offset();
        const body_start = b.offset();
        const table_start = body_start + 4;
        const slots: [1]u16 = [@intCast(table_start - f0_off)];
        const tbl = try writeTable(b, alloc, 1, &slots, body_end, body_start);
        return EncodedType{ .type_id = 3, .off = tbl };
    }
    if (t == .Bool) {
        const body_end = b.offset();
        const body_start = b.offset();
        const slots: [0]u16 = [];
        const tbl = try writeTable(b, alloc, 0, &slots, body_end, body_start);
        return EncodedType{ .type_id = 6, .off = tbl };
    }
    const body_end = b.offset();
    const body_start = b.offset();
    const slots: [0]u16 = [];
    const tbl = try writeTable(b, alloc, 0, &slots, body_end, body_start);
    return EncodedType{ .type_id = 5, .off = tbl };
}

fn encodeSchemaTable(b: *FbBuilder, alloc: std.mem.Allocator, schema: *const ArrowSchema): ArrowError!u32 {
    let field_offs = alloc.alloc(u32, schema.fields.len) catch return ArrowError.OutOfMemory;
    defer alloc.free(field_offs);

    let i: usize = 0;
    while (i < schema.fields.len) {
        const f = schema.fields[i];
        const name_off = try b.createString(alloc, f.name);
        const type_res = try encodeTypeTable(b, alloc, f.arrow_type);

        const body_end = b.offset();
        try b.pushOffset(alloc, type_res.off);
        const f3_off = b.offset();
        try b.pushU8(alloc, type_res.type_id);
        const f2_off = b.offset();
        try b.pushU8(alloc, if (f.nullable) 1 else 0);
        const f1_off = b.offset();
        try b.pushOffset(alloc, name_off);
        const f0_off = b.offset();

        const body_start = b.offset();
        const table_start = body_start + 4;
        const slots: [4]u16 = [
            @intCast(table_start - f0_off),
            @intCast(table_start - f1_off),
            @intCast(table_start - f2_off),
            @intCast(table_start - f3_off),
        ];
        field_offs[i] = try writeTable(b, alloc, 4, &slots, body_end, body_start);
        i += 1;
    }

    const fields_vec = try b.createOffsetVector(alloc, field_offs);
    const body_end = b.offset();
    try b.pushOffset(alloc, fields_vec);
    const f1_off = b.offset();
    try b.pushI16(alloc, 0);
    const f0_off = b.offset();
    const body_start = b.offset();
    const table_start = body_start + 4;
    const slots: [2]u16 = [
        @intCast(table_start - f0_off),
        @intCast(table_start - f1_off),
    ];
    return writeTable(b, alloc, 2, &slots, body_end, body_start);
}

pub fn encodeSchemaMessage(alloc: std.mem.Allocator, schema: *const ArrowSchema): ArrowError![]u8 {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    const schema_off = try encodeSchemaTable(&b, alloc, schema);

    const body_end = b.offset();
    try b.pushI64(alloc, 0);
    const f3_off = b.offset();
    try b.pushOffset(alloc, schema_off);
    const f2_off = b.offset();
    try b.pushU8(alloc, 1);
    const f1_off = b.offset();
    try b.pushI16(alloc, 4);
    const f0_off = b.offset();

    const body_start = b.offset();
    const table_start = body_start + 4;
    const slots: [4]u16 = [
        @intCast(table_start - f0_off),
        @intCast(table_start - f1_off),
        @intCast(table_start - f2_off),
        @intCast(table_start - f3_off),
    ];
    const msg_off = try writeTable(&b, alloc, 4, &slots, body_end, body_start);
    return b.finish(alloc, msg_off);
}

pub fn encodeRecordBatchMessage(
    alloc: std.mem.Allocator,
    row_count: i64,
    body_length: i64,
    nodes: []const FbNode,
    buffers: []const FbBuffer,
): ArrowError![]u8 {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    let i: usize = buffers.len;
    while (i > 0) {
        i -= 1;
        try b.pushI64(alloc, buffers[i].length);
        try b.pushI64(alloc, buffers[i].offset);
    }
    try b.pushU32(alloc, @intCast(buffers.len));
    const buffers_vec = b.offset();

    i = nodes.len;
    while (i > 0) {
        i -= 1;
        try b.pushI64(alloc, nodes[i].null_count);
        try b.pushI64(alloc, nodes[i].length);
    }
    try b.pushU32(alloc, @intCast(nodes.len));
    const nodes_vec = b.offset();

    const rb_body_end = b.offset();
    try b.pushOffset(alloc, buffers_vec);
    const rb_f2 = b.offset();
    try b.pushOffset(alloc, nodes_vec);
    const rb_f1 = b.offset();
    try b.pushI64(alloc, row_count);
    const rb_f0 = b.offset();
    const rb_body_start = b.offset();
    const rb_tstart = rb_body_start + 4;
    const rb_slots: [3]u16 = [
        @intCast(rb_tstart - rb_f0),
        @intCast(rb_tstart - rb_f1),
        @intCast(rb_tstart - rb_f2),
    ];
    const rb_off = try writeTable(&b, alloc, 3, &rb_slots, rb_body_end, rb_body_start);

    const body_end = b.offset();
    try b.pushI64(alloc, body_length);
    const f3_off = b.offset();
    try b.pushOffset(alloc, rb_off);
    const f2_off = b.offset();
    try b.pushU8(alloc, 3);
    const f1_off = b.offset();
    try b.pushI16(alloc, 4);
    const f0_off = b.offset();

    const body_start = b.offset();
    const table_start = body_start + 4;
    const slots: [4]u16 = [
        @intCast(table_start - f0_off),
        @intCast(table_start - f1_off),
        @intCast(table_start - f2_off),
        @intCast(table_start - f3_off),
    ];
    const msg_off = try writeTable(&b, alloc, 4, &slots, body_end, body_start);
    return b.finish(alloc, msg_off);
}

pub fn encodeFooter(alloc: std.mem.Allocator, schema: *const ArrowSchema, blocks: []const FbBlock): ArrowError![]u8 {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    const schema_off = try encodeSchemaTable(&b, alloc, schema);

    let i: usize = blocks.len;
    while (i > 0) {
        i -= 1;
        try b.pushI64(alloc, blocks[i].body_len);
        try b.pushI32(alloc, 0);
        try b.pushI32(alloc, blocks[i].meta_len);
        try b.pushI64(alloc, blocks[i].offset);
    }
    try b.pushU32(alloc, @intCast(blocks.len));
    const blocks_vec = b.offset();

    const body_end = b.offset();
    try b.pushOffset(alloc, blocks_vec);
    const f3_off = b.offset();
    try b.pushOffset(alloc, schema_off);
    const f1_off = b.offset();
    try b.pushI16(alloc, 4);
    const f0_off = b.offset();

    const body_start = b.offset();
    const table_start = body_start + 4;
    const slots: [4]u16 = [
        @intCast(table_start - f0_off),
        @intCast(table_start - f1_off),
        0,
        @intCast(table_start - f3_off),
    ];
    const footer_off = try writeTable(&b, alloc, 4, &slots, body_end, body_start);
    return b.finish(alloc, footer_off);
}

struct FbReader {
    data: []const u8,

    fn readU8(pos: usize): ArrowError!u8 {
        if (pos >= _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return _.data[pos];
    }

    fn readU16(pos: usize): ArrowError!u16 {
        if (pos + 2 > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(u16, _.data[pos..][0..2], .little);
    }

    fn readU32(pos: usize): ArrowError!u32 {
        if (pos + 4 > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(u32, _.data[pos..][0..4], .little);
    }

    fn readI32(pos: usize): ArrowError!i32 {
        if (pos + 4 > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(i32, _.data[pos..][0..4], .little);
    }

    fn readI64(pos: usize): ArrowError!i64 {
        if (pos + 8 > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(i64, _.data[pos..][0..8], .little);
    }

    fn followUOffset(pos: usize): ArrowError!usize {
        const rel = try _.readU32(pos);
        const target = pos + @as(usize, rel);
        if (target >= _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return target;
    }

    fn fieldOffset(table_pos: usize, field_idx: usize): ArrowError!(usize?) {
        const vtable_rel = try _.readI32(table_pos);
        const tpos_i64: i64 = @intCast(table_pos);
        const vpos_i64 = tpos_i64 - @as(i64, vtable_rel);
        if (vpos_i64 < 0 or @as(usize, @intCast(vpos_i64)) + 4 > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        const vpos: usize = @intCast(vpos_i64);
        const vtable_len = try _.readU16(vpos);
        const slot_pos = 4 + field_idx * 2;
        if (slot_pos + 2 > vtable_len) {
            return null;
        }
        const foff = try _.readU16(vpos + slot_pos);
        if (foff == 0) {
            return null;
        }
        const abs = table_pos + @as(usize, foff);
        if (abs >= _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return abs;
    }

    fn readString(str_pos: usize): ArrowError![]const u8 {
        const len = try _.readU32(str_pos);
        const start = str_pos + 4;
        const end = start + @as(usize, len);
        if (end > _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return _.data[start..end];
    }
}

fn decodeSchemaTable(r: *const FbReader, alloc: std.mem.Allocator, schema_pos: usize): ArrowError!ArrowSchema {
    const fields_slot = (try r.fieldOffset(schema_pos, 1)) orelse return ArrowError.EmptySchema;
    const vec_pos = try r.followUOffset(fields_slot);
    const vec_len = try r.readU32(vec_pos);
    if (vec_len == 0 or vec_len > 65536) {
        return ArrowError.EmptySchema;
    }

    let fields = alloc.alloc(ArrowField, vec_len) catch return ArrowError.OutOfMemory;
    let initialized: usize = 0;
    errdefer {
        let k: usize = 0;
        while (k < initialized) {
            alloc.free(fields[k].name);
            k += 1;
        }
        alloc.free(fields);
    }

    let i: usize = 0;
    while (i < vec_len) {
        const elem_off_pos = vec_pos + 4 + i * 4;
        const field_pos = try r.followUOffset(elem_off_pos);

        let name_slice: []const u8 = "";
        if (nslot := try r.fieldOffset(field_pos, 0)) {
            const spos = try r.followUOffset(nslot);
            name_slice = try r.readString(spos);
        }
        let nullable: bool = true;
        if (null_slot := try r.fieldOffset(field_pos, 1)) {
            nullable = (try r.readU8(null_slot)) != 0;
        }
        const type_type_slot = (try r.fieldOffset(field_pos, 2)) orelse return ArrowError.InvalidFlatbuffer;
        const type_id = try r.readU8(type_type_slot);
        const type_tbl_slot = (try r.fieldOffset(field_pos, 3)) orelse return ArrowError.InvalidFlatbuffer;
        const type_tbl_pos = try r.followUOffset(type_tbl_slot);

        let arrow_type: ArrowType = .Int32;
        if (type_id == 2) {
            const bw_slot = (try r.fieldOffset(type_tbl_pos, 0)) orelse return ArrowError.InvalidFlatbuffer;
            const bw = try r.readI32(bw_slot);
            if (bw == 32) {
                arrow_type = .Int32;
            } else if (bw == 64) {
                arrow_type = .Int64;
            } else {
                return ArrowError.TypeMismatch;
            }
        } else if (type_id == 3) {
            arrow_type = .Float64;
        } else if (type_id == 6) {
            arrow_type = .Bool;
        } else if (type_id == 5) {
            arrow_type = .Utf8;
        } else {
            return ArrowError.TypeMismatch;
        }

        const name_copy = alloc.dupe(u8, name_slice) catch return ArrowError.OutOfMemory;
        fields[i] = ArrowField{
            .name = name_copy,
            .arrow_type = arrow_type,
            .nullable = nullable,
        };
        initialized += 1;
        i += 1;
    }
    return ArrowSchema{ .fields = fields };
}

pub fn decodeMessage(alloc: std.mem.Allocator, bytes: []const u8): ArrowError!FbDecodedMessage {
    if (bytes.len < 8) {
        return ArrowError.InvalidFlatbuffer;
    }
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);

    const ht_slot = (try r.fieldOffset(root_pos, 1)) orelse return ArrowError.InvalidFlatbuffer;
    const ht = try r.readU8(ht_slot);
    const hdr_slot = (try r.fieldOffset(root_pos, 2)) orelse return ArrowError.InvalidFlatbuffer;
    const hdr_pos = try r.followUOffset(hdr_slot);

    let body_length: i64 = 0;
    if (bl_slot := try r.fieldOffset(root_pos, 3)) {
        body_length = try r.readI64(bl_slot);
    }
    if (body_length < 0) {
        return ArrowError.BufferOutOfBounds;
    }

    if (ht == 1) {
        const schema = try decodeSchemaTable(&r, alloc, hdr_pos);
        return FbDecodedMessage{
            .header_type = .Schema,
            .body_length = body_length,
            .schema = schema,
            .row_count = 0,
            .nodes = &.{},
            .buffers = &.{},
        };
    }
    if (ht == 3) {
        const len_slot = (try r.fieldOffset(hdr_pos, 0)) orelse return ArrowError.InvalidFlatbuffer;
        const row_count = try r.readI64(len_slot);
        if (row_count < 0) {
            return ArrowError.RowCountMismatch;
        }

        const nodes_slot = (try r.fieldOffset(hdr_pos, 1)) orelse return ArrowError.InvalidFlatbuffer;
        const nodes_vec = try r.followUOffset(nodes_slot);
        const nodes_len = try r.readU32(nodes_vec);
        if (nodes_vec + 4 + @as(usize, nodes_len) * 16 > bytes.len) {
            return ArrowError.InvalidFlatbuffer;
        }

        let nodes = alloc.alloc(FbNode, nodes_len) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(nodes);
        let i: usize = 0;
        while (i < nodes_len) {
            const p = nodes_vec + 4 + i * 16;
            nodes[i] = FbNode{
                .length = try r.readI64(p),
                .null_count = try r.readI64(p + 8),
            };
            i += 1;
        }

        const bufs_slot = (try r.fieldOffset(hdr_pos, 2)) orelse return ArrowError.InvalidFlatbuffer;
        const bufs_vec = try r.followUOffset(bufs_slot);
        const bufs_len = try r.readU32(bufs_vec);
        if (bufs_vec + 4 + @as(usize, bufs_len) * 16 > bytes.len) {
            return ArrowError.InvalidFlatbuffer;
        }

        let buffers = alloc.alloc(FbBuffer, bufs_len) catch return ArrowError.OutOfMemory;
        i = 0;
        while (i < bufs_len) {
            const p = bufs_vec + 4 + i * 16;
            buffers[i] = FbBuffer{
                .offset = try r.readI64(p),
                .length = try r.readI64(p + 8),
            };
            i += 1;
        }

        return FbDecodedMessage{
            .header_type = .RecordBatch,
            .body_length = body_length,
            .schema = null,
            .row_count = row_count,
            .nodes = nodes,
            .buffers = buffers,
        };
    }
    return ArrowError.InvalidFlatbuffer;
}

pub fn decodeFooter(alloc: std.mem.Allocator, bytes: []const u8): ArrowError!FbDecodedFooter {
    if (bytes.len < 8) {
        return ArrowError.InvalidFooter;
    }
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);

    const schema_slot = (try r.fieldOffset(root_pos, 1)) orelse return ArrowError.InvalidFooter;
    const schema_pos = try r.followUOffset(schema_slot);
    let schema = try decodeSchemaTable(&r, alloc, schema_pos);
    errdefer schema.deinit(alloc);

    let blocks: []FbBlock = undefined;
    if (rb_slot := try r.fieldOffset(root_pos, 3)) {
        const vec_pos = try r.followUOffset(rb_slot);
        const vec_len = try r.readU32(vec_pos);
        if (vec_pos + 4 + @as(usize, vec_len) * 24 > bytes.len) {
            return ArrowError.InvalidFooter;
        }
        blocks = alloc.alloc(FbBlock, vec_len) catch return ArrowError.OutOfMemory;
        let i: usize = 0;
        while (i < vec_len) {
            const p = vec_pos + 4 + i * 24;
            blocks[i] = FbBlock{
                .offset = try r.readI64(p),
                .meta_len = try r.readI32(p + 8),
                .body_len = try r.readI64(p + 16),
            };
            i += 1;
        }
    } else {
        blocks = alloc.alloc(FbBlock, 0) catch return ArrowError.OutOfMemory;
    }
    return FbDecodedFooter{
        .schema = schema,
        .record_batches = blocks,
    };
}
