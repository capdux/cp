// Source: cp/lib/gguf/types.cp (GGUF v3 types, constants, and tensor descriptors)
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const GGUF_MAGIC: u32 = 0x46554747; // "GGUF" little-endian
pub const GGUF_VERSION_2: u32 = 2;
pub const GGUF_VERSION_3: u32 = 3;
pub const GGUF_DEFAULT_ALIGNMENT: u64 = 32;
pub const GGUF_MAX_DIMS: usize = 4;
pub const GGUF_MAX_ARRAY_LEN: u64 = 1_000_000;
pub const GGUF_MAX_STRING_LEN: u64 = 16 * 1024 * 1024;
pub const GGUF_MAX_KV_COUNT: u64 = 1_000_000;
pub const GGUF_MAX_TENSOR_COUNT: u64 = 1_000_000;
pub const GGUF_MAX_NESTING_DEPTH: u32 = 8;

pub const GgufError = error{
    InvalidMagic,
    UnsupportedVersion,
    InvalidAlignment,
    UnexpectedEof,
    InvalidValueType,
    InvalidString,
    NestingTooDeep,
    ArrayTooLarge,
    TooManyKvs,
    TooManyTensors,
    TooManyDims,
    InvalidTensorType,
    TensorOutOfBounds,
    OutOfMemory,
};

pub const GgufValueType = enum(u32) {
    U8 = 0,
    I8 = 1,
    U16 = 2,
    I16 = 3,
    U32 = 4,
    I32 = 5,
    F32 = 6,
    BOOL = 7,
    STRING = 8,
    ARRAY = 9,
    U64 = 10,
    I64 = 11,
    F64 = 12,

    pub static fn fromU32(raw: u32): GgufError!GgufValueType {
        return switch (raw) {
            0 => .U8,
            1 => .I8,
            2 => .U16,
            3 => .I16,
            4 => .U32,
            5 => .I32,
            6 => .F32,
            7 => .BOOL,
            8 => .STRING,
            9 => .ARRAY,
            10 => .U64,
            11 => .I64,
            12 => .F64,
            else => GgufError.InvalidValueType,
        };
    }
};

pub const GgmlType = enum(u32) {
    F32 = 0,
    F16 = 1,
    Q4_0 = 2,
    Q4_1 = 3,
    Q5_0 = 6,
    Q5_1 = 7,
    Q8_0 = 8,
    Q8_1 = 9,
    Q2_K = 10,
    Q3_K = 11,
    Q4_K = 12,
    Q5_K = 13,
    Q6_K = 14,
    Q8_K = 15,
    I8 = 24,
    I16 = 25,
    I32 = 26,
    I64 = 27,
    F64 = 28,
    BF16 = 30,

    pub static fn fromU32(raw: u32): GgufError!GgmlType {
        return switch (raw) {
            0 => .F32,
            1 => .F16,
            2 => .Q4_0,
            3 => .Q4_1,
            6 => .Q5_0,
            7 => .Q5_1,
            8 => .Q8_0,
            9 => .Q8_1,
            10 => .Q2_K,
            11 => .Q3_K,
            12 => .Q4_K,
            13 => .Q5_K,
            14 => .Q6_K,
            15 => .Q8_K,
            24 => .I8,
            25 => .I16,
            26 => .I32,
            27 => .I64,
            28 => .F64,
            30 => .BF16,
            else => GgufError.InvalidTensorType,
        };
    }

    pub fn blockSize(): u64 {
        return switch (_) {
            .F32, .F16, .I8, .I16, .I32, .I64, .F64, .BF16 => 1,
            .Q4_0, .Q4_1, .Q5_0, .Q5_1, .Q8_0, .Q8_1 => 32,
            .Q2_K, .Q3_K, .Q4_K, .Q5_K, .Q6_K, .Q8_K => 256,
        };
    }

    pub fn typeSize(): u64 {
        return switch (_) {
            .F32 => 4,
            .F16, .BF16, .I16 => 2,
            .I8 => 1,
            .I32 => 4,
            .I64, .F64 => 8,
            .Q4_0 => 18,
            .Q4_1 => 20,
            .Q5_0 => 22,
            .Q5_1 => 24,
            .Q8_0 => 34,
            .Q8_1 => 36,
            .Q2_K => 84,
            .Q3_K => 110,
            .Q4_K => 144,
            .Q5_K => 176,
            .Q6_K => 210,
            .Q8_K => 292,
        };
    }

    pub fn byteSize(n_elements: u64): GgufError!u64 {
        const bs = _.blockSize();
        const ts = _.typeSize();
        if (n_elements == 0) {
            return 0;
        }
        const n_blocks = (n_elements + bs - 1) / bs;
        return std.math.mul(u64, n_blocks, ts) catch GgufError.TensorOutOfBounds;
    }
};

pub const GgufArray = struct {
    elem_type: GgufValueType,
    items: []GgufValue,
};

pub const GgufValue = union(enum) {
    u8_val: u8,
    i8_val: i8,
    u16_val: u16,
    i16_val: i16,
    u32_val: u32,
    i32_val: i32,
    f32_val: f32,
    bool_val: bool,
    string_val: []const u8,
    array_val: GgufArray,
    u64_val: u64,
    i64_val: i64,
    f64_val: f64,

    pub fn valueType(): GgufValueType {
        return switch (_) {
            .u8_val => .U8,
            .i8_val => .I8,
            .u16_val => .U16,
            .i16_val => .I16,
            .u32_val => .U32,
            .i32_val => .I32,
            .f32_val => .F32,
            .bool_val => .BOOL,
            .string_val => .STRING,
            .array_val => .ARRAY,
            .u64_val => .U64,
            .i64_val => .I64,
            .f64_val => .F64,
        };
    }

    pub fn deinit(*_, alloc: Allocator) {
        switch (_.*) {
            .string_val => |s| alloc.free(s),
            .array_val => |arr| {
                for (*item of arr.items) {
                    item.deinit(alloc);
                }
                alloc.free(arr.items);
            },
            else => {},
        }
    }

    pub fn clone(alloc: Allocator): GgufError!GgufValue {
        return switch (_) {
            .u8_val => |v| GgufValue{ .u8_val = v },
            .i8_val => |v| GgufValue{ .i8_val = v },
            .u16_val => |v| GgufValue{ .u16_val = v },
            .i16_val => |v| GgufValue{ .i16_val = v },
            .u32_val => |v| GgufValue{ .u32_val = v },
            .i32_val => |v| GgufValue{ .i32_val = v },
            .f32_val => |v| GgufValue{ .f32_val = v },
            .bool_val => |v| GgufValue{ .bool_val = v },
            .u64_val => |v| GgufValue{ .u64_val = v },
            .i64_val => |v| GgufValue{ .i64_val = v },
            .f64_val => |v| GgufValue{ .f64_val = v },
            .string_val => |s| {
                const dup = alloc.dupe(u8, s) catch return GgufError.OutOfMemory;
                return GgufValue{ .string_val = dup };
            },
            .array_val => |arr| {
                let cloned = alloc.alloc(GgufValue, arr.items.len) catch return GgufError.OutOfMemory;
                let initialized: usize = 0;
                errdefer {
                    let i: usize = 0;
                    while (i < initialized) : (i += 1) {
                        cloned[i].deinit(alloc);
                    }
                    alloc.free(cloned);
                }
                for (*item of arr.items) {
                    cloned[initialized] = try item.clone(alloc);
                    initialized += 1;
                }
                return GgufValue{
                    .array_val = GgufArray{
                        .elem_type = arr.elem_type,
                        .items = cloned,
                    },
                };
            },
        };
    }
};

pub const GgufKv = struct {
    key: []const u8,
    value: GgufValue,
};

pub const GgufTensorInfo = struct {
    name: []const u8,
    n_dims: u32,
    dims: [4]u64,
    ggml_type: GgmlType,
    offset: u64,

    pub fn numElements(): GgufError!u64 {
        if (_.n_dims == 0) {
            return 0;
        }
        let total: u64 = 1;
        let i: u32 = 0;
        while (i < _.n_dims) : (i += 1) {
            total = std.math.mul(u64, total, _.dims[i]) catch return GgufError.TensorOutOfBounds;
        }
        return total;
    }

    pub fn byteSize(): GgufError!u64 {
        const elems = try _.numElements();
        return _.ggml_type.byteSize(elems);
    }
};

pub const GgufFile = struct {
    version: u32,
    alignment: u64,
    kvs: []GgufKv,
    tensors: []GgufTensorInfo,
    data_offset: u64,
    data: []const u8,

    pub fn deinit(*_, alloc: Allocator) {
        for (*kv of _.kvs) {
            alloc.free(kv.key);
            kv.value.deinit(alloc);
        }
        alloc.free(_.kvs);

        for (t of _.tensors) {
            alloc.free(t.name);
        }
        alloc.free(_.tensors);
    }

    pub fn getKv(key: []const u8): (*const GgufValue)? {
        let i: usize = 0;
        while (i < _.kvs.len) : (i += 1) {
            if (std.mem.eql(u8, _.kvs[i].key, key)) {
                return &_.kvs[i].value;
            }
        }
        return null;
    }

    pub fn getString(key: []const u8): ([]const u8)? {
        const v = _.getKv(key) orelse return null;
        return switch (v.*) {
            .string_val => |s| s,
            else => null,
        };
    }

    pub fn getU32(key: []const u8): u32? {
        const v = _.getKv(key) orelse return null;
        return switch (v.*) {
            .u32_val => |n| n,
            else => null,
        };
    }

    pub fn getU64(key: []const u8): u64? {
        const v = _.getKv(key) orelse return null;
        return switch (v.*) {
            .u64_val => |n| n,
            else => null,
        };
    }

    pub fn getF32(key: []const u8): f32? {
        const v = _.getKv(key) orelse return null;
        return switch (v.*) {
            .f32_val => |n| n,
            else => null,
        };
    }

    pub fn getBool(key: []const u8): bool? {
        const v = _.getKv(key) orelse return null;
        return switch (v.*) {
            .bool_val => |b| b,
            else => null,
        };
    }

    pub fn getTensor(name: []const u8): (*const GgufTensorInfo)? {
        let i: usize = 0;
        while (i < _.tensors.len) : (i += 1) {
            if (std.mem.eql(u8, _.tensors[i].name, name)) {
                return &_.tensors[i];
            }
        }
        return null;
    }

    pub fn tensorData(tensor: *const GgufTensorInfo): GgufError![]const u8 {
        const size = try tensor.byteSize();
        const start = std.math.add(u64, _.data_offset, tensor.offset) catch return GgufError.TensorOutOfBounds;
        const end = std.math.add(u64, start, size) catch return GgufError.TensorOutOfBounds;
        if (end > _.data.len) {
            return GgufError.TensorOutOfBounds;
        }
        return _.data[@intCast(start)..@intCast(end)];
    }
};

pub fn isPowerOfTwo(n: u64): bool {
    return n > 0 and (n & (n - 1)) == 0;
}

pub fn alignUp(offset: u64, alignment: u64): GgufError!u64 {
    if (!isPowerOfTwo(alignment)) {
        return GgufError.InvalidAlignment;
    }
    const mask = alignment - 1;
    const added = std.math.add(u64, offset, mask) catch return GgufError.TensorOutOfBounds;
    return added & ~mask;
}
