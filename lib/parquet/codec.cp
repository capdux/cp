// Source: cp/lib/parquet/codec.cp (Parquet file container, Footer PAR1, and column chunk reader/writer)
const std = @import("std");
import {
    ParquetError,
    ParquetType,
    ParquetCompression,
    ParquetEncoding,
    ColumnSchema,
    ColumnData,
    ParquetFile,
} from "./types.cp";
import { ThriftWriter, ThriftReader, ThriftType } from "./thrift.cp";

const PARQUET_MAGIC = "PAR1";

struct ColumnMetaInfo {
    col_type: ParquetType,
    codec: ParquetCompression,
    num_values: i64,
    data_page_offset: i64,
    col_name: []const u8,
}

struct RowGroupMetaInfo {
    num_rows: i64,
    columns: []ColumnMetaInfo,
}

struct FileMetaInfo {
    num_rows: i64,
    schemas: []ColumnSchema,
    row_groups: []RowGroupMetaInfo,
    created_by: []const u8,
}

struct ParsedSchemaElement {
    name: []u8,
    col_type: ParquetType?,
    repetition_type: i32,
    num_children: i32,
}

struct PageHeaderInfo {
    page_type: i32,
    compressed_size: usize,
    num_values: usize,
    encoding: ParquetEncoding,
}

fn encodeColumnPlain(alloc: std.mem.Allocator, col: *const ColumnData): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    switch (col.col_type) {
        .INT32 => {
            let tmp: [4]u8 = std.mem.zeroes([4]u8);
            for (v of col.int32_vals) {
                std.mem.writeInt(i32, &tmp, v, .little);
                try buf.appendSlice(alloc, &tmp);
            }
        },
        .INT64 => {
            let tmp: [8]u8 = std.mem.zeroes([8]u8);
            for (v of col.int64_vals) {
                std.mem.writeInt(i64, &tmp, v, .little);
                try buf.appendSlice(alloc, &tmp);
            }
        },
        .DOUBLE => {
            let tmp: [8]u8 = std.mem.zeroes([8]u8);
            for (v of col.double_vals) {
                const bits: u64 = @bitCast(v);
                std.mem.writeInt(u64, &tmp, bits, .little);
                try buf.appendSlice(alloc, &tmp);
            }
        },
        .BYTE_ARRAY => {
            let tmp: [4]u8 = std.mem.zeroes([4]u8);
            for (s of col.byte_array_vals) {
                std.mem.writeInt(u32, &tmp, @as(u32, @intCast(s.len)), .little);
                try buf.appendSlice(alloc, &tmp);
                try buf.appendSlice(alloc, s);
            }
        },
        .BOOLEAN => {
            const num_bytes = (col.bool_vals.len + 7) / 8;
            try buf.resize(alloc, num_bytes);
            @memset(buf.items, 0);
            let i: usize = 0;
            while (i < col.bool_vals.len) : (i += 1) {
                if (col.bool_vals[i]) {
                    const byte_idx = i >> 3;
                    const bit_idx: u3 = @intCast(i & 7);
                    buf.items[byte_idx] |= (@as(u8, 1) << bit_idx);
                }
            }
        },
        else => return ParquetError.UnsupportedType,
    }

    return buf.toOwnedSlice(alloc);
}

fn writeDataPageHeader(w: *ThriftWriter, num_values: i32, payload_len: i32): !void {
    try w.writeI32Field(1, 0);
    try w.writeI32Field(2, payload_len);
    try w.writeI32Field(3, payload_len);
    try w.writeFieldHeader(5, .Struct);
    try w.beginStruct();
    try w.writeI32Field(1, num_values);
    try w.writeI32Field(2, 0);
    try w.writeI32Field(3, 3);
    try w.writeI32Field(4, 3);
    try w.endStruct();
    try w.writeFieldStop();
}

pub fn writeParquet(alloc: std.mem.Allocator, columns: []const ColumnData): ![]u8 {
    if (columns.len == 0) {
        return ParquetError.InvalidColumnData;
    }

    const row_count = columns[0].len();
    for (col of columns) {
        if (col.len() != row_count) {
            return ParquetError.InvalidColumnData;
        }
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, PARQUET_MAGIC);

    let col_offsets = try alloc.alloc(i64, columns.len);
    defer alloc.free(col_offsets);
    let col_sizes = try alloc.alloc(i64, columns.len);
    defer alloc.free(col_sizes);

    let total_byte_size: i64 = 0;

    let i: usize = 0;
    while (i < columns.len) : (i += 1) {
        const col = &columns[i];
        col_offsets[i] = @intCast(out.items.len);

        const payload = try encodeColumnPlain(alloc, col);
        defer alloc.free(payload);

        let pw = ThriftWriter.init(alloc);
        defer pw.deinit();
        try writeDataPageHeader(&pw, @as(i32, @intCast(row_count)), @as(i32, @intCast(payload.len)));

        try out.appendSlice(alloc, pw.bytes());
        try out.appendSlice(alloc, payload);

        const chunk_size: i64 = @intCast(pw.bytes().len + payload.len);
        col_sizes[i] = chunk_size;
        total_byte_size += chunk_size;
    }

    let fw = ThriftWriter.init(alloc);
    defer fw.deinit();

    try fw.writeI32Field(1, 1);

    try fw.writeFieldHeader(2, .List);
    try fw.writeListHeader(.Struct, columns.len + 1);
    try fw.beginStruct();
    try fw.writeBinaryField(4, "schema");
    try fw.writeI32Field(5, @as(i32, @intCast(columns.len)));
    try fw.endStruct();

    for (col of columns) {
        try fw.beginStruct();
        try fw.writeI32Field(1, @intFromEnum(col.col_type));
        try fw.writeI32Field(3, 0);
        try fw.writeBinaryField(4, col.name);
        try fw.endStruct();
    }

    try fw.writeI64Field(3, @as(i64, @intCast(row_count)));

    try fw.writeFieldHeader(4, .List);
    try fw.writeListHeader(.Struct, 1);
    try fw.beginStruct();

    try fw.writeFieldHeader(1, .List);
    try fw.writeListHeader(.Struct, columns.len);
    i = 0;
    while (i < columns.len) : (i += 1) {
        const col = &columns[i];
        try fw.beginStruct();
        try fw.writeI64Field(2, col_offsets[i]);
        try fw.writeFieldHeader(3, .Struct);
        try fw.beginStruct();
        try fw.writeI32Field(1, @intFromEnum(col.col_type));
        try fw.writeFieldHeader(2, .List);
        try fw.writeListHeader(.I32, 1);
        try fw.writeZigZagI32(0);
        try fw.writeFieldHeader(3, .List);
        try fw.writeListHeader(.Binary, 1);
        try fw.writeBinary(col.name);
        try fw.writeI32Field(4, 0);
        try fw.writeI64Field(5, @as(i64, @intCast(row_count)));
        try fw.writeI64Field(6, col_sizes[i]);
        try fw.writeI64Field(7, col_sizes[i]);
        try fw.writeI64Field(9, col_offsets[i]);
        try fw.endStruct();
        try fw.endStruct();
    }

    try fw.writeI64Field(2, total_byte_size);
    try fw.writeI64Field(3, @as(i64, @intCast(row_count)));
    try fw.endStruct();

    try fw.writeBinaryField(6, "cp-parquet 1.0");
    try fw.writeFieldStop();

    const footer_bytes = fw.bytes();
    try out.appendSlice(alloc, footer_bytes);

    let len_buf: [4]u8 = std.mem.zeroes([4]u8);
    std.mem.writeInt(u32, &len_buf, @as(u32, @intCast(footer_bytes.len)), .little);
    try out.appendSlice(alloc, &len_buf);
    try out.appendSlice(alloc, PARQUET_MAGIC);

    return out.toOwnedSlice(alloc);
}

fn freeFileMetaInfo(alloc: std.mem.Allocator, meta: *FileMetaInfo) {
    for (s of meta.schemas) {
        if (s.name.len > 0) {
            alloc.free(s.name);
        }
    }
    if (meta.schemas.len > 0) {
        alloc.free(meta.schemas);
    }
    for (rg of meta.row_groups) {
        for (c of rg.columns) {
            if (c.col_name.len > 0) {
                alloc.free(c.col_name);
            }
        }
        if (rg.columns.len > 0) {
            alloc.free(rg.columns);
        }
    }
    if (meta.row_groups.len > 0) {
        alloc.free(meta.row_groups);
    }
    if (meta.created_by.len > 0) {
        alloc.free(meta.created_by);
    }
}

fn unwrapOrEmptyBytes(alloc: std.mem.Allocator, opt: ([]u8)?): ![]u8 {
    if (n := opt) {
        return n;
    }
    return alloc.alloc(u8, 0);
}

fn parseSchemaElement(alloc: std.mem.Allocator, r: *ThriftReader): !ParsedSchemaElement {
    try r.beginStruct();
    defer r.endStruct();

    let name_opt: ([]u8)? = null;
    errdefer {
        if (n := name_opt) {
            alloc.free(n);
        }
    }
    let col_type: ParquetType? = null;
    let repetition_type: i32 = 0;
    let num_children: i32 = 0;

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            1 => {
                if (fh.field_type == .I32) {
                    const t_i32 = try r.readZigZagI32();
                    col_type = try ParquetType.fromI32(t_i32);
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .I32) {
                    repetition_type = try r.readZigZagI32();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            4 => {
                if (fh.field_type == .Binary) {
                    const raw = try r.readBinary();
                    if (old := name_opt) {
                        alloc.free(old);
                    }
                    name_opt = try alloc.dupe(u8, raw);
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            5 => {
                if (fh.field_type == .I32) {
                    num_children = try r.readZigZagI32();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    const final_name = try unwrapOrEmptyBytes(alloc, name_opt);

    return ParsedSchemaElement{
        .name = final_name,
        .col_type = col_type,
        .repetition_type = repetition_type,
        .num_children = num_children,
    };
}

fn parseColumnMetaData(alloc: std.mem.Allocator, r: *ThriftReader): !ColumnMetaInfo {
    try r.beginStruct();
    defer r.endStruct();

    let col_type: ParquetType = .INT32;
    let codec: ParquetCompression = .UNCOMPRESSED;
    let num_values: i64 = 0;
    let data_page_offset: i64 = 0;
    let name_opt: ([]u8)? = null;
    errdefer {
        if (n := name_opt) {
            alloc.free(n);
        }
    }

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            1 => {
                if (fh.field_type == .I32) {
                    col_type = try ParquetType.fromI32(try r.readZigZagI32());
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .List) {
                    const lh = try r.readListHeader();
                    let i: usize = 0;
                    while (i < lh.count) : (i += 1) {
                        if (lh.elem_type == .Binary) {
                            const s = try r.readBinary();
                            if (i == lh.count - 1) {
                                if (old := name_opt) {
                                    alloc.free(old);
                                }
                                name_opt = try alloc.dupe(u8, s);
                            }
                        } else {
                            try r.skipValue(lh.elem_type);
                        }
                    }
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            4 => {
                if (fh.field_type == .I32) {
                    codec = try ParquetCompression.fromI32(try r.readZigZagI32());
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            5 => {
                if (fh.field_type == .I64) {
                    num_values = try r.readZigZagI64();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            9 => {
                if (fh.field_type == .I64) {
                    data_page_offset = try r.readZigZagI64();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    const final_name = try unwrapOrEmptyBytes(alloc, name_opt);

    return ColumnMetaInfo{
        .col_type = col_type,
        .codec = codec,
        .num_values = num_values,
        .data_page_offset = data_page_offset,
        .col_name = final_name,
    };
}

fn parseColumnChunk(alloc: std.mem.Allocator, r: *ThriftReader): !ColumnMetaInfo {
    try r.beginStruct();
    defer r.endStruct();

    let file_offset: i64 = 0;
    let has_meta = false;
    let meta = ColumnMetaInfo{
        .col_type = .INT32,
        .codec = .UNCOMPRESSED,
        .num_values = 0,
        .data_page_offset = 0,
        .col_name = "",
    };
    errdefer {
        if (meta.col_name.len > 0) {
            alloc.free(meta.col_name);
        }
    }

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            2 => {
                if (fh.field_type == .I64) {
                    file_offset = try r.readZigZagI64();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .Struct) {
                    if (has_meta and meta.col_name.len > 0) {
                        alloc.free(meta.col_name);
                    }
                    meta = try parseColumnMetaData(alloc, r);
                    has_meta = true;
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    if (meta.data_page_offset == 0 and file_offset > 0) {
        meta.data_page_offset = file_offset;
    }
    return meta;
}

fn parseRowGroup(alloc: std.mem.Allocator, r: *ThriftReader): !RowGroupMetaInfo {
    try r.beginStruct();
    defer r.endStruct();

    let cols = std.ArrayList(ColumnMetaInfo).empty;
    errdefer {
        for (c of cols.items) {
            if (c.col_name.len > 0) {
                alloc.free(c.col_name);
            }
        }
        cols.deinit(alloc);
    }
    let num_rows: i64 = 0;

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            1 => {
                if (fh.field_type == .List) {
                    const lh = try r.readListHeader();
                    let i: usize = 0;
                    while (i < lh.count) : (i += 1) {
                        const cm = try parseColumnChunk(alloc, r);
                        try cols.append(alloc, cm);
                    }
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .I64) {
                    num_rows = try r.readZigZagI64();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    return RowGroupMetaInfo{
        .num_rows = num_rows,
        .columns = try cols.toOwnedSlice(alloc),
    };
}

fn parseFileMetaData(alloc: std.mem.Allocator, footer_bytes: []const u8): !FileMetaInfo {
    let r = ThriftReader.init(footer_bytes);

    let schemas = std.ArrayList(ColumnSchema).empty;
    errdefer {
        for (s of schemas.items) {
            if (s.name.len > 0) {
                alloc.free(s.name);
            }
        }
        schemas.deinit(alloc);
    }

    let row_groups = std.ArrayList(RowGroupMetaInfo).empty;
    errdefer {
        for (rg of row_groups.items) {
            for (c of rg.columns) {
                if (c.col_name.len > 0) {
                    alloc.free(c.col_name);
                }
            }
            if (rg.columns.len > 0) {
                alloc.free(rg.columns);
            }
        }
        row_groups.deinit(alloc);
    }

    let num_rows: i64 = 0;
    let created_opt: ([]u8)? = null;
    errdefer {
        if (cb := created_opt) {
            alloc.free(cb);
        }
    }

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            2 => {
                if (fh.field_type == .List) {
                    const lh = try r.readListHeader();
                    let i: usize = 0;
                    while (i < lh.count) : (i += 1) {
                        const se = try parseSchemaElement(alloc, &r);
                        if (ct := se.col_type) {
                            try schemas.append(alloc, ColumnSchema{
                                .name = se.name,
                                .col_type = ct,
                                .repetition_type = se.repetition_type,
                            });
                        } else {
                            if (se.name.len > 0) {
                                alloc.free(se.name);
                            }
                        }
                    }
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .I64) {
                    num_rows = try r.readZigZagI64();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            4 => {
                if (fh.field_type == .List) {
                    const lh = try r.readListHeader();
                    let i: usize = 0;
                    while (i < lh.count) : (i += 1) {
                        const rg = try parseRowGroup(alloc, &r);
                        try row_groups.append(alloc, rg);
                    }
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            6 => {
                if (fh.field_type == .Binary) {
                    const raw = try r.readBinary();
                    if (old := created_opt) {
                        alloc.free(old);
                    }
                    created_opt = try alloc.dupe(u8, raw);
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    let final_created: []const u8 = "";
    if (cb := created_opt) {
        final_created = cb;
    }

    return FileMetaInfo{
        .num_rows = num_rows,
        .schemas = try schemas.toOwnedSlice(alloc),
        .row_groups = try row_groups.toOwnedSlice(alloc),
        .created_by = final_created,
    };
}

fn parsePageHeader(r: *ThriftReader): !PageHeaderInfo {
    let page_type: i32 = 0;
    let compressed_size: i32 = 0;
    let num_values: i32 = 0;
    let encoding: ParquetEncoding = .PLAIN;

    while (true) {
        const fh = try r.readFieldHeader();
        if (fh.field_type == .Stop) {
            break;
        }
        switch (fh.field_id) {
            1 => {
                if (fh.field_type == .I32) {
                    page_type = try r.readZigZagI32();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            3 => {
                if (fh.field_type == .I32) {
                    compressed_size = try r.readZigZagI32();
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            5 => {
                if (fh.field_type == .Struct) {
                    try r.beginStruct();
                    defer r.endStruct();
                    while (true) {
                        const dfh = try r.readFieldHeader();
                        if (dfh.field_type == .Stop) {
                            break;
                        }
                        switch (dfh.field_id) {
                            1 => {
                                if (dfh.field_type == .I32) {
                                    num_values = try r.readZigZagI32();
                                } else {
                                    try r.skipValue(dfh.field_type);
                                }
                            },
                            2 => {
                                if (dfh.field_type == .I32) {
                                    encoding = try ParquetEncoding.fromI32(try r.readZigZagI32());
                                } else {
                                    try r.skipValue(dfh.field_type);
                                }
                            },
                            else => try r.skipValue(dfh.field_type),
                        }
                    }
                } else {
                    try r.skipValue(fh.field_type);
                }
            },
            else => try r.skipValue(fh.field_type),
        }
    }

    if (compressed_size < 0 or num_values < 0) {
        return ParquetError.TruncatedData;
    }

    return PageHeaderInfo{
        .page_type = page_type,
        .compressed_size = @intCast(compressed_size),
        .num_values = @intCast(num_values),
        .encoding = encoding,
    };
}

fn decodeColumnPlain(
    alloc: std.mem.Allocator,
    col_type: ParquetType,
    payload: []const u8,
    num_values: usize,
): !ColumnData {
    switch (col_type) {
        .INT32 => {
            if (num_values * 4 > payload.len) {
                return ParquetError.TruncatedData;
            }
            let vals = try alloc.alloc(i32, num_values);
            errdefer alloc.free(vals);
            let i: usize = 0;
            while (i < num_values) : (i += 1) {
                const off = i * 4;
                vals[i] = std.mem.readInt(i32, payload[off .. off + 4][0..4], .little);
            }
            return ColumnData.fromInt32("", vals);
        },
        .INT64 => {
            if (num_values * 8 > payload.len) {
                return ParquetError.TruncatedData;
            }
            let vals = try alloc.alloc(i64, num_values);
            errdefer alloc.free(vals);
            let i: usize = 0;
            while (i < num_values) : (i += 1) {
                const off = i * 8;
                vals[i] = std.mem.readInt(i64, payload[off .. off + 8][0..8], .little);
            }
            return ColumnData.fromInt64("", vals);
        },
        .DOUBLE => {
            if (num_values * 8 > payload.len) {
                return ParquetError.TruncatedData;
            }
            let vals = try alloc.alloc(f64, num_values);
            errdefer alloc.free(vals);
            let i: usize = 0;
            while (i < num_values) : (i += 1) {
                const off = i * 8;
                const bits = std.mem.readInt(u64, payload[off .. off + 8][0..8], .little);
                vals[i] = @bitCast(bits);
            }
            return ColumnData.fromDouble("", vals);
        },
        .BOOLEAN => {
            const req_bytes = (num_values + 7) / 8;
            if (req_bytes > payload.len) {
                return ParquetError.TruncatedData;
            }
            let vals = try alloc.alloc(bool, num_values);
            errdefer alloc.free(vals);
            let i: usize = 0;
            while (i < num_values) : (i += 1) {
                const byte_idx = i >> 3;
                const bit_idx: u3 = @intCast(i & 7);
                vals[i] = ((payload[byte_idx] >> bit_idx) & 1) != 0;
            }
            return ColumnData.fromBool("", vals);
        },
        .BYTE_ARRAY => {
            const ByteSlice = @TypeOf(@as([]const u8, ""));
            let vals = try alloc.alloc(ByteSlice, num_values);
            let filled: usize = 0;
            errdefer {
                let j: usize = 0;
                while (j < filled) : (j += 1) {
                    if (vals[j].len > 0) {
                        alloc.free(vals[j]);
                    }
                }
                alloc.free(vals);
            }
            let pos: usize = 0;
            while (filled < num_values) : (filled += 1) {
                if (4 > payload.len - pos) {
                    return ParquetError.TruncatedData;
                }
                const slen: usize = @intCast(std.mem.readInt(u32, payload[pos .. pos + 4][0..4], .little));
                pos += 4;
                if (slen > payload.len - pos) {
                    return ParquetError.TruncatedData;
                }
                vals[filled] = try alloc.dupe(u8, payload[pos .. pos + slen]);
                pos += slen;
            }
            return ColumnData.fromByteArray("", vals);
        },
        else => return ParquetError.UnsupportedType,
    }
}

fn freeSingleColumnData(alloc: std.mem.Allocator, col: *const ColumnData) {
    if (col.name.len > 0) {
        alloc.free(col.name);
    }
    switch (col.col_type) {
        .INT32 => {
            if (col.int32_vals.len > 0) {
                alloc.free(col.int32_vals);
            }
        },
        .INT64 => {
            if (col.int64_vals.len > 0) {
                alloc.free(col.int64_vals);
            }
        },
        .DOUBLE => {
            if (col.double_vals.len > 0) {
                alloc.free(col.double_vals);
            }
        },
        .BOOLEAN => {
            if (col.bool_vals.len > 0) {
                alloc.free(col.bool_vals);
            }
        },
        .BYTE_ARRAY => {
            for (b of col.byte_array_vals) {
                if (b.len > 0) {
                    alloc.free(b);
                }
            }
            if (col.byte_array_vals.len > 0) {
                alloc.free(col.byte_array_vals);
            }
        },
        else => {},
    }
}

pub fn readParquet(alloc: std.mem.Allocator, data: []const u8): !ParquetFile {
    if (data.len < 12) {
        return ParquetError.TruncatedData;
    }
    if (!std.mem.eql(u8, data[0..4], PARQUET_MAGIC)) {
        return ParquetError.InvalidMagic;
    }
    if (!std.mem.eql(u8, data[data.len - 4 .. data.len], PARQUET_MAGIC)) {
        return ParquetError.InvalidMagic;
    }

    const footer_len: usize = @intCast(std.mem.readInt(u32, data[data.len - 8 .. data.len - 4][0..4], .little));
    if (footer_len == 0 or footer_len > data.len - 12) {
        return ParquetError.InvalidFooterLength;
    }

    const body_end = data.len - 8 - footer_len;
    const footer_bytes = data[body_end .. data.len - 8];

    let meta = try parseFileMetaData(alloc, footer_bytes);
    defer freeFileMetaInfo(alloc, &meta);

    let out_schemas = try alloc.alloc(ColumnSchema, meta.schemas.len);
    let schemas_copied: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < schemas_copied) : (j += 1) {
            if (out_schemas[j].name.len > 0) {
                alloc.free(out_schemas[j].name);
            }
        }
        alloc.free(out_schemas);
    }
    while (schemas_copied < meta.schemas.len) : (schemas_copied += 1) {
        out_schemas[schemas_copied] = ColumnSchema{
            .name = try alloc.dupe(u8, meta.schemas[schemas_copied].name),
            .col_type = meta.schemas[schemas_copied].col_type,
            .repetition_type = meta.schemas[schemas_copied].repetition_type,
        };
    }

    const num_cols = meta.schemas.len;
    let out_cols = try alloc.alloc(ColumnData, num_cols);
    let cols_decoded: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < cols_decoded) : (j += 1) {
            freeSingleColumnData(alloc, &out_cols[j]);
        }
        alloc.free(out_cols);
    }

    if (meta.row_groups.len > 0) {
        const rg = &meta.row_groups[0];
        if (rg.columns.len != num_cols) {
            return ParquetError.InvalidColumnData;
        }
        while (cols_decoded < num_cols) : (cols_decoded += 1) {
            const cm = &rg.columns[cols_decoded];
            if (cm.codec != .UNCOMPRESSED) {
                return ParquetError.UnsupportedCompression;
            }
            if (cm.data_page_offset < 4 or @as(usize, @intCast(cm.data_page_offset)) >= body_end) {
                return ParquetError.TruncatedData;
            }
            const page_start: usize = @intCast(cm.data_page_offset);
            let pr = ThriftReader.init(data[page_start..body_end]);
            const ph = try parsePageHeader(&pr);
            if (ph.encoding != .PLAIN) {
                return ParquetError.UnsupportedEncoding;
            }
            const payload_start = page_start + pr.pos;
            if (ph.compressed_size > body_end - payload_start) {
                return ParquetError.TruncatedData;
            }
            const payload = data[payload_start .. payload_start + ph.compressed_size];
            let decoded = try decodeColumnPlain(alloc, cm.col_type, payload, ph.num_values);
            decoded.name = try alloc.dupe(u8, out_schemas[cols_decoded].name);
            out_cols[cols_decoded] = decoded;
        }
    } else {
        while (cols_decoded < num_cols) : (cols_decoded += 1) {
            out_cols[cols_decoded] = ColumnData{
                .name = try alloc.dupe(u8, out_schemas[cols_decoded].name),
                .col_type = out_schemas[cols_decoded].col_type,
                .int32_vals = &.{},
                .int64_vals = &.{},
                .double_vals = &.{},
                .bool_vals = &.{},
                .byte_array_vals = &.{},
            };
        }
    }

    let created_copy: []const u8 = "";
    if (meta.created_by.len > 0) {
        created_copy = try alloc.dupe(u8, meta.created_by);
    }

    return ParquetFile{
        .num_rows = meta.num_rows,
        .num_row_groups = meta.row_groups.len,
        .schemas = out_schemas,
        .columns = out_cols,
        .created_by = created_copy,
    };
}
