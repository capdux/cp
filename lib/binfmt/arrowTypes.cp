// Source: cp/lib/arrow_ipc/types.cp (Arrow columnar data model & validity bitmap)
// Reference-hardened against apache/arrow-rs (Apache-2.0)

const std = @import("std");

type ArrowVec16 = @Vector(16, u8);

/// Resource protection caps aligned with `apache/arrow-rs` IPC bounds.
pub const ARROW_MAX_ROWS: usize = 16 * 1024 * 1024;
pub const ARROW_MAX_FIELDS: usize = 65536;
pub const ARROW_MAX_METADATA_LEN: usize = 64 * 1024 * 1024;
pub const ARROW_MAX_BODY_LEN: usize = 512 * 1024 * 1024;

pub err ArrowError {
    OutOfMemory,
    InvalidMagic,
    InvalidFooter,
    InvalidFlatbuffer,
    InvalidContinuation,
    UnexpectedEof,
    RowCountMismatch,
    ColumnCountMismatch,
    TypeMismatch,
    IndexOutOfBounds,
    InvalidOffset,
    BufferOutOfBounds,
    InvalidUtf8,
    EmptySchema,
    MetadataTooLarge,
    BodyTooLarge,
    TooManyRows,
}

/// 64-bit word-parallel `@popCount` valid-bit counter for Arrow validity bitmaps.
pub fn popcountValidBits(bytes: []const u8, bit_len: usize): usize {
    if (bit_len == 0) return 0;
    const full_bytes = bit_len >> 3;
    const rem_bits = bit_len & 7;
    let valid_count: usize = 0;
    let i: usize = 0;

    while (i + 8 <= full_bytes) {
        const word = std.mem.readInt(u64, bytes[i..][0..8], .little);
        valid_count += @as(usize, @popCount(word));
        i += 8;
    }
    while (i < full_bytes) : (i += 1) {
        valid_count += @as(usize, @popCount(bytes[i]));
    }
    if (rem_bits > 0) {
        const sh: u3 = @intCast(rem_bits);
        const mask: u8 = (@as(u8, 1) << sh) - 1;
        valid_count += @as(usize, @popCount(bytes[full_bytes] & mask));
    }
    return valid_count;
}

/// 64-bit word-parallel `@popCount` null-bit counter for Arrow validity bitmaps.
pub fn popcountNullBits(bytes: []const u8, bit_len: usize): usize {
    const valid = popcountValidBits(bytes, bit_len);
    return bit_len - valid;
}

/// 16-lane `@Vector(16, u8)` SIMD buffer copy for Arrow columnar payloads.
pub fn simdArrowCopyBytes(dst: []u8, src: []const u8): usize {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const chunk: ArrowVec16 = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

pub enum ArrowType {
    Int32,
    Int64,
    Float64,
    Bool,
    Utf8,
}

pub struct ArrowField {
    pub name: []const u8,
    pub arrow_type: ArrowType,
    pub nullable: bool,

    pub static fn init(name: []const u8, arrow_type: ArrowType, nullable: bool): ArrowField {
        return ArrowField{
            .name = name,
            .arrow_type = arrow_type,
            .nullable = nullable,
        };
    }
}

pub struct ArrowSchema {
    pub fields: []ArrowField,

    pub static fn init(alloc: std.mem.Allocator, fields: []const ArrowField): ArrowSchema ! ArrowError {
        if (fields.len == 0) {
            return ArrowError.EmptySchema;
        }
        if (fields.len > ARROW_MAX_FIELDS) {
            return ArrowError.ColumnCountMismatch;
        }
        let cloned = alloc.alloc(ArrowField, fields.len) catch return ArrowError.OutOfMemory;
        let initialized: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < initialized) {
                alloc.free(cloned[k].name);
                k += 1;
            }
            alloc.free(cloned);
        }
        let i: usize = 0;
        while (i < fields.len) {
            const name_copy = alloc.dupe(u8, fields[i].name) catch return ArrowError.OutOfMemory;
            cloned[i] = ArrowField{
                .name = name_copy,
                .arrow_type = fields[i].arrow_type,
                .nullable = fields[i].nullable,
            };
            initialized += 1;
            i += 1;
        }
        return ArrowSchema{ .fields = cloned };
    }

    pub fn clone(alloc: std.mem.Allocator): ArrowSchema ! ArrowError {
        return ArrowSchema.init(alloc, _.fields);
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.fields.len) {
            alloc.free(_.fields[i].name);
            i += 1;
        }
        alloc.free(_.fields);
        _.fields = &.{};
    }
}

pub struct ValidityBitmap {
    pub bytes: []u8,
    pub bit_len: usize,
    pub null_count: usize,

    pub static fn byteLenForBits(bit_len: usize): usize {
        return (bit_len + 7) / 8;
    }

    pub static fn initAllValid(alloc: std.mem.Allocator, bit_len: usize): ValidityBitmap ! ArrowError {
        const nbytes = ValidityBitmap.byteLenForBits(bit_len);
        let buf = alloc.alloc(u8, nbytes) catch return ArrowError.OutOfMemory;
        @memset(buf, 0xFF);
        return ValidityBitmap{
            .bytes = buf,
            .bit_len = bit_len,
            .null_count = 0,
        };
    }

    pub static fn initFromBools(alloc: std.mem.Allocator, valid: []const bool): ValidityBitmap ! ArrowError {
        const nbytes = ValidityBitmap.byteLenForBits(valid.len);
        let buf = alloc.alloc(u8, nbytes) catch return ArrowError.OutOfMemory;
        @memset(buf, 0);
        let nulls: usize = 0;
        let i: usize = 0;
        while (i < valid.len) {
            if (valid[i]) {
                const sh: u3 = @intCast(i & 7);
                buf[i >> 3] |= (@as(u8, 1) << sh);
            } else {
                nulls += 1;
            }
            i += 1;
        }
        return ValidityBitmap{
            .bytes = buf,
            .bit_len = valid.len,
            .null_count = nulls,
        };
    }

    pub static fn fromRawBytes(alloc: std.mem.Allocator, raw: []const u8, bit_len: usize, expected_nulls: usize?): ValidityBitmap ! ArrowError {
        const needed = ValidityBitmap.byteLenForBits(bit_len);
        if (raw.len < needed) {
            return ArrowError.BufferOutOfBounds;
        }
        let buf = alloc.alloc(u8, needed) catch return ArrowError.OutOfMemory;
        const copied = simdArrowCopyBytes(buf, raw[0..needed]);
        if (copied != needed) {
            alloc.free(buf);
            return ArrowError.BufferOutOfBounds;
        }
        const nulls = popcountNullBits(buf, bit_len);
        if (exp := expected_nulls) {
            if (exp != nulls) {
                alloc.free(buf);
                return ArrowError.RowCountMismatch;
            }
        }
        return ValidityBitmap{
            .bytes = buf,
            .bit_len = bit_len,
            .null_count = nulls,
        };
    }

    pub fn isValid(idx: usize): bool {
        if (idx >= _.bit_len) {
            return false;
        }
        const sh: u3 = @intCast(idx & 7);
        return ((_.bytes[idx >> 3] >> sh) & 1) != 0;
    }

    pub fn setValid(*_, idx: usize, valid: bool): bool ! ArrowError {
        if (idx >= _.bit_len) {
            return ArrowError.IndexOutOfBounds;
        }
        const was_valid = _.isValid(idx);
        const sh: u3 = @intCast(idx & 7);
        const mask: u8 = @as(u8, 1) << sh;
        if (valid) {
            _.bytes[idx >> 3] |= mask;
            if (!was_valid and _.null_count > 0) {
                _.null_count -= 1;
            }
        } else {
            _.bytes[idx >> 3] &= ~mask;
            if (was_valid) {
                _.null_count += 1;
            }
        }
        return true;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.bytes);
        _.bytes = &.{};
        _.bit_len = 0;
        _.null_count = 0;
    }
}

pub struct ArrowColumn {
    pub field: ArrowField,
    pub len: usize,
    pub null_count: usize,
    pub validity: ValidityBitmap?,
    pub data: []u8,
    pub offsets: []i32,

    pub static fn fromInt32(alloc: std.mem.Allocator, name: []const u8, values: []const i32, valid: ([]const bool)?): ArrowColumn ! ArrowError {
        let bm: ValidityBitmap? = null;
        let nulls: usize = 0;
        if (v := valid) {
            if (v.len != values.len) {
                return ArrowError.RowCountMismatch;
            }
            const b = try ValidityBitmap.initFromBools(alloc, v);
            nulls = b.null_count;
            bm = b;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }
        const name_copy = alloc.dupe(u8, name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        let data_buf = alloc.alloc(u8, values.len * 4) catch return ArrowError.OutOfMemory;
        let i: usize = 0;
        while (i < values.len) {
            std.mem.writeInt(i32, data_buf[i * 4 ..][0..4], values[i], .little);
            i += 1;
        }
        return ArrowColumn{
            .field = ArrowField.init(name_copy, .Int32, valid != null),
            .len = values.len,
            .null_count = nulls,
            .validity = bm,
            .data = data_buf,
            .offsets = &.{},
        };
    }

    pub static fn fromInt64(alloc: std.mem.Allocator, name: []const u8, values: []const i64, valid: ([]const bool)?): ArrowColumn ! ArrowError {
        let bm: ValidityBitmap? = null;
        let nulls: usize = 0;
        if (v := valid) {
            if (v.len != values.len) {
                return ArrowError.RowCountMismatch;
            }
            const b = try ValidityBitmap.initFromBools(alloc, v);
            nulls = b.null_count;
            bm = b;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }
        const name_copy = alloc.dupe(u8, name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        let data_buf = alloc.alloc(u8, values.len * 8) catch return ArrowError.OutOfMemory;
        let i: usize = 0;
        while (i < values.len) {
            std.mem.writeInt(i64, data_buf[i * 8 ..][0..8], values[i], .little);
            i += 1;
        }
        return ArrowColumn{
            .field = ArrowField.init(name_copy, .Int64, valid != null),
            .len = values.len,
            .null_count = nulls,
            .validity = bm,
            .data = data_buf,
            .offsets = &.{},
        };
    }

    pub static fn fromFloat64(alloc: std.mem.Allocator, name: []const u8, values: []const f64, valid: ([]const bool)?): ArrowColumn ! ArrowError {
        let bm: ValidityBitmap? = null;
        let nulls: usize = 0;
        if (v := valid) {
            if (v.len != values.len) {
                return ArrowError.RowCountMismatch;
            }
            const b = try ValidityBitmap.initFromBools(alloc, v);
            nulls = b.null_count;
            bm = b;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }
        const name_copy = alloc.dupe(u8, name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        let data_buf = alloc.alloc(u8, values.len * 8) catch return ArrowError.OutOfMemory;
        let i: usize = 0;
        while (i < values.len) {
            const bits: u64 = @bitCast(values[i]);
            std.mem.writeInt(u64, data_buf[i * 8 ..][0..8], bits, .little);
            i += 1;
        }
        return ArrowColumn{
            .field = ArrowField.init(name_copy, .Float64, valid != null),
            .len = values.len,
            .null_count = nulls,
            .validity = bm,
            .data = data_buf,
            .offsets = &.{},
        };
    }

    pub static fn fromBool(alloc: std.mem.Allocator, name: []const u8, values: []const bool, valid: ([]const bool)?): ArrowColumn ! ArrowError {
        let bm: ValidityBitmap? = null;
        let nulls: usize = 0;
        if (v := valid) {
            if (v.len != values.len) {
                return ArrowError.RowCountMismatch;
            }
            const b = try ValidityBitmap.initFromBools(alloc, v);
            nulls = b.null_count;
            bm = b;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }
        const name_copy = alloc.dupe(u8, name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        const nbytes = ValidityBitmap.byteLenForBits(values.len);
        let data_buf = alloc.alloc(u8, nbytes) catch return ArrowError.OutOfMemory;
        @memset(data_buf, 0);
        let i: usize = 0;
        while (i < values.len) {
            if (values[i]) {
                const sh: u3 = @intCast(i & 7);
                data_buf[i >> 3] |= (@as(u8, 1) << sh);
            }
            i += 1;
        }
        return ArrowColumn{
            .field = ArrowField.init(name_copy, .Bool, valid != null),
            .len = values.len,
            .null_count = nulls,
            .validity = bm,
            .data = data_buf,
            .offsets = &.{},
        };
    }

    pub static fn fromUtf8(alloc: std.mem.Allocator, name: []const u8, values: []const []const u8, valid: ([]const bool)?): ArrowColumn ! ArrowError {
        let bm: ValidityBitmap? = null;
        let nulls: usize = 0;
        if (v := valid) {
            if (v.len != values.len) {
                return ArrowError.RowCountMismatch;
            }
            const b = try ValidityBitmap.initFromBools(alloc, v);
            nulls = b.null_count;
            bm = b;
        }
        errdefer {
            if (*b := bm) {
                b.deinit(alloc);
            }
        }
        const name_copy = alloc.dupe(u8, name) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(name_copy);

        let offsets = alloc.alloc(i32, values.len + 1) catch return ArrowError.OutOfMemory;
        errdefer alloc.free(offsets);

        let total_bytes: usize = 0;
        let i: usize = 0;
        offsets[0] = 0;
        while (i < values.len) {
            let is_val = true;
            if (b := bm) {
                is_val = b.isValid(i);
            }
            if (is_val) {
                if (!std.unicode.utf8ValidateSlice(values[i])) {
                    return ArrowError.InvalidUtf8;
                }
                total_bytes += values[i].len;
            }
            if (total_bytes > @as(usize, std.math.maxInt(i32))) {
                return ArrowError.InvalidOffset;
            }
            offsets[i + 1] = @intCast(total_bytes);
            i += 1;
        }

        let data_buf = alloc.alloc(u8, total_bytes) catch return ArrowError.OutOfMemory;
        let pos: usize = 0;
        i = 0;
        while (i < values.len) {
            let is_val = true;
            if (b := bm) {
                is_val = b.isValid(i);
            }
            if (is_val and values[i].len > 0) {
                const copied = simdArrowCopyBytes(data_buf[pos .. pos + values[i].len], values[i]);
                pos += copied;
            }
            i += 1;
        }

        return ArrowColumn{
            .field = ArrowField.init(name_copy, .Utf8, valid != null),
            .len = values.len,
            .null_count = nulls,
            .validity = bm,
            .data = data_buf,
            .offsets = offsets,
        };
    }

    pub fn isNull(idx: usize): bool ! ArrowError {
        if (idx >= _.len) {
            return ArrowError.IndexOutOfBounds;
        }
        if (bm := _.validity) {
            return !bm.isValid(idx);
        }
        return false;
    }

    pub fn getInt32(idx: usize): (i32?) ! ArrowError {
        if (_.field.arrow_type != .Int32) {
            return ArrowError.TypeMismatch;
        }
        if (try _.isNull(idx)) {
            return null;
        }
        return std.mem.readInt(i32, _.data[idx * 4 ..][0..4], .little);
    }

    pub fn getInt64(idx: usize): (i64?) ! ArrowError {
        if (_.field.arrow_type != .Int64) {
            return ArrowError.TypeMismatch;
        }
        if (try _.isNull(idx)) {
            return null;
        }
        return std.mem.readInt(i64, _.data[idx * 8 ..][0..8], .little);
    }

    pub fn getFloat64(idx: usize): (f64?) ! ArrowError {
        if (_.field.arrow_type != .Float64) {
            return ArrowError.TypeMismatch;
        }
        if (try _.isNull(idx)) {
            return null;
        }
        const bits = std.mem.readInt(u64, _.data[idx * 8 ..][0..8], .little);
        return @bitCast(bits);
    }

    pub fn getBool(idx: usize): (bool?) ! ArrowError {
        if (_.field.arrow_type != .Bool) {
            return ArrowError.TypeMismatch;
        }
        if (try _.isNull(idx)) {
            return null;
        }
        const sh: u3 = @intCast(idx & 7);
        return ((_.data[idx >> 3] >> sh) & 1) != 0;
    }

    pub fn getUtf8(idx: usize): (([]const u8)?) ! ArrowError {
        if (_.field.arrow_type != .Utf8) {
            return ArrowError.TypeMismatch;
        }
        if (try _.isNull(idx)) {
            return null;
        }
        if (idx + 1 >= _.offsets.len) {
            return ArrowError.InvalidOffset;
        }
        const start = _.offsets[idx];
        const end = _.offsets[idx + 1];
        if (start < 0 or end < start or @as(usize, @intCast(end)) > _.data.len) {
            return ArrowError.InvalidOffset;
        }
        return _.data[@intCast(start)..@intCast(end)];
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.field.name);
        if (*bm := _.validity) {
            bm.deinit(alloc);
        }
        alloc.free(_.data);
        if (_.offsets.len > 0) {
            alloc.free(_.offsets);
        }
        _.data = &.{};
        _.offsets = &.{};
        _.len = 0;
    }
}

pub struct RecordBatch {
    pub schema: ArrowSchema,
    pub columns: []ArrowColumn,
    pub num_rows: usize,

    pub static fn init(alloc: std.mem.Allocator, schema: ArrowSchema, columns: []ArrowColumn): RecordBatch ! ArrowError {
        if (schema.fields.len != columns.len) {
            return ArrowError.ColumnCountMismatch;
        }
        let expected_rows: usize = 0;
        if (columns.len > 0) {
            expected_rows = columns[0].len;
        }
        let i: usize = 0;
        while (i < columns.len) {
            if (columns[i].len != expected_rows) {
                return ArrowError.RowCountMismatch;
            }
            if (columns[i].field.arrow_type != schema.fields[i].arrow_type) {
                return ArrowError.TypeMismatch;
            }
            if (bm := columns[i].validity) {
                if (bm.bit_len != expected_rows or bm.null_count != columns[i].null_count) {
                    return ArrowError.RowCountMismatch;
                }
            } else {
                if (columns[i].null_count != 0) {
                    return ArrowError.RowCountMismatch;
                }
            }
            i += 1;
        }
        const schema_copy = try schema.clone(alloc);
        errdefer {
            let sc = schema_copy;
            sc.deinit(alloc);
        }
        const cols_copy = alloc.alloc(ArrowColumn, columns.len) catch return ArrowError.OutOfMemory;
        @memcpy(cols_copy, columns);

        return RecordBatch{
            .schema = schema_copy,
            .columns = cols_copy,
            .num_rows = expected_rows,
        };
    }

    pub fn numRows(): usize {
        return _.num_rows;
    }

    pub fn numCols(): usize {
        return _.columns.len;
    }

    pub fn column(idx: usize): (*const ArrowColumn) ! ArrowError {
        if (idx >= _.columns.len) {
            return ArrowError.IndexOutOfBounds;
        }
        return &_.columns[idx];
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.schema.deinit(alloc);
        let i: usize = 0;
        while (i < _.columns.len) {
            _.columns[i].deinit(alloc);
            i += 1;
        }
        alloc.free(_.columns);
        _.columns = &.{};
        _.num_rows = 0;
    }
}
