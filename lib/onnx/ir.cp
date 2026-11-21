// Source: cp/lib/onnx/ir.cp (ONNX Intermediate Representation types & memory management)
const std = @import("std");
import { OnnxError } from "./wire.cp";

pub const OnnxDataType = enum(i32) {
    UNDEFINED = 0,
    FLOAT = 1,
    UINT8 = 2,
    INT8 = 3,
    UINT16 = 4,
    INT16 = 5,
    INT32 = 6,
    INT64 = 7,
    STRING = 8,
    BOOL = 9,
    FLOAT16 = 10,
    DOUBLE = 11,
    UINT32 = 12,
    UINT64 = 13,
    BFLOAT16 = 16,

    pub fn fromI32(val: i32): OnnxDataType {
        return switch (val) {
            1 => .FLOAT,
            2 => .UINT8,
            3 => .INT8,
            4 => .UINT16,
            5 => .INT16,
            6 => .INT32,
            7 => .INT64,
            8 => .STRING,
            9 => .BOOL,
            10 => .FLOAT16,
            11 => .DOUBLE,
            12 => .UINT32,
            13 => .UINT64,
            16 => .BFLOAT16,
            else => .UNDEFINED,
        };
    }

    pub fn elementSizeBytes(self: OnnxDataType): usize {
        return switch (self) {
            .FLOAT, .INT32, .UINT32 => 4,
            .DOUBLE, .INT64, .UINT64 => 8,
            .FLOAT16, .BFLOAT16, .INT16, .UINT16 => 2,
            .UINT8, .INT8, .BOOL => 1,
            .UNDEFINED, .STRING => 0,
        };
    }
};

pub const OnnxDim = union(enum) {
    dim_value: i64,
    dim_param: []const u8,
};

pub const OnnxValueInfo = struct {
    name: []const u8 = "",
    elem_type: OnnxDataType = .UNDEFINED,
    dims: []const OnnxDim = &.{},
    doc_string: []const u8 = "",

    pub fn deinit(self: *OnnxValueInfo, alloc: std.mem.Allocator) {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        for (d of self.dims) {
            switch (d) {
                .dim_param => |s| {
                    if (s.len > 0) {
                        alloc.free(s);
                    }
                },
                .dim_value => {},
            }
        }
        if (self.dims.len > 0) {
            alloc.free(self.dims);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
    }
};

pub const OnnxAttrType = enum(i32) {
    UNDEFINED = 0,
    FLOAT = 1,
    INT = 2,
    STRING = 3,
    FLOATS = 6,
    INTS = 7,
    STRINGS = 8,
};

pub const OnnxAttrValue = union(enum) {
    none: void,
    float_val: f32,
    int_val: i64,
    string_val: []const u8,
    floats: []const f32,
    ints: []const i64,
    strings: []const []const u8,
};

pub const OnnxAttribute = struct {
    name: []const u8 = "",
    value: OnnxAttrValue = .{ .none = {} },
    doc_string: []const u8 = "",

    pub fn attrType(self: *const OnnxAttribute): OnnxAttrType {
        return switch (self.value) {
            .none => .UNDEFINED,
            .float_val => .FLOAT,
            .int_val => .INT,
            .string_val => .STRING,
            .floats => .FLOATS,
            .ints => .INTS,
            .strings => .STRINGS,
        };
    }

    pub fn deinit(self: *OnnxAttribute, alloc: std.mem.Allocator) {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
        switch (self.value) {
            .none, .float_val, .int_val => {},
            .string_val => |s| {
                if (s.len > 0) {
                    alloc.free(s);
                }
            },
            .floats => |fs| {
                if (fs.len > 0) {
                    alloc.free(fs);
                }
            },
            .ints => |is| {
                if (is.len > 0) {
                    alloc.free(is);
                }
            },
            .strings => |ss| {
                for (s of ss) {
                    if (s.len > 0) {
                        alloc.free(s);
                    }
                }
                if (ss.len > 0) {
                    alloc.free(ss);
                }
            },
        }
    }
};

pub const OnnxNode = struct {
    name: []const u8 = "",
    op_type: []const u8 = "",
    domain: []const u8 = "",
    inputs: []const []const u8 = &.{},
    outputs: []const []const u8 = &.{},
    attributes: []const OnnxAttribute = &.{},
    doc_string: []const u8 = "",

    pub fn deinit(self: *OnnxNode, alloc: std.mem.Allocator) {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        if (self.op_type.len > 0) {
            alloc.free(self.op_type);
        }
        if (self.domain.len > 0) {
            alloc.free(self.domain);
        }
        for (inp of self.inputs) {
            if (inp.len > 0) {
                alloc.free(inp);
            }
        }
        if (self.inputs.len > 0) {
            alloc.free(self.inputs);
        }
        for (out of self.outputs) {
            if (out.len > 0) {
                alloc.free(out);
            }
        }
        if (self.outputs.len > 0) {
            alloc.free(self.outputs);
        }
        for (attr of self.attributes) {
            let a = attr;
            a.deinit(alloc);
        }
        if (self.attributes.len > 0) {
            alloc.free(self.attributes);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
    }
};

pub const OnnxTensor = struct {
    name: []const u8 = "",
    dims: []const i64 = &.{},
    data_type: OnnxDataType = .UNDEFINED,
    raw_data: []const u8 = "",
    float_data: []const f32 = &.{},
    int32_data: []const i32 = &.{},
    int64_data: []const i64 = &.{},
    doc_string: []const u8 = "",

    pub fn elementCount(self: *const OnnxTensor): usize {
        if (self.dims.len == 0) {
            if (self.float_data.len > 0) {
                return self.float_data.len;
            }
            if (self.int32_data.len > 0) {
                return self.int32_data.len;
            }
            if (self.int64_data.len > 0) {
                return self.int64_data.len;
            }
            const elem_sz = self.data_type.elementSizeBytes();
            if (elem_sz > 0 and self.raw_data.len > 0) {
                return self.raw_data.len / elem_sz;
            }
            return 0;
        }
        let total: usize = 1;
        for (d of self.dims) {
            if (d <= 0) {
                return 0;
            }
            total *= @as(usize, @intCast(d));
        }
        return total;
    }

    pub fn extractFloats(self: *const OnnxTensor, alloc: std.mem.Allocator): ![]f32 {
        if (self.data_type != .FLOAT) {
            return OnnxError.InvalidDataType;
        }
        if (self.float_data.len > 0) {
            const out = try alloc.alloc(f32, self.float_data.len);
            @memcpy(out, self.float_data);
            return out;
        }
        if (self.raw_data.len % 4 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = self.raw_data.len / 4;
        let out = try alloc.alloc(f32, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 4;
            const bits = std.mem.readInt(u32, self.raw_data[off .. off + 4][0..4], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn extractInt64s(self: *const OnnxTensor, alloc: std.mem.Allocator): ![]i64 {
        if (self.data_type != .INT64) {
            return OnnxError.InvalidDataType;
        }
        if (self.int64_data.len > 0) {
            const out = try alloc.alloc(i64, self.int64_data.len);
            @memcpy(out, self.int64_data);
            return out;
        }
        if (self.raw_data.len % 8 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = self.raw_data.len / 8;
        let out = try alloc.alloc(i64, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 8;
            const bits = std.mem.readInt(u64, self.raw_data[off .. off + 8][0..8], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn extractInt32s(self: *const OnnxTensor, alloc: std.mem.Allocator): ![]i32 {
        if (self.data_type != .INT32) {
            return OnnxError.InvalidDataType;
        }
        if (self.int32_data.len > 0) {
            const out = try alloc.alloc(i32, self.int32_data.len);
            @memcpy(out, self.int32_data);
            return out;
        }
        if (self.raw_data.len % 4 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = self.raw_data.len / 4;
        let out = try alloc.alloc(i32, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 4;
            const bits = std.mem.readInt(u32, self.raw_data[off .. off + 4][0..4], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn deinit(self: *OnnxTensor, alloc: std.mem.Allocator) {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        if (self.dims.len > 0) {
            alloc.free(self.dims);
        }
        if (self.raw_data.len > 0) {
            alloc.free(self.raw_data);
        }
        if (self.float_data.len > 0) {
            alloc.free(self.float_data);
        }
        if (self.int32_data.len > 0) {
            alloc.free(self.int32_data);
        }
        if (self.int64_data.len > 0) {
            alloc.free(self.int64_data);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
    }
};

pub const OnnxOpsetImport = struct {
    domain: []const u8 = "",
    version: i64 = 0,

    pub fn deinit(self: *OnnxOpsetImport, alloc: std.mem.Allocator) {
        if (self.domain.len > 0) {
            alloc.free(self.domain);
        }
    }
};

pub const OnnxGraph = struct {
    name: []const u8 = "",
    nodes: []const OnnxNode = &.{},
    inputs: []const OnnxValueInfo = &.{},
    outputs: []const OnnxValueInfo = &.{},
    initializers: []const OnnxTensor = &.{},
    doc_string: []const u8 = "",

    pub fn findInitializer(self: *const OnnxGraph, name: []const u8): ?*const OnnxTensor {
        let i: usize = 0;
        while (i < self.initializers.len) {
            if (std.mem.eql(u8, self.initializers[i].name, name)) {
                return &self.initializers[i];
            }
            i += 1;
        }
        return null;
    }

    pub fn deinit(self: *OnnxGraph, alloc: std.mem.Allocator) {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        for (n of self.nodes) {
            let item = n;
            item.deinit(alloc);
        }
        if (self.nodes.len > 0) {
            alloc.free(self.nodes);
        }
        for (vi of self.inputs) {
            let item = vi;
            item.deinit(alloc);
        }
        if (self.inputs.len > 0) {
            alloc.free(self.inputs);
        }
        for (vo of self.outputs) {
            let item = vo;
            item.deinit(alloc);
        }
        if (self.outputs.len > 0) {
            alloc.free(self.outputs);
        }
        for (t of self.initializers) {
            let item = t;
            item.deinit(alloc);
        }
        if (self.initializers.len > 0) {
            alloc.free(self.initializers);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
    }
};

pub const OnnxModel = struct {
    ir_version: i64 = 8,
    opset_import: []const OnnxOpsetImport = &.{},
    producer_name: []const u8 = "",
    producer_version: []const u8 = "",
    domain: []const u8 = "",
    model_version: i64 = 0,
    doc_string: []const u8 = "",
    graph: ?OnnxGraph = null,

    pub fn findInitializer(self: *const OnnxModel, name: []const u8): ?*const OnnxTensor {
        if (self.graph) |*g| {
            return g.findInitializer(name);
        }
        return null;
    }

    pub fn deinit(self: *OnnxModel, alloc: std.mem.Allocator) {
        for (op of self.opset_import) {
            let item = op;
            item.deinit(alloc);
        }
        if (self.opset_import.len > 0) {
            alloc.free(self.opset_import);
        }
        if (self.producer_name.len > 0) {
            alloc.free(self.producer_name);
        }
        if (self.producer_version.len > 0) {
            alloc.free(self.producer_version);
        }
        if (self.domain.len > 0) {
            alloc.free(self.domain);
        }
        if (self.doc_string.len > 0) {
            alloc.free(self.doc_string);
        }
        if (self.graph) |*g| {
            g.deinit(alloc);
        }
    }
};
