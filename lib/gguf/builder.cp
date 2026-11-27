// Source: cp/lib/gguf/builder.cp (GGUF v3 binary builder with automatic alignment padding)
const std = @import("std");
const Allocator = std.mem.Allocator;
const types = @import("./types.cp");

const GGUF_MAGIC = types.GGUF_MAGIC;
const GGUF_VERSION_3 = types.GGUF_VERSION_3;
const GGUF_DEFAULT_ALIGNMENT = types.GGUF_DEFAULT_ALIGNMENT;
const GGUF_MAX_DIMS = types.GGUF_MAX_DIMS;

const GgufError = types.GgufError;
const GgufValueType = types.GgufValueType;
const GgmlType = types.GgmlType;
const GgufArray = types.GgufArray;
const GgufValue = types.GgufValue;
const GgufKv = types.GgufKv;
const isPowerOfTwo = types.isPowerOfTwo;
const alignUp = types.alignUp;

const BuilderTensor = struct {
    name: []const u8,
    n_dims: u32,
    dims: [4]u64,
    ggml_type: GgmlType,
    data: []const u8,
};

fn appendInt(comptime T: type, list: *std.ArrayList(u8), alloc: Allocator, val: T): GgufError!void {
    let buf: [@sizeOf(T)]u8 = undefined;
    std.mem.writeInt(T, &buf, val, .little);
    list.appendSlice(alloc, &buf) catch return GgufError.OutOfMemory;
}

fn appendString(list: *std.ArrayList(u8), alloc: Allocator, s: []const u8): GgufError!void {
    try appendInt(u64, list, alloc, @as(u64, s.len));
    list.appendSlice(alloc, s) catch return GgufError.OutOfMemory;
}

fn appendValue(list: *std.ArrayList(u8), alloc: Allocator, val: *const GgufValue): GgufError!void {
    switch (val.*) {
        .u8_val => |v| list.append(alloc, v) catch return GgufError.OutOfMemory,
        .i8_val => |v| list.append(alloc, @bitCast(v)) catch return GgufError.OutOfMemory,
        .u16_val => |v| try appendInt(u16, list, alloc, v),
        .i16_val => |v| try appendInt(i16, list, alloc, v),
        .u32_val => |v| try appendInt(u32, list, alloc, v),
        .i32_val => |v| try appendInt(i32, list, alloc, v),
        .f32_val => |v| try appendInt(u32, list, alloc, @bitCast(v)),
        .bool_val => |v| list.append(alloc, if (v) 1 else 0) catch return GgufError.OutOfMemory,
        .string_val => |s| try appendString(list, alloc, s),
        .u64_val => |v| try appendInt(u64, list, alloc, v),
        .i64_val => |v| try appendInt(i64, list, alloc, v),
        .f64_val => |v| try appendInt(u64, list, alloc, @bitCast(v)),
        .array_val => |arr| {
            try appendInt(u32, list, alloc, @intFromEnum(arr.elem_type));
            try appendInt(u64, list, alloc, @as(u64, arr.items.len));
            for (*item of arr.items) {
                try appendValue(list, alloc, item);
            }
        },
    }
}

pub const GgufBuilder = struct {
    alloc: Allocator,
    alignment: u64,
    kvs: std.ArrayList(GgufKv),
    tensors: std.ArrayList(BuilderTensor),

    pub static fn init(alloc: Allocator): GgufBuilder {
        return GgufBuilder{
            .alloc = alloc,
            .alignment = GGUF_DEFAULT_ALIGNMENT,
            .kvs = std.ArrayList(GgufKv).empty,
            .tensors = std.ArrayList(BuilderTensor).empty,
        };
    }

    pub fn deinit(*_) {
        for (*kv of _.kvs.items) {
            _.alloc.free(kv.key);
            kv.value.deinit(_.alloc);
        }
        _.kvs.deinit(_.alloc);

        for (t of _.tensors.items) {
            _.alloc.free(t.name);
            _.alloc.free(t.data);
        }
        _.tensors.deinit(_.alloc);
    }

    pub fn setAlignment(*_, alignment: u64): GgufError!void {
        if (!isPowerOfTwo(alignment)) {
            return GgufError.InvalidAlignment;
        }
        _.alignment = alignment;
        for (*kv of _.kvs.items) {
            if (std.mem.eql(u8, kv.key, "general.alignment")) {
                kv.value.deinit(_.alloc);
                kv.value = GgufValue{ .u32_val = @intCast(alignment) };
                return;
            }
        }
        try _.addU32("general.alignment", @intCast(alignment));
    }

    pub fn addKv(*_, key: []const u8, val: GgufValue): GgufError!void {
        const key_dup = _.alloc.dupe(u8, key) catch return GgufError.OutOfMemory;
        errdefer _.alloc.free(key_dup);

        let val_dup = try val.clone(_.alloc);
        errdefer val_dup.deinit(_.alloc);

        _.kvs.append(_.alloc, GgufKv{
            .key = key_dup,
            .value = val_dup,
        }) catch return GgufError.OutOfMemory;
    }

    pub fn addU32(*_, key: []const u8, val: u32): GgufError!void {
        try _.addKv(key, GgufValue{ .u32_val = val });
    }

    pub fn addI32(*_, key: []const u8, val: i32): GgufError!void {
        try _.addKv(key, GgufValue{ .i32_val = val });
    }

    pub fn addU64(*_, key: []const u8, val: u64): GgufError!void {
        try _.addKv(key, GgufValue{ .u64_val = val });
    }

    pub fn addF32(*_, key: []const u8, val: f32): GgufError!void {
        try _.addKv(key, GgufValue{ .f32_val = val });
    }

    pub fn addBool(*_, key: []const u8, val: bool): GgufError!void {
        try _.addKv(key, GgufValue{ .bool_val = val });
    }

    pub fn addString(*_, key: []const u8, val: []const u8): GgufError!void {
        try _.addKv(key, GgufValue{ .string_val = val });
    }

    pub fn addStringArray(*_, key: []const u8, items: []const []const u8): GgufError!void {
        const key_dup = _.alloc.dupe(u8, key) catch return GgufError.OutOfMemory;
        errdefer _.alloc.free(key_dup);

        let arr_items = _.alloc.alloc(GgufValue, items.len) catch return GgufError.OutOfMemory;
        let init_count: usize = 0;
        errdefer {
            let i: usize = 0;
            while (i < init_count) : (i += 1) {
                arr_items[i].deinit(_.alloc);
            }
            _.alloc.free(arr_items);
        }

        for (s of items) {
            const s_dup = _.alloc.dupe(u8, s) catch return GgufError.OutOfMemory;
            arr_items[init_count] = GgufValue{ .string_val = s_dup };
            init_count += 1;
        }

        const val = GgufValue{
            .array_val = GgufArray{
                .elem_type = .STRING,
                .items = arr_items,
            },
        };
        _.kvs.append(_.alloc, GgufKv{
            .key = key_dup,
            .value = val,
        }) catch {
            let v = val;
            v.deinit(_.alloc);
            return GgufError.OutOfMemory;
        };
    }

    pub fn addTensor(
        *_,
        name: []const u8,
        dims: []const u64,
        ggml_type: GgmlType,
        data: []const u8,
    ): GgufError!void {
        if (dims.len > GGUF_MAX_DIMS) {
            return GgufError.TooManyDims;
        }
        let dims_arr: [4]u64 = [ 1, 1, 1, 1 ];
        let n_elems: u64 = if (dims.len == 0) 0 else 1;
        let idx: usize = 0;
        while (idx < dims.len) : (idx += 1) {
            dims_arr[idx] = dims[idx];
            n_elems = std.math.mul(u64, n_elems, dims[idx]) catch return GgufError.TensorOutOfBounds;
        }
        const expected_bytes = try ggml_type.byteSize(n_elems);
        if (data.len != expected_bytes) {
            return GgufError.TensorOutOfBounds;
        }

        const name_dup = _.alloc.dupe(u8, name) catch return GgufError.OutOfMemory;
        errdefer _.alloc.free(name_dup);

        const data_dup = _.alloc.dupe(u8, data) catch return GgufError.OutOfMemory;
        errdefer _.alloc.free(data_dup);

        _.tensors.append(_.alloc, BuilderTensor{
            .name = name_dup,
            .n_dims = @intCast(dims.len),
            .dims = dims_arr,
            .ggml_type = ggml_type,
            .data = data_dup,
        }) catch return GgufError.OutOfMemory;
    }

    pub fn build(alloc: Allocator): GgufError![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        // Header: magic, version, tensor_count, kv_count
        try appendInt(u32, &out, alloc, GGUF_MAGIC);
        try appendInt(u32, &out, alloc, GGUF_VERSION_3);
        try appendInt(u64, &out, alloc, @as(u64, _.tensors.items.len));
        try appendInt(u64, &out, alloc, @as(u64, _.kvs.items.len));

        // KVs
        for (kv of _.kvs.items) {
            try appendString(&out, alloc, kv.key);
            try appendInt(u32, &out, alloc, @intFromEnum(kv.value.valueType()));
            try appendValue(&out, alloc, &kv.value);
        }

        // Tensor infos with aligned relative offsets
        let rel_offset: u64 = 0;
        for (t of _.tensors.items) {
            try appendString(&out, alloc, t.name);
            try appendInt(u32, &out, alloc, t.n_dims);
            let d: u32 = 0;
            while (d < t.n_dims) : (d += 1) {
                try appendInt(u64, &out, alloc, t.dims[d]);
            }
            try appendInt(u32, &out, alloc, @intFromEnum(t.ggml_type));
            try appendInt(u64, &out, alloc, rel_offset);

            const padded_size = try alignUp(@as(u64, t.data.len), _.alignment);
            rel_offset = std.math.add(u64, rel_offset, padded_size) catch return GgufError.TensorOutOfBounds;
        }

        // Pad header to alignment before tensor data section
        const data_start = try alignUp(@as(u64, out.items.len), _.alignment);
        while (out.items.len < data_start) {
            out.append(alloc, 0) catch return GgufError.OutOfMemory;
        }

        // Tensor data payloads with alignment padding
        for (t of _.tensors.items) {
            out.appendSlice(alloc, t.data) catch return GgufError.OutOfMemory;
            const padded_size = try alignUp(@as(u64, t.data.len), _.alignment);
            let pad = padded_size - @as(u64, t.data.len);
            while (pad > 0) : (pad -= 1) {
                out.append(alloc, 0) catch return GgufError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch GgufError.OutOfMemory;
    }
};
