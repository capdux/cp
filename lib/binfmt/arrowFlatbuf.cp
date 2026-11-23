// Source: cp/lib/arrow_ipc/flatbuf.cp (Lightweight deterministic Flatbuffer codec for Arrow IPC)
// Reference-hardened against apache/arrow-rs (Apache-2.0)

const std = @import("std");
import {
    ARROW_MAX_ROWS,
    ARROW_MAX_FIELDS,
    ARROW_MAX_BODY_LEN,
    ArrowError,
    ArrowType,
    ArrowField,
    ArrowSchema,
} from "./arrowTypes.cp";

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

    static fn init(): FbBuilder {
        return FbBuilder{ .buf = std.ArrayList(u8).empty };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    fn offset(): u32 {
        return @intCast(_.buf.items.len);
    }

    fn pushBytesRev(*_, alloc: std.mem.Allocator, data: []const u8): bool ! ArrowError {
        let i: usize = data.len;
        while (i > 0) {
            i -= 1;
            _.buf.append(alloc, data[i]) catch return ArrowError.OutOfMemory;
        }
        return true;
    }

    fn pushU8(*_, alloc: std.mem.Allocator, v: u8): bool ! ArrowError {
        _.buf.append(alloc, v) catch return ArrowError.OutOfMemory;
        return true;
    }

    fn pushU16(*_, alloc: std.mem.Allocator, v: u16): bool ! ArrowError {
        let b: [2]u8 = undefined;
        std.mem.writeInt(u16, &b, v, .little);
        return _.pushBytesRev(alloc, &b);
    }

    fn pushI16(*_, alloc: std.mem.Allocator, v: i16): bool ! ArrowError {
        let b: [2]u8 = undefined;
        std.mem.writeInt(i16, &b, v, .little);
        return _.pushBytesRev(alloc, &b);
    }

    fn pushU32(*_, alloc: std.mem.Allocator, v: u32): bool ! ArrowError {
        let b: [4]u8 = undefined;
        std.mem.writeInt(u32, &b, v, .little);
        return _.pushBytesRev(alloc, &b);
    }

    fn pushI32(*_, alloc: std.mem.Allocator, v: i32): bool ! ArrowError {
        let b: [4]u8 = undefined;
        std.mem.writeInt(i32, &b, v, .little);
        return _.pushBytesRev(alloc, &b);
    }

    fn pushI64(*_, alloc: std.mem.Allocator, v: i64): bool ! ArrowError {
        let b: [8]u8 = undefined;
        std.mem.writeInt(i64, &b, v, .little);
        return _.pushBytesRev(alloc, &b);
    }

    fn pushOffset(*_, alloc: std.mem.Allocator, target: u32): bool ! ArrowError {
        const cur = _.offset() + 4;
        return _.pushU32(alloc, cur - target);
    }

    fn createString(*_, alloc: std.mem.Allocator, s: []const u8): u32 ! ArrowError {
        * = try _.pushU8(alloc, 0);
        * = try _.pushBytesRev(alloc, s);
        * = try _.pushU32(alloc, @intCast(s.len));
        return _.offset();
    }

    fn createOffsetVector(*_, alloc: std.mem.Allocator, targets: []const u32): u32 ! ArrowError {
        let i: usize = targets.len;
        while (i > 0) {
            i -= 1;
            * = try _.pushOffset(alloc, targets[i]);
        }
        * = try _.pushU32(alloc, @intCast(targets.len));
        return _.offset();
    }

    fn finish(*_, alloc: std.mem.Allocator, root_table: u32): ([]u8) ! ArrowError {
        * = try _.pushOffset(alloc, root_table);
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
): u32 ! ArrowError {
    const table_start = table_body_start + 4;
    const obj_size: u16 = @intCast(table_start - table_body_end);
    const vtable_size: u16 = @intCast(4 + 2 * num_fields);
    * = try b.pushI32(alloc, @intCast(vtable_size));
    const final_table_off = b.offset();

    let i: usize = num_fields;
    while (i > 0) {
        i -= 1;
        * = try b.pushU16(alloc, field_offsets_from_end[i]);
    }
    * = try b.pushU16(alloc, obj_size);
    * = try b.pushU16(alloc, vtable_size);
    return final_table_off;
}

fn encodeTypeTable(b: *FbBuilder, alloc: std.mem.Allocator, t: ArrowType): EncodedType ! ArrowError {
    if (t == .Int32 or t == .Int64) {
        const body_end = b.offset();
        const bit_width: i32 = if (t == .Int32) 32 else 64;
        * = try b.pushU8(alloc, 1);
        const f1_off = b.offset();
        * = try b.pushI32(alloc, bit_width);
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
        * = try b.pushI16(alloc, 2);
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
        const empty_slots: []const u16 = [];
        const tbl = try writeTable(b, alloc, 0, empty_slots, body_end, body_start);
        return EncodedType{ .type_id = 6, .off = tbl };
    }
    const body_end = b.offset();
    const body_start = b.offset();
    const empty_slots: []const u16 = [];
    const tbl = try writeTable(b, alloc, 0, empty_slots, body_end, body_start);
    return EncodedType{ .type_id = 5, .off = tbl };
}

fn encodeSchemaTable(b: *FbBuilder, alloc: std.mem.Allocator, schema: *const ArrowSchema): u32 ! ArrowError {
    let field_offs = alloc.alloc(u32, schema.fields.len) catch return ArrowError.OutOfMemory;
    defer alloc.free(field_offs);

    let i: usize = 0;
    while (i < schema.fields.len) {
        const f = schema.fields[i];
        const name_off = try b.createString(alloc, f.name);
        const type_res = try encodeTypeTable(b, alloc, f.arrow_type);

        const body_end = b.offset();
        * = try b.pushOffset(alloc, type_res.off);
        const f3_off = b.offset();
        * = try b.pushU8(alloc, type_res.type_id);
        const f2_off = b.offset();
        * = try b.pushU8(alloc, if (f.nullable) 1 else 0);
        const f1_off = b.offset();
        * = try b.pushOffset(alloc, name_off);
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
    * = try b.pushOffset(alloc, fields_vec);
    const f1_off = b.offset();
    * = try b.pushI16(alloc, 0);
    const f0_off = b.offset();
    const body_start = b.offset();
    const table_start = body_start + 4;
    const slots: [2]u16 = [
        @intCast(table_start - f0_off),
        @intCast(table_start - f1_off),
    ];
    return writeTable(b, alloc, 2, &slots, body_end, body_start);
}

pub fn encodeSchemaMessage(alloc: std.mem.Allocator, schema: *const ArrowSchema): ([]u8) ! ArrowError {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    const schema_off = try encodeSchemaTable(&b, alloc, schema);

    const body_end = b.offset();
    * = try b.pushI64(alloc, 0);
    const f3_off = b.offset();
    * = try b.pushOffset(alloc, schema_off);
    const f2_off = b.offset();
    * = try b.pushU8(alloc, 1);
    const f1_off = b.offset();
    * = try b.pushI16(alloc, 4);
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
): ([]u8) ! ArrowError {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    let i: usize = buffers.len;
    while (i > 0) {
        i -= 1;
        * = try b.pushI64(alloc, buffers[i].length);
        * = try b.pushI64(alloc, buffers[i].offset);
    }
    * = try b.pushU32(alloc, @intCast(buffers.len));
    const buffers_vec = b.offset();

    i = nodes.len;
    while (i > 0) {
        i -= 1;
        * = try b.pushI64(alloc, nodes[i].null_count);
        * = try b.pushI64(alloc, nodes[i].length);
    }
    * = try b.pushU32(alloc, @intCast(nodes.len));
    const nodes_vec = b.offset();

    const rb_body_end = b.offset();
    * = try b.pushOffset(alloc, buffers_vec);
    const rb_f2 = b.offset();
    * = try b.pushOffset(alloc, nodes_vec);
    const rb_f1 = b.offset();
    * = try b.pushI64(alloc, row_count);
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
    * = try b.pushI64(alloc, body_length);
    const f3_off = b.offset();
    * = try b.pushOffset(alloc, rb_off);
    const f2_off = b.offset();
    * = try b.pushU8(alloc, 3);
    const f1_off = b.offset();
    * = try b.pushI16(alloc, 4);
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

pub fn encodeFooter(alloc: std.mem.Allocator, schema: *const ArrowSchema, blocks: []const FbBlock): ([]u8) ! ArrowError {
    let b = FbBuilder.init();
    defer b.deinit(alloc);

    const schema_off = try encodeSchemaTable(&b, alloc, schema);

    let i: usize = blocks.len;
    while (i > 0) {
        i -= 1;
        * = try b.pushI64(alloc, blocks[i].body_len);
        * = try b.pushI32(alloc, 0);
        * = try b.pushI32(alloc, blocks[i].meta_len);
        * = try b.pushI64(alloc, blocks[i].offset);
    }
    * = try b.pushU32(alloc, @intCast(blocks.len));
    const blocks_vec = b.offset();

    const body_end = b.offset();
    * = try b.pushOffset(alloc, blocks_vec);
    const f3_off = b.offset();
    * = try b.pushOffset(alloc, schema_off);
    const f1_off = b.offset();
    * = try b.pushI16(alloc, 4);
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

    fn readU8(pos: usize): u8 ! ArrowError {
        if (pos >= _.data.len) {
            return ArrowError.InvalidFlatbuffer;
        }
        return _.data[pos];
    }

    fn readU16(pos: usize): u16 ! ArrowError {
        if (pos > _.data.len or _.data.len - pos < 2) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(u16, _.data[pos..][0..2], .little);
    }

    fn readU32(pos: usize): u32 ! ArrowError {
        if (pos > _.data.len or _.data.len - pos < 4) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(u32, _.data[pos..][0..4], .little);
    }

    fn readI32(pos: usize): i32 ! ArrowError {
        if (pos > _.data.len or _.data.len - pos < 4) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(i32, _.data[pos..][0..4], .little);
    }

    fn readI64(pos: usize): i64 ! ArrowError {
        if (pos > _.data.len or _.data.len - pos < 8) {
            return ArrowError.InvalidFlatbuffer;
        }
        return std.mem.readInt(i64, _.data[pos..][0..8], .little);
    }

    fn followUOffset(pos: usize): usize ! ArrowError {
        const rel = try _.readU32(pos);
        const rel_u: usize = @as(usize, rel);
        if (rel_u >= _.data.len - pos) {
            return ArrowError.InvalidFlatbuffer;
        }
        return pos + rel_u;
    }

    fn fieldOffset(table_pos: usize, field_idx: usize): (usize?) ! ArrowError {
        const vtable_rel = try _.readI32(table_pos);
        const tpos_i64: i64 = @intCast(table_pos);
        const vpos_i64 = tpos_i64 - @as(i64, vtable_rel);
        if (vpos_i64 < 0) {
            return ArrowError.InvalidFlatbuffer;
        }
        const vpos: usize = @intCast(vpos_i64);
        if (vpos > _.data.len or _.data.len - vpos < 4) {
            return ArrowError.InvalidFlatbuffer;
        }
        const vtable_len = try _.readU16(vpos);
        if (vtable_len < 4 or @as(usize, vtable_len) > _.data.len - vpos) {
            return ArrowError.InvalidFlatbuffer;
        }
        const slot_pos = 4 + field_idx * 2;
        if (slot_pos + 2 > vtable_len) {
            return null;
        }
        const foff = try _.readU16(vpos + slot_pos);
        if (foff == 0) {
            return null;
        }
        const foff_u: usize = @as(usize, foff);
        if (foff_u >= _.data.len - table_pos) {
            return ArrowError.InvalidFlatbuffer;
        }
        return table_pos + foff_u;
    }

    fn readString(str_pos: usize): ([]const u8) ! ArrowError {
        const len = try _.readU32(str_pos);
        const start = str_pos + 4;
        const len_u: usize = @as(usize, len);
        if (start > _.data.len or len_u > _.data.len - start) {
            return ArrowError.InvalidFlatbuffer;
        }
        return _.data[start .. start + len_u];
    }
}

fn decodeSchemaTable(r: *const FbReader, alloc: std.mem.Allocator, schema_pos: usize): ArrowSchema ! ArrowError {
    const fields_slot_opt = r.fieldOffset(schema_pos, 1) catch |e| return e;
    const fields_slot = fields_slot_opt orelse return ArrowError.EmptySchema;
    const vec_pos = try r.followUOffset(fields_slot);
    const vec_len = try r.readU32(vec_pos);
    if (vec_len == 0 or vec_len > ARROW_MAX_FIELDS) {
        return ArrowError.EmptySchema;
    }
    if (@as(usize, vec_len) > (r.data.len - (vec_pos + 4)) / 4) {
        return ArrowError.InvalidFlatbuffer;
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
        const nslot_opt = r.fieldOffset(field_pos, 0) catch |e| return e;
        if (nslot := nslot_opt) {
            const spos = try r.followUOffset(nslot);
            name_slice = try r.readString(spos);
        }
        let nullable: bool = true;
        const null_slot_opt = r.fieldOffset(field_pos, 1) catch |e| return e;
        if (null_slot := null_slot_opt) {
            nullable = (try r.readU8(null_slot)) != 0;
        }
        const tt_opt = r.fieldOffset(field_pos, 2) catch |e| return e;
        const type_type_slot = tt_opt orelse return ArrowError.InvalidFlatbuffer;
        const type_id = try r.readU8(type_type_slot);
        const ttbl_opt = r.fieldOffset(field_pos, 3) catch |e| return e;
        const type_tbl_slot = ttbl_opt orelse return ArrowError.InvalidFlatbuffer;
        const type_tbl_pos = try r.followUOffset(type_tbl_slot);

        let arrow_type: ArrowType = .Int32;
        if (type_id == 2) {
            const bw_opt = r.fieldOffset(type_tbl_pos, 0) catch |e| return e;
            const bw_slot = bw_opt orelse return ArrowError.InvalidFlatbuffer;
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

pub fn decodeMessage(alloc: std.mem.Allocator, bytes: []const u8): FbDecodedMessage ! ArrowError {
    if (bytes.len < 8) {
        return ArrowError.InvalidFlatbuffer;
    }
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);

    const ht_opt = r.fieldOffset(root_pos, 1) catch |e| return e;
    const ht_slot = ht_opt orelse return ArrowError.InvalidFlatbuffer;
    const ht = try r.readU8(ht_slot);
    const hdr_opt = r.fieldOffset(root_pos, 2) catch |e| return e;
    const hdr_slot = hdr_opt orelse return ArrowError.InvalidFlatbuffer;
    const hdr_pos = try r.followUOffset(hdr_slot);

    let body_length: i64 = 0;
    const bl_opt = r.fieldOffset(root_pos, 3) catch |e| return e;
    if (bl_slot := bl_opt) {
        body_length = try r.readI64(bl_slot);
    }
    if (body_length < 0) {
        return ArrowError.BufferOutOfBounds;
    }
    if (@as(usize, @intCast(body_length)) > ARROW_MAX_BODY_LEN) {
        return ArrowError.BodyTooLarge;
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
        const len_opt = r.fieldOffset(hdr_pos, 0) catch |e| return e;
        const len_slot = len_opt orelse return ArrowError.InvalidFlatbuffer;
        const row_count = try r.readI64(len_slot);
        if (row_count < 0) {
            return ArrowError.RowCountMismatch;
        }
        if (@as(usize, @intCast(row_count)) > ARROW_MAX_ROWS) {
            return ArrowError.TooManyRows;
        }

        const nodes_opt = r.fieldOffset(hdr_pos, 1) catch |e| return e;
        const nodes_slot = nodes_opt orelse return ArrowError.InvalidFlatbuffer;
        const nodes_vec = try r.followUOffset(nodes_slot);
        const nodes_len = try r.readU32(nodes_vec);
        if (@as(usize, nodes_len) > ARROW_MAX_FIELDS or @as(usize, nodes_len) > (bytes.len - (nodes_vec + 4)) / 16) {
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

        const bufs_opt = r.fieldOffset(hdr_pos, 2) catch |e| return e;
        const bufs_slot = bufs_opt orelse return ArrowError.InvalidFlatbuffer;
        const bufs_vec = try r.followUOffset(bufs_slot);
        const bufs_len = try r.readU32(bufs_vec);
        if (@as(usize, bufs_len) > (bytes.len - (bufs_vec + 4)) / 16) {
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

pub fn decodeFooter(alloc: std.mem.Allocator, bytes: []const u8): FbDecodedFooter ! ArrowError {
    if (bytes.len < 8) {
        return ArrowError.InvalidFooter;
    }
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);

    const sc_opt = r.fieldOffset(root_pos, 1) catch |e| return e;
    const schema_slot = sc_opt orelse return ArrowError.InvalidFooter;
    const schema_pos = try r.followUOffset(schema_slot);
    let schema = try decodeSchemaTable(&r, alloc, schema_pos);
    errdefer schema.deinit(alloc);

    let blocks: []FbBlock = undefined;
    const rb_opt = r.fieldOffset(root_pos, 3) catch |e| return e;
    if (rb_slot := rb_opt) {
        const vec_pos = try r.followUOffset(rb_slot);
        const vec_len = try r.readU32(vec_pos);
        if (@as(usize, vec_len) > (bytes.len - (vec_pos + 4)) / 24) {
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

/// Zero-allocation Flatbuffer Message validator returning validated `body_length`.
pub fn validateMessageFlatbuf(bytes: []const u8): usize ! ArrowError {
    if (bytes.len < 8) return ArrowError.InvalidFlatbuffer;
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);
    const ht_opt = r.fieldOffset(root_pos, 1) catch |e| return e;
    const ht_slot = ht_opt orelse return ArrowError.InvalidFlatbuffer;
    const ht = try r.readU8(ht_slot);
    const hdr_opt = r.fieldOffset(root_pos, 2) catch |e| return e;
    const hdr_slot = hdr_opt orelse return ArrowError.InvalidFlatbuffer;
    const hdr_pos = try r.followUOffset(hdr_slot);

    let body_length: i64 = 0;
    const bl_opt = r.fieldOffset(root_pos, 3) catch |e| return e;
    if (bl_slot := bl_opt) {
        body_length = try r.readI64(bl_slot);
    }
    if (body_length < 0) return ArrowError.BufferOutOfBounds;
    const body_u: usize = @intCast(body_length);
    if (body_u > ARROW_MAX_BODY_LEN) return ArrowError.BodyTooLarge;

    if (ht == 1) {
        const fs_opt = r.fieldOffset(hdr_pos, 1) catch |e| return e;
        const fields_slot = fs_opt orelse return ArrowError.EmptySchema;
        const vec_pos = try r.followUOffset(fields_slot);
        const vec_len = try r.readU32(vec_pos);
        if (vec_len == 0 or vec_len > ARROW_MAX_FIELDS) return ArrowError.EmptySchema;
        if (@as(usize, vec_len) > (bytes.len - (vec_pos + 4)) / 4) return ArrowError.InvalidFlatbuffer;
        return body_u;
    }
    if (ht == 3) {
        const len_opt = r.fieldOffset(hdr_pos, 0) catch |e| return e;
        const len_slot = len_opt orelse return ArrowError.InvalidFlatbuffer;
        const row_count = try r.readI64(len_slot);
        if (row_count < 0) return ArrowError.RowCountMismatch;
        if (@as(usize, @intCast(row_count)) > ARROW_MAX_ROWS) return ArrowError.TooManyRows;

        const nodes_opt = r.fieldOffset(hdr_pos, 1) catch |e| return e;
        const nodes_slot = nodes_opt orelse return ArrowError.InvalidFlatbuffer;
        const nodes_vec = try r.followUOffset(nodes_slot);
        const nodes_len = try r.readU32(nodes_vec);
        if (@as(usize, nodes_len) > ARROW_MAX_FIELDS or @as(usize, nodes_len) > (bytes.len - (nodes_vec + 4)) / 16) {
            return ArrowError.InvalidFlatbuffer;
        }

        const bufs_opt = r.fieldOffset(hdr_pos, 2) catch |e| return e;
        const bufs_slot = bufs_opt orelse return ArrowError.InvalidFlatbuffer;
        const bufs_vec = try r.followUOffset(bufs_slot);
        const bufs_len = try r.readU32(bufs_vec);
        if (@as(usize, bufs_len) > (bytes.len - (bufs_vec + 4)) / 16) {
            return ArrowError.InvalidFlatbuffer;
        }
        return body_u;
    }
    return ArrowError.InvalidFlatbuffer;
}

/// Zero-allocation Flatbuffer Footer validator verifying schema & all RecordBatch block bounds.
pub fn validateFooterFlatbuf(bytes: []const u8, footer_start: usize): u32 ! ArrowError {
    if (bytes.len < 8) return ArrowError.InvalidFooter;
    const r = FbReader{ .data = bytes };
    const root_pos = try r.followUOffset(0);
    const sc_opt = r.fieldOffset(root_pos, 1) catch |e| return e;
    const schema_slot = sc_opt orelse return ArrowError.InvalidFooter;
    const schema_pos = try r.followUOffset(schema_slot);
    const fs_opt = r.fieldOffset(schema_pos, 1) catch |e| return e;
    const fields_slot = fs_opt orelse return ArrowError.EmptySchema;
    const fvec_pos = try r.followUOffset(fields_slot);
    const fvec_len = try r.readU32(fvec_pos);
    if (fvec_len == 0 or fvec_len > ARROW_MAX_FIELDS) return ArrowError.EmptySchema;
    if (@as(usize, fvec_len) > (bytes.len - (fvec_pos + 4)) / 4) return ArrowError.InvalidFooter;

    const rb_opt = r.fieldOffset(root_pos, 3) catch |e| return e;
    if (rb_slot := rb_opt) {
        const vec_pos = try r.followUOffset(rb_slot);
        const vec_len = try r.readU32(vec_pos);
        if (@as(usize, vec_len) > (bytes.len - (vec_pos + 4)) / 24) {
            return ArrowError.InvalidFooter;
        }
        let i: usize = 0;
        while (i < vec_len) : (i += 1) {
            const p = vec_pos + 4 + i * 24;
            const off = try r.readI64(p);
            const meta_len = try r.readI32(p + 8);
            const body_len = try r.readI64(p + 16);
            if (off < 8 or meta_len < 8 or body_len < 0) return ArrowError.BufferOutOfBounds;
            const off_u: usize = @intCast(off);
            const meta_u: usize = @intCast(meta_len);
            const body_u: usize = @intCast(body_len);
            if (body_u > ARROW_MAX_BODY_LEN) return ArrowError.BodyTooLarge;
            if (off_u > footer_start or meta_u > footer_start - off_u) return ArrowError.BufferOutOfBounds;
            if (body_u > footer_start - off_u - meta_u) return ArrowError.BufferOutOfBounds;
        }
        return vec_len;
    }
    return 0;
}
