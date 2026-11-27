// Source: cp/lib/gguf/parser.cp (GGUF v2/v3 binary parser with bounds and bomb protection)
const std = @import("std");
const Allocator = std.mem.Allocator;
const types = @import("./types.cp");

const GGUF_MAGIC = types.GGUF_MAGIC;
const GGUF_VERSION_2 = types.GGUF_VERSION_2;
const GGUF_VERSION_3 = types.GGUF_VERSION_3;
const GGUF_DEFAULT_ALIGNMENT = types.GGUF_DEFAULT_ALIGNMENT;
const GGUF_MAX_DIMS = types.GGUF_MAX_DIMS;
const GGUF_MAX_ARRAY_LEN = types.GGUF_MAX_ARRAY_LEN;
const GGUF_MAX_STRING_LEN = types.GGUF_MAX_STRING_LEN;
const GGUF_MAX_KV_COUNT = types.GGUF_MAX_KV_COUNT;
const GGUF_MAX_TENSOR_COUNT = types.GGUF_MAX_TENSOR_COUNT;
const GGUF_MAX_NESTING_DEPTH = types.GGUF_MAX_NESTING_DEPTH;

const GgufError = types.GgufError;
const GgufValueType = types.GgufValueType;
const GgmlType = types.GgmlType;
const GgufArray = types.GgufArray;
const GgufValue = types.GgufValue;
const GgufKv = types.GgufKv;
const GgufTensorInfo = types.GgufTensorInfo;
const GgufFile = types.GgufFile;
const isPowerOfTwo = types.isPowerOfTwo;
const alignUp = types.alignUp;

const Cursor = struct {
    buf: []const u8,
    pos: usize,

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn readBytes(*_, len: usize): GgufError![]const u8 {
        if (len > _.remaining()) {
            return GgufError.UnexpectedEof;
        }
        const slice = _.buf[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    fn readU8(*_): GgufError!u8 {
        const b = try _.readBytes(1);
        return b[0];
    }

    fn readI8(*_): GgufError!i8 {
        const b = try _.readU8();
        return @bitCast(b);
    }

    fn readU16(*_): GgufError!u16 {
        const b = try _.readBytes(2);
        return std.mem.readInt(u16, b[0..2], .little);
    }

    fn readI16(*_): GgufError!i16 {
        const b = try _.readBytes(2);
        return std.mem.readInt(i16, b[0..2], .little);
    }

    fn readU32(*_): GgufError!u32 {
        const b = try _.readBytes(4);
        return std.mem.readInt(u32, b[0..4], .little);
    }

    fn readI32(*_): GgufError!i32 {
        const b = try _.readBytes(4);
        return std.mem.readInt(i32, b[0..4], .little);
    }

    fn readU64(*_): GgufError!u64 {
        const b = try _.readBytes(8);
        return std.mem.readInt(u64, b[0..8], .little);
    }

    fn readI64(*_): GgufError!i64 {
        const b = try _.readBytes(8);
        return std.mem.readInt(i64, b[0..8], .little);
    }

    fn readF32(*_): GgufError!f32 {
        const raw = try _.readU32();
        return @bitCast(raw);
    }

    fn readF64(*_): GgufError!f64 {
        const raw = try _.readU64();
        return @bitCast(raw);
    }

    fn readString(*_, alloc: Allocator): GgufError![]const u8 {
        const len_u64 = try _.readU64();
        if (len_u64 > GGUF_MAX_STRING_LEN) {
            return GgufError.InvalidString;
        }
        const len: usize = @intCast(len_u64);
        const raw = try _.readBytes(len);
        return alloc.dupe(u8, raw) catch GgufError.OutOfMemory;
    }
};

fn readValue(alloc: Allocator, cur: *Cursor, vtype: GgufValueType, depth: u32): GgufError!GgufValue {
    if (depth > GGUF_MAX_NESTING_DEPTH) {
        return GgufError.NestingTooDeep;
    }
    return switch (vtype) {
        .U8 => GgufValue{ .u8_val = try cur.readU8() },
        .I8 => GgufValue{ .i8_val = try cur.readI8() },
        .U16 => GgufValue{ .u16_val = try cur.readU16() },
        .I16 => GgufValue{ .i16_val = try cur.readI16() },
        .U32 => GgufValue{ .u32_val = try cur.readU32() },
        .I32 => GgufValue{ .i32_val = try cur.readI32() },
        .F32 => GgufValue{ .f32_val = try cur.readF32() },
        .BOOL => GgufValue{ .bool_val = (try cur.readU8()) != 0 },
        .STRING => GgufValue{ .string_val = try cur.readString(alloc) },
        .U64 => GgufValue{ .u64_val = try cur.readU64() },
        .I64 => GgufValue{ .i64_val = try cur.readI64() },
        .F64 => GgufValue{ .f64_val = try cur.readF64() },
        .ARRAY => {
            const elem_raw = try cur.readU32();
            const elem_type = try GgufValueType.fromU32(elem_raw);
            const count_u64 = try cur.readU64();
            if (count_u64 > GGUF_MAX_ARRAY_LEN) {
                return GgufError.ArrayTooLarge;
            }
            const count: usize = @intCast(count_u64);
            if (count > cur.remaining()) {
                return GgufError.UnexpectedEof;
            }
            let items = alloc.alloc(GgufValue, count) catch return GgufError.OutOfMemory;
            let initialized: usize = 0;
            errdefer {
                let i: usize = 0;
                while (i < initialized) : (i += 1) {
                    items[i].deinit(alloc);
                }
                alloc.free(items);
            }
            while (initialized < count) : (initialized += 1) {
                items[initialized] = try readValue(alloc, cur, elem_type, depth + 1);
            }
            return GgufValue{
                .array_val = GgufArray{
                    .elem_type = elem_type,
                    .items = items,
                },
            };
        },
    };
}

pub fn parseGguf(alloc: Allocator, bytes: []const u8): GgufError!GgufFile {
    let cur = Cursor{ .buf = bytes, .pos = 0 };

    const magic = try cur.readU32();
    if (magic != GGUF_MAGIC) {
        return GgufError.InvalidMagic;
    }

    const version = try cur.readU32();
    if (version != GGUF_VERSION_2 and version != GGUF_VERSION_3) {
        return GgufError.UnsupportedVersion;
    }

    const tensor_count_u64 = try cur.readU64();
    if (tensor_count_u64 > GGUF_MAX_TENSOR_COUNT) {
        return GgufError.TooManyTensors;
    }
    const kv_count_u64 = try cur.readU64();
    if (kv_count_u64 > GGUF_MAX_KV_COUNT) {
        return GgufError.TooManyKvs;
    }

    const kv_count: usize = @intCast(kv_count_u64);
    const tensor_count: usize = @intCast(tensor_count_u64);

    let kvs = alloc.alloc(GgufKv, kv_count) catch return GgufError.OutOfMemory;
    let kv_init: usize = 0;
    errdefer {
        let i: usize = 0;
        while (i < kv_init) : (i += 1) {
            alloc.free(kvs[i].key);
            kvs[i].value.deinit(alloc);
        }
        alloc.free(kvs);
    }

    let alignment: u64 = GGUF_DEFAULT_ALIGNMENT;

    while (kv_init < kv_count) : (kv_init += 1) {
        const key = try cur.readString(alloc);
        errdefer alloc.free(key);

        const vtype_raw = try cur.readU32();
        const vtype = try GgufValueType.fromU32(vtype_raw);
        let val = try readValue(alloc, &cur, vtype, 0);
        errdefer val.deinit(alloc);

        if (std.mem.eql(u8, key, "general.alignment")) {
            alignment = switch (val) {
                .u32_val => |v| @as(u64, v),
                .u64_val => |v| v,
                else => return GgufError.InvalidAlignment,
            };
            if (!isPowerOfTwo(alignment)) {
                return GgufError.InvalidAlignment;
            }
        }

        kvs[kv_init] = GgufKv{
            .key = key,
            .value = val,
        };
    }

    let tensors = alloc.alloc(GgufTensorInfo, tensor_count) catch return GgufError.OutOfMemory;
    let t_init: usize = 0;
    errdefer {
        let i: usize = 0;
        while (i < t_init) : (i += 1) {
            alloc.free(tensors[i].name);
        }
        alloc.free(tensors);
    }

    while (t_init < tensor_count) : (t_init += 1) {
        const name = try cur.readString(alloc);
        errdefer alloc.free(name);

        const n_dims = try cur.readU32();
        if (n_dims > GGUF_MAX_DIMS) {
            return GgufError.TooManyDims;
        }
        let dims: [4]u64 = [ 1, 1, 1, 1 ];
        let d: u32 = 0;
        while (d < n_dims) : (d += 1) {
            dims[d] = try cur.readU64();
        }

        const ggml_raw = try cur.readU32();
        const ggml_type = try GgmlType.fromU32(ggml_raw);
        const offset = try cur.readU64();

        tensors[t_init] = GgufTensorInfo{
            .name = name,
            .n_dims = n_dims,
            .dims = dims,
            .ggml_type = ggml_type,
            .offset = offset,
        };
    }

    const data_offset = try alignUp(@as(u64, cur.pos), alignment);
    if (data_offset > bytes.len) {
        return GgufError.TensorOutOfBounds;
    }

    for (*t of tensors) {
        const size = try t.byteSize();
        const start = std.math.add(u64, data_offset, t.offset) catch return GgufError.TensorOutOfBounds;
        const end = std.math.add(u64, start, size) catch return GgufError.TensorOutOfBounds;
        if (end > bytes.len) {
            return GgufError.TensorOutOfBounds;
        }
    }

    return GgufFile{
        .version = version,
        .alignment = alignment,
        .kvs = kvs,
        .tensors = tensors,
        .data_offset = data_offset,
        .data = bytes,
    };
}
