// Source: cp/lib/arrow_ipc/ipc.cp (Arrow IPC Streaming & File Format "ARROW1" codec)
// Reference-hardened against apache/arrow-rs (Apache-2.0)

const std = @import("std");
import {
    ARROW_MAX_ROWS,
    ARROW_MAX_METADATA_LEN,
    ARROW_MAX_BODY_LEN,
    ArrowError,
    ArrowType,
    ArrowField,
    ArrowSchema,
    ArrowColumn,
    ValidityBitmap,
    RecordBatch,
    simdArrowCopyBytes,
} from "./arrowTypes.cp";
import {
    FbNode,
    FbBuffer,
    FbBlock,
    encodeSchemaMessage,
    encodeRecordBatchMessage,
    encodeFooter,
    decodeMessage,
    decodeFooter,
    validateMessageFlatbuf,
    validateFooterFlatbuf,
} from "./arrowFlatbuf.cp";

pub const ARROW_MAGIC: []const u8 = "ARROW1";
pub const ARROW_MAGIC_PADDED: []const u8 = "ARROW1\x00\x00";
pub const IPC_CONTINUATION_TOKEN: u32 = 0xFFFFFFFF;

pub struct ArrowStreamResult {
    pub schema: ArrowSchema,
    pub batches: []RecordBatch,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.schema.deinit(alloc);
        let i: usize = 0;
        while (i < _.batches.len) {
            _.batches[i].deinit(alloc);
            i += 1;
        }
        alloc.free(_.batches);
        _.batches = &.{};
    }
}

fn alignUp8(n: usize): usize {
    return (n + 7) & ~@as(usize, 7);
}

fn appendU32LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): bool ! ArrowError {
    let b: [4]u8 = undefined;
    std.mem.writeInt(u32, &b, v, .little);
    out.appendSlice(alloc, &b) catch return ArrowError.OutOfMemory;
    return true;
}

fn appendI32LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: i32): bool ! ArrowError {
    let b: [4]u8 = undefined;
    std.mem.writeInt(i32, &b, v, .little);
    out.appendSlice(alloc, &b) catch return ArrowError.OutOfMemory;
    return true;
}

fn appendZeroPad(out: *std.ArrayList(u8), alloc: std.mem.Allocator, count: usize): bool ! ArrowError {
    let i: usize = 0;
    while (i < count) {
        out.append(alloc, 0) catch return ArrowError.OutOfMemory;
        i += 1;
    }
    return true;
}

fn writeEncapsulatedMessage(
    out: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    meta_bytes: []const u8,
    body_bytes: []const u8,
): FbBlock ! ArrowError {
    const msg_offset: i64 = @intCast(out.items.len);
    const meta_padded_len = alignUp8(meta_bytes.len);
    const pad_len = meta_padded_len - meta_bytes.len;

    try appendU32LE(out, alloc, IPC_CONTINUATION_TOKEN);
    try appendI32LE(out, alloc, @intCast(meta_padded_len));
    out.appendSlice(alloc, meta_bytes) catch return ArrowError.OutOfMemory;
    try appendZeroPad(out, alloc, pad_len);

    const total_meta_len: i32 = @intCast(8 + meta_padded_len);

    const body_padded_len = alignUp8(body_bytes.len);
    if (body_bytes.len > 0) {
        out.appendSlice(alloc, body_bytes) catch return ArrowError.OutOfMemory;
        try appendZeroPad(out, alloc, body_padded_len - body_bytes.len);
    }

    return FbBlock{
        .offset = msg_offset,
        .meta_len = total_meta_len,
        .body_len = @intCast(body_padded_len),
    };
}

struct EncodedBatchPayload {
    meta: []u8,
    body: []u8,
}

fn encodeRecordBatchPayload(
    alloc: std.mem.Allocator,
    batch: *const RecordBatch,
): EncodedBatchPayload ! ArrowError {
    let nodes = alloc.alloc(FbNode, batch.columns.len) catch return ArrowError.OutOfMemory;
    defer alloc.free(nodes);

    let buffers_list = std.ArrayList(FbBuffer).empty;
    defer buffers_list.deinit(alloc);

    let body = std.ArrayList(u8).empty;
    defer body.deinit(alloc);

    let c: usize = 0;
    while (c < batch.columns.len) {
        const col = &batch.columns[c];
        nodes[c] = FbNode{
            .length = @intCast(col.len),
            .null_count = @intCast(col.null_count),
        };

        if (bm := col.validity) {
            const off: i64 = @intCast(body.items.len);
            const len: i64 = @intCast(bm.bytes.len);
            body.appendSlice(alloc, bm.bytes) catch return ArrowError.OutOfMemory;
            try appendZeroPad(&body, alloc, alignUp8(bm.bytes.len) - bm.bytes.len);
            buffers_list.append(alloc, FbBuffer{ .offset = off, .length = len }) catch return ArrowError.OutOfMemory;
        } else {
            buffers_list.append(alloc, FbBuffer{ .offset = @intCast(body.items.len), .length = 0 }) catch return ArrowError.OutOfMemory;
        }

        if (col.field.arrow_type == .Utf8) {
            const off1: i64 = @intCast(body.items.len);
            const off_bytes_len = col.offsets.len * 4;
            let j: usize = 0;
            while (j < col.offsets.len) {
                try appendI32LE(&body, alloc, col.offsets[j]);
                j += 1;
            }
            try appendZeroPad(&body, alloc, alignUp8(off_bytes_len) - off_bytes_len);
            buffers_list.append(alloc, FbBuffer{ .offset = off1, .length = @intCast(off_bytes_len) }) catch return ArrowError.OutOfMemory;

            const off2: i64 = @intCast(body.items.len);
            body.appendSlice(alloc, col.data) catch return ArrowError.OutOfMemory;
            try appendZeroPad(&body, alloc, alignUp8(col.data.len) - col.data.len);
            buffers_list.append(alloc, FbBuffer{ .offset = off2, .length = @intCast(col.data.len) }) catch return ArrowError.OutOfMemory;
        } else {
            const off1: i64 = @intCast(body.items.len);
            body.appendSlice(alloc, col.data) catch return ArrowError.OutOfMemory;
            try appendZeroPad(&body, alloc, alignUp8(col.data.len) - col.data.len);
            buffers_list.append(alloc, FbBuffer{ .offset = off1, .length = @intCast(col.data.len) }) catch return ArrowError.OutOfMemory;
        }

        c += 1;
    }

    const body_slice = body.toOwnedSlice(alloc) catch return ArrowError.OutOfMemory;
    errdefer alloc.free(body_slice);

    const meta_slice = try encodeRecordBatchMessage(
        alloc,
        @intCast(batch.num_rows),
        @intCast(body_slice.len),
        nodes,
        buffers_list.items,
    );

    return EncodedBatchPayload{
        .meta = meta_slice,
        .body = body_slice,
    };
}

fn sliceBodyBuffer(body: []const u8, buf: FbBuffer): ([]const u8) ! ArrowError {
    if (buf.offset < 0 or buf.length < 0) {
        return ArrowError.BufferOutOfBounds;
    }
    const start: usize = @intCast(buf.offset);
    const len: usize = @intCast(buf.length);
    if (start > body.len or len > body.len - start) {
        return ArrowError.BufferOutOfBounds;
    }
    return body[start .. start + len];
}

fn decodeRecordBatchFromParts(
    alloc: std.mem.Allocator,
    schema: *const ArrowSchema,
    row_count_i64: i64,
    nodes: []const FbNode,
    buffers: []const FbBuffer,
    body: []const u8,
): RecordBatch ! ArrowError {
    if (row_count_i64 < 0) {
        return ArrowError.RowCountMismatch;
    }
    const row_count: usize = @intCast(row_count_i64);
    if (row_count > ARROW_MAX_ROWS) {
        return ArrowError.TooManyRows;
    }
    if (nodes.len != schema.fields.len) {
        return ArrowError.ColumnCountMismatch;
    }

    let cols = alloc.alloc(ArrowColumn, schema.fields.len) catch return ArrowError.OutOfMemory;
    let initialized: usize = 0;
    errdefer {
        let k: usize = 0;
        while (k < initialized) {
            cols[k].deinit(alloc);
            k += 1;
        }
        alloc.free(cols);
    }

    let buf_idx: usize = 0;
    let c: usize = 0;
    while (c < schema.fields.len) {
        const f = schema.fields[c];
        const node = nodes[c];
        if (node.length != row_count_i64 or node.null_count < 0 or node.null_count > row_count_i64) {
            return ArrowError.RowCountMismatch;
        }
        if (buf_idx >= buffers.len) {
            return ArrowError.BufferOutOfBounds;
        }

        const val_buf_desc = buffers[buf_idx];
        buf_idx += 1;
        const val_slice = try sliceBodyBuffer(body, val_buf_desc);

        let bm: ValidityBitmap? = null;
        if (val_buf_desc.length > 0) {
            bm = try ValidityBitmap.fromRawBytes(alloc, val_slice, row_count, @intCast(node.null_count));
        } else if (node.null_count != 0) {
            return ArrowError.RowCountMismatch;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }

        const name_copy = alloc.dupe(u8, f.name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        if (f.arrow_type == .Utf8) {
            if (buf_idx + 1 >= buffers.len) {
                return ArrowError.BufferOutOfBounds;
            }
            const off_slice = try sliceBodyBuffer(body, buffers[buf_idx]);
            const data_slice = try sliceBodyBuffer(body, buffers[buf_idx + 1]);
            buf_idx += 2;

            const needed_offsets = (row_count + 1) * 4;
            if (off_slice.len < needed_offsets) {
                return ArrowError.BufferOutOfBounds;
            }

            let offsets = alloc.alloc(i32, row_count + 1) catch return ArrowError.OutOfMemory;
            errdefer alloc.free(offsets);

            let prev_off: i32 = 0;
            let j: usize = 0;
            while (j < row_count + 1) {
                const off_val = std.mem.readInt(i32, off_slice[j * 4 ..][0..4], .little);
                if (off_val < prev_off or off_val < 0 or @as(usize, @intCast(off_val)) > data_slice.len) {
                    return ArrowError.InvalidOffset;
                }
                offsets[j] = off_val;
                prev_off = off_val;
                j += 1;
            }

            j = 0;
            while (j < row_count) {
                let is_val = true;
                if (b := bm) {
                    is_val = b.isValid(j);
                }
                if (is_val) {
                    const s: usize = @intCast(offsets[j]);
                    const e: usize = @intCast(offsets[j + 1]);
                    if (!std.unicode.utf8ValidateSlice(data_slice[s..e])) {
                        return ArrowError.InvalidUtf8;
                    }
                }
                j += 1;
            }

            let data_copy = alloc.alloc(u8, data_slice.len) catch return ArrowError.OutOfMemory;
            const copied = simdArrowCopyBytes(data_copy, data_slice);
            if (copied != data_slice.len) {
                alloc.free(data_copy);
                return ArrowError.BufferOutOfBounds;
            }
            cols[c] = ArrowColumn{
                .field = ArrowField{
                    .name = name_copy,
                    .arrow_type = f.arrow_type,
                    .nullable = f.nullable,
                },
                .len = row_count,
                .null_count = @intCast(node.null_count),
                .validity = bm,
                .data = data_copy,
                .offsets = offsets,
            };
        } else {
            if (buf_idx >= buffers.len) {
                return ArrowError.BufferOutOfBounds;
            }
            const data_slice = try sliceBodyBuffer(body, buffers[buf_idx]);
            buf_idx += 1;

            let needed_bytes: usize = 0;
            if (f.arrow_type == .Int32) {
                needed_bytes = row_count * 4;
            } else if (f.arrow_type == .Int64 or f.arrow_type == .Float64) {
                needed_bytes = row_count * 8;
            } else if (f.arrow_type == .Bool) {
                needed_bytes = ValidityBitmap.byteLenForBits(row_count);
            }
            if (data_slice.len < needed_bytes) {
                return ArrowError.BufferOutOfBounds;
            }

            let data_copy = alloc.alloc(u8, needed_bytes) catch return ArrowError.OutOfMemory;
            const copied = simdArrowCopyBytes(data_copy, data_slice[0..needed_bytes]);
            if (copied != needed_bytes) {
                alloc.free(data_copy);
                return ArrowError.BufferOutOfBounds;
            }
            cols[c] = ArrowColumn{
                .field = ArrowField{
                    .name = name_copy,
                    .arrow_type = f.arrow_type,
                    .nullable = f.nullable,
                },
                .len = row_count,
                .null_count = @intCast(node.null_count),
                .validity = bm,
                .data = data_copy,
                .offsets = &.{},
            };
        }

        initialized += 1;
        c += 1;
    }

    const rb = try RecordBatch.init(alloc, schema.*, cols);
    alloc.free(cols);
    return rb;
}

pub fn writeStream(alloc: std.mem.Allocator, schema: *const ArrowSchema, batches: []const RecordBatch): ([]u8) ! ArrowError {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    const schema_meta = try encodeSchemaMessage(alloc, schema);
    defer alloc.free(schema_meta);
    try writeEncapsulatedMessage(&out, alloc, schema_meta, &.{});

    let i: usize = 0;
    while (i < batches.len) {
        const enc_b = try encodeRecordBatchPayload(alloc, &batches[i]);
        defer alloc.free(enc_b.meta);
        defer alloc.free(enc_b.body);
        try writeEncapsulatedMessage(&out, alloc, enc_b.meta, enc_b.body);
        i += 1;
    }

    try appendU32LE(&out, alloc, IPC_CONTINUATION_TOKEN);
    try appendU32LE(&out, alloc, 0);

    return out.toOwnedSlice(alloc) catch return ArrowError.OutOfMemory;
}

pub fn readStream(alloc: std.mem.Allocator, bytes: []const u8): ArrowStreamResult ! ArrowError {
    let pos: usize = 0;
    let schema: ArrowSchema? = null;
    errdefer {
        if (*s := schema) {
            s.deinit(alloc);
        }
    }

    let batches = std.ArrayList(RecordBatch).empty;
    errdefer {
        let k: usize = 0;
        while (k < batches.items.len) {
            batches.items[k].deinit(alloc);
            k += 1;
        }
        batches.deinit(alloc);
    }

    while (pos < bytes.len) {
        if (bytes.len - pos < 8) {
            return ArrowError.UnexpectedEof;
        }
        const cont = std.mem.readInt(u32, bytes[pos..][0..4], .little);
        if (cont != IPC_CONTINUATION_TOKEN) {
            return ArrowError.InvalidContinuation;
        }
        const meta_size_i32 = std.mem.readInt(i32, bytes[pos + 4 ..][0..4], .little);
        pos += 8;
        if (meta_size_i32 == 0) {
            break;
        }
        if (meta_size_i32 < 0) {
            return ArrowError.InvalidFlatbuffer;
        }
        const meta_size: usize = @intCast(meta_size_i32);
        if (meta_size > ARROW_MAX_METADATA_LEN) {
            return ArrowError.MetadataTooLarge;
        }
        if (meta_size > bytes.len - pos) {
            return ArrowError.UnexpectedEof;
        }
        const meta_bytes = bytes[pos .. pos + meta_size];
        pos += meta_size;

        let msg = try decodeMessage(alloc, meta_bytes);
        defer msg.deinit(alloc);

        const body_len: usize = @intCast(msg.body_length);
        if (body_len > ARROW_MAX_BODY_LEN) {
            return ArrowError.BodyTooLarge;
        }
        if (body_len > bytes.len - pos) {
            return ArrowError.BufferOutOfBounds;
        }
        const body_bytes = bytes[pos .. pos + body_len];
        const padded_body = alignUp8(body_len);
        if (padded_body <= bytes.len - pos) {
            pos += padded_body;
        } else {
            pos = bytes.len;
        }

        if (msg.header_type == .Schema) {
            if (schema != null) {
                return ArrowError.InvalidFlatbuffer;
            }
            schema = msg.schema;
            msg.schema = null;
        } else if (msg.header_type == .RecordBatch) {
            if (s := schema) {
                const rb = try decodeRecordBatchFromParts(alloc, &s, msg.row_count, msg.nodes, msg.buffers, body_bytes);
                batches.append(alloc, rb) catch return ArrowError.OutOfMemory;
            } else {
                return ArrowError.EmptySchema;
            }
        }
    }

    if (final_schema := schema) {
        const owned_batches = batches.toOwnedSlice(alloc) catch return ArrowError.OutOfMemory;
        return ArrowStreamResult{
            .schema = final_schema,
            .batches = owned_batches,
        };
    }
    return ArrowError.EmptySchema;
}

pub fn writeFile(alloc: std.mem.Allocator, schema: *const ArrowSchema, batches: []const RecordBatch): ([]u8) ! ArrowError {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, ARROW_MAGIC_PADDED) catch return ArrowError.OutOfMemory;

    const schema_meta = try encodeSchemaMessage(alloc, schema);
    defer alloc.free(schema_meta);
    try writeEncapsulatedMessage(&out, alloc, schema_meta, &.{});

    let blocks = alloc.alloc(FbBlock, batches.len) catch return ArrowError.OutOfMemory;
    defer alloc.free(blocks);

    let i: usize = 0;
    while (i < batches.len) {
        const enc_b = try encodeRecordBatchPayload(alloc, &batches[i]);
        defer alloc.free(enc_b.meta);
        defer alloc.free(enc_b.body);
        blocks[i] = try writeEncapsulatedMessage(&out, alloc, enc_b.meta, enc_b.body);
        i += 1;
    }

    try appendU32LE(&out, alloc, IPC_CONTINUATION_TOKEN);
    try appendU32LE(&out, alloc, 0);

    const footer_bytes = try encodeFooter(alloc, schema, blocks);
    defer alloc.free(footer_bytes);

    out.appendSlice(alloc, footer_bytes) catch return ArrowError.OutOfMemory;
    try appendI32LE(&out, alloc, @intCast(footer_bytes.len));
    out.appendSlice(alloc, ARROW_MAGIC) catch return ArrowError.OutOfMemory;

    return out.toOwnedSlice(alloc) catch return ArrowError.OutOfMemory;
}

pub fn readFile(alloc: std.mem.Allocator, bytes: []const u8): ArrowStreamResult ! ArrowError {
    if (bytes.len < 18) {
        return ArrowError.InvalidMagic;
    }
    if (!std.mem.eql(u8, bytes[0..8], ARROW_MAGIC_PADDED)) {
        return ArrowError.InvalidMagic;
    }
    if (!std.mem.eql(u8, bytes[bytes.len - 6 ..], ARROW_MAGIC)) {
        return ArrowError.InvalidMagic;
    }

    const footer_len_i32 = std.mem.readInt(i32, bytes[bytes.len - 10 ..][0..4], .little);
    if (footer_len_i32 <= 0) {
        return ArrowError.InvalidFooter;
    }
    const footer_len: usize = @intCast(footer_len_i32);
    if (footer_len > ARROW_MAX_METADATA_LEN) {
        return ArrowError.MetadataTooLarge;
    }
    if (footer_len > bytes.len - 18) {
        return ArrowError.InvalidFooter;
    }
    const footer_start = bytes.len - 10 - footer_len;
    const footer_bytes = bytes[footer_start .. footer_start + footer_len];

    let footer = try decodeFooter(alloc, footer_bytes);
    defer footer.deinit(alloc);

    let batches = alloc.alloc(RecordBatch, footer.record_batches.len) catch return ArrowError.OutOfMemory;
    let initialized: usize = 0;
    errdefer {
        let k: usize = 0;
        while (k < initialized) {
            batches[k].deinit(alloc);
            k += 1;
        }
        alloc.free(batches);
    }

    let i: usize = 0;
    while (i < footer.record_batches.len) {
        const blk = footer.record_batches[i];
        if (blk.offset < 8 or blk.meta_len < 8 or blk.body_len < 0) {
            return ArrowError.BufferOutOfBounds;
        }
        const blk_off: usize = @intCast(blk.offset);
        const meta_len: usize = @intCast(blk.meta_len);
        const body_len: usize = @intCast(blk.body_len);
        if (meta_len > ARROW_MAX_METADATA_LEN) {
            return ArrowError.MetadataTooLarge;
        }
        if (body_len > ARROW_MAX_BODY_LEN) {
            return ArrowError.BodyTooLarge;
        }
        if (blk_off > footer_start or meta_len > footer_start - blk_off) {
            return ArrowError.BufferOutOfBounds;
        }
        if (body_len > footer_start - blk_off - meta_len) {
            return ArrowError.BufferOutOfBounds;
        }

        const cont = std.mem.readInt(u32, bytes[blk_off..][0..4], .little);
        if (cont != IPC_CONTINUATION_TOKEN) {
            return ArrowError.InvalidContinuation;
        }
        const fb_len_i32 = std.mem.readInt(i32, bytes[blk_off + 4 ..][0..4], .little);
        if (fb_len_i32 <= 0 or @as(usize, @intCast(fb_len_i32)) > meta_len - 8) {
            return ArrowError.InvalidFlatbuffer;
        }
        const fb_bytes = bytes[blk_off + 8 .. blk_off + 8 + @as(usize, @intCast(fb_len_i32))];
        let msg = try decodeMessage(alloc, fb_bytes);
        defer msg.deinit(alloc);

        if (msg.header_type != .RecordBatch or msg.body_length > blk.body_len) {
            return ArrowError.InvalidFlatbuffer;
        }
        const body_start = blk_off + meta_len;
        const body_bytes = bytes[body_start .. body_start + @as(usize, @intCast(msg.body_length))];

        batches[i] = try decodeRecordBatchFromParts(
            alloc,
            &footer.schema,
            msg.row_count,
            msg.nodes,
            msg.buffers,
            body_bytes,
        );
        initialized += 1;
        i += 1;
    }

    const schema_copy = try footer.schema.clone(alloc);
    return ArrowStreamResult{
        .schema = schema_copy,
        .batches = batches,
    };
}

/// Zero-allocation structural validator for Apache Arrow IPC Stream format.
/// Verifies continuation tokens (`0xFFFFFFFF`), EOS (`0x00000000`), 8-byte alignment,
/// metadata/body limits, and Flatbuffer message bounds; returns count of validated messages.
pub fn validateArrowStream(bytes: []const u8): u32 ! ArrowError {
    if (bytes.len < 8) return ArrowError.UnexpectedEof;
    let pos: usize = 0;
    let msg_count: u32 = 0;

    while (pos < bytes.len) {
        if (bytes.len - pos < 8) return ArrowError.UnexpectedEof;
        const cont = std.mem.readInt(u32, bytes[pos..][0..4], .little);
        if (cont != IPC_CONTINUATION_TOKEN) return ArrowError.InvalidContinuation;
        const meta_size_i32 = std.mem.readInt(i32, bytes[pos + 4 ..][0..4], .little);
        pos += 8;
        if (meta_size_i32 == 0) {
            break;
        }
        if (meta_size_i32 < 0) return ArrowError.InvalidFlatbuffer;
        const meta_size: usize = @intCast(meta_size_i32);
        if (meta_size > ARROW_MAX_METADATA_LEN) return ArrowError.MetadataTooLarge;
        if (meta_size > bytes.len - pos) return ArrowError.UnexpectedEof;
        const meta_bytes = bytes[pos .. pos + meta_size];
        pos += meta_size;

        const body_len = try validateMessageFlatbuf(meta_bytes);
        if (body_len > bytes.len - pos) return ArrowError.BufferOutOfBounds;
        const padded_body = alignUp8(body_len);
        if (padded_body <= bytes.len - pos) {
            pos += padded_body;
        } else {
            pos = bytes.len;
        }
        msg_count += 1;
    }

    if (msg_count == 0) return ArrowError.EmptySchema;
    return msg_count;
}

/// Zero-allocation structural validator for Apache Arrow IPC File ("ARROW1") format.
/// Verifies 8B header magic `ARROW1\0\0`, 6B trailer magic `ARROW1`, overflow-safe footer size,
/// Footer Flatbuffer schema, and all RecordBatch block bounds; returns RecordBatch block count.
pub fn validateArrowFile(bytes: []const u8): u32 ! ArrowError {
    if (bytes.len < 18) return ArrowError.InvalidMagic;
    if (!std.mem.eql(u8, bytes[0..8], ARROW_MAGIC_PADDED)) return ArrowError.InvalidMagic;
    if (!std.mem.eql(u8, bytes[bytes.len - 6 ..], ARROW_MAGIC)) return ArrowError.InvalidMagic;

    const footer_len_i32 = std.mem.readInt(i32, bytes[bytes.len - 10 ..][0..4], .little);
    if (footer_len_i32 <= 0) return ArrowError.InvalidFooter;
    const footer_len: usize = @intCast(footer_len_i32);
    if (footer_len > ARROW_MAX_METADATA_LEN) return ArrowError.MetadataTooLarge;
    if (footer_len > bytes.len - 18) return ArrowError.InvalidFooter;

    const footer_start = bytes.len - 10 - footer_len;
    const footer_bytes = bytes[footer_start .. footer_start + footer_len];
    return validateFooterFlatbuf(footer_bytes, footer_start);
}
