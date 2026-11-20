// Source: cp/lib/parquet/types.cp (Parquet core types, errors, schemas and column data)
const std = @import("std");

pub const ParquetError = error{
    InvalidMagic,
    InvalidFooterLength,
    TruncatedData,
    DepthLimitExceeded,
    InvalidThriftType,
    InvalidVarint,
    InvalidRleStream,
    InvalidBitWidth,
    UnsupportedType,
    UnsupportedCompression,
    UnsupportedEncoding,
    InvalidColumnData,
    OutOfMemory,
};

pub const ParquetType = enum(i32) {
    BOOLEAN = 0,
    INT32 = 1,
    INT64 = 2,
    INT96 = 3,
    FLOAT = 4,
    DOUBLE = 5,
    BYTE_ARRAY = 6,
    FIXED_LEN_BYTE_ARRAY = 7,

    pub fn fromI32(v: i32): !ParquetType {
        return switch (v) {
            0 => .BOOLEAN,
            1 => .INT32,
            2 => .INT64,
            3 => .INT96,
            4 => .FLOAT,
            5 => .DOUBLE,
            6 => .BYTE_ARRAY,
            7 => .FIXED_LEN_BYTE_ARRAY,
            else => ParquetError.UnsupportedType,
        };
    }
};

pub const ParquetCompression = enum(i32) {
    UNCOMPRESSED = 0,
    SNAPPY = 1,
    GZIP = 2,
    LZO = 3,
    BROTLI = 4,
    LZ4 = 5,
    ZSTD = 6,

    pub fn fromI32(v: i32): !ParquetCompression {
        return switch (v) {
            0 => .UNCOMPRESSED,
            1 => .SNAPPY,
            2 => .GZIP,
            3 => .LZO,
            4 => .BROTLI,
            5 => .LZ4,
            6 => .ZSTD,
            else => ParquetError.UnsupportedCompression,
        };
    }
};

pub const ParquetEncoding = enum(i32) {
    PLAIN = 0,
    PLAIN_DICTIONARY = 2,
    RLE = 3,
    BIT_PACKED = 4,
    DELTA_BINARY_PACKED = 5,
    RLE_DICTIONARY = 8,

    pub fn fromI32(v: i32): !ParquetEncoding {
        return switch (v) {
            0 => .PLAIN,
            2 => .PLAIN_DICTIONARY,
            3 => .RLE,
            4 => .BIT_PACKED,
            5 => .DELTA_BINARY_PACKED,
            8 => .RLE_DICTIONARY,
            else => ParquetError.UnsupportedEncoding,
        };
    }
};

pub struct ColumnSchema {
    name: []const u8,
    col_type: ParquetType,
    repetition_type: i32,
}

pub struct ColumnData {
    name: []const u8,
    col_type: ParquetType,
    int32_vals: []const i32,
    int64_vals: []const i64,
    double_vals: []const f64,
    bool_vals: []const bool,
    byte_array_vals: []const []const u8,

    pub fn fromInt32(name: []const u8, vals: []const i32): ColumnData {
        return ColumnData{
            .name = name,
            .col_type = .INT32,
            .int32_vals = vals,
            .int64_vals = &.{},
            .double_vals = &.{},
            .bool_vals = &.{},
            .byte_array_vals = &.{},
        };
    }

    pub fn fromInt64(name: []const u8, vals: []const i64): ColumnData {
        return ColumnData{
            .name = name,
            .col_type = .INT64,
            .int32_vals = &.{},
            .int64_vals = vals,
            .double_vals = &.{},
            .bool_vals = &.{},
            .byte_array_vals = &.{},
        };
    }

    pub fn fromDouble(name: []const u8, vals: []const f64): ColumnData {
        return ColumnData{
            .name = name,
            .col_type = .DOUBLE,
            .int32_vals = &.{},
            .int64_vals = &.{},
            .double_vals = vals,
            .bool_vals = &.{},
            .byte_array_vals = &.{},
        };
    }

    pub fn fromBool(name: []const u8, vals: []const bool): ColumnData {
        return ColumnData{
            .name = name,
            .col_type = .BOOLEAN,
            .int32_vals = &.{},
            .int64_vals = &.{},
            .double_vals = &.{},
            .bool_vals = vals,
            .byte_array_vals = &.{},
        };
    }

    pub fn fromByteArray(name: []const u8, vals: []const []const u8): ColumnData {
        return ColumnData{
            .name = name,
            .col_type = .BYTE_ARRAY,
            .int32_vals = &.{},
            .int64_vals = &.{},
            .double_vals = &.{},
            .bool_vals = &.{},
            .byte_array_vals = vals,
        };
    }

    pub fn len(): usize {
        return switch (_.col_type) {
            .INT32 => _.int32_vals.len,
            .INT64 => _.int64_vals.len,
            .DOUBLE => _.double_vals.len,
            .BOOLEAN => _.bool_vals.len,
            .BYTE_ARRAY => _.byte_array_vals.len,
            else => 0,
        };
    }
}

pub struct ParquetFile {
    num_rows: i64,
    num_row_groups: usize,
    schemas: []ColumnSchema,
    columns: []ColumnData,
    created_by: []const u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (s of _.schemas) {
            if (s.name.len > 0) {
                alloc.free(s.name);
            }
        }
        if (_.schemas.len > 0) {
            alloc.free(_.schemas);
        }

        for (col of _.columns) {
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
                    for (item of col.byte_array_vals) {
                        if (item.len > 0) {
                            alloc.free(item);
                        }
                    }
                    if (col.byte_array_vals.len > 0) {
                        alloc.free(col.byte_array_vals);
                    }
                },
                else => {},
            }
        }
        if (_.columns.len > 0) {
            alloc.free(_.columns);
        }
        if (_.created_by.len > 0) {
            alloc.free(_.created_by);
        }
    }
}
