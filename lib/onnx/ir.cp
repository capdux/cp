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

    pub static fn fromI32(val: i32): OnnxDataType {
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

    pub fn elementSizeBytes(): usize {
        return switch (_) {
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

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) {
            alloc.free(_.name);
        }
        for (d of _.dims) {
            switch (d) {
                .dim_param => |s| {
                    if (s.len > 0) {
                        alloc.free(s);
                    }
                },
                .dim_value => {},
            }
        }
        if (_.dims.len > 0) {
            alloc.free(_.dims);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
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

    pub fn attrType(): OnnxAttrType {
        return switch (_.value) {
            .none => .UNDEFINED,
            .float_val => .FLOAT,
            .int_val => .INT,
            .string_val => .STRING,
            .floats => .FLOATS,
            .ints => .INTS,
            .strings => .STRINGS,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) {
            alloc.free(_.name);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
        }
        switch (_.value) {
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

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) {
            alloc.free(_.name);
        }
        if (_.op_type.len > 0) {
            alloc.free(_.op_type);
        }
        if (_.domain.len > 0) {
            alloc.free(_.domain);
        }
        for (inp of _.inputs) {
            if (inp.len > 0) {
                alloc.free(inp);
            }
        }
        if (_.inputs.len > 0) {
            alloc.free(_.inputs);
        }
        for (out of _.outputs) {
            if (out.len > 0) {
                alloc.free(out);
            }
        }
        if (_.outputs.len > 0) {
            alloc.free(_.outputs);
        }
        for (attr of _.attributes) {
            let a = attr;
            a.deinit(alloc);
        }
        if (_.attributes.len > 0) {
            alloc.free(_.attributes);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
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

    pub fn elementCount(): usize {
        if (_.dims.len == 0) {
            if (_.float_data.len > 0) {
                return _.float_data.len;
            }
            if (_.int32_data.len > 0) {
                return _.int32_data.len;
            }
            if (_.int64_data.len > 0) {
                return _.int64_data.len;
            }
            const elem_sz = _.data_type.elementSizeBytes();
            if (elem_sz > 0 and _.raw_data.len > 0) {
                return _.raw_data.len / elem_sz;
            }
            return 0;
        }
        let total: usize = 1;
        for (d of _.dims) {
            if (d <= 0) {
                return 0;
            }
            total *= @as(usize, @intCast(d));
        }
        return total;
    }

    pub fn extractFloats(alloc: std.mem.Allocator): ![]f32 {
        if (_.data_type != .FLOAT) {
            return OnnxError.InvalidDataType;
        }
        if (_.float_data.len > 0) {
            const out = try alloc.alloc(f32, _.float_data.len);
            @memcpy(out, _.float_data);
            return out;
        }
        if (_.raw_data.len % 4 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = _.raw_data.len / 4;
        let out = try alloc.alloc(f32, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 4;
            const bits = std.mem.readInt(u32, _.raw_data[off .. off + 4][0..4], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn extractInt64s(alloc: std.mem.Allocator): ![]i64 {
        if (_.data_type != .INT64) {
            return OnnxError.InvalidDataType;
        }
        if (_.int64_data.len > 0) {
            const out = try alloc.alloc(i64, _.int64_data.len);
            @memcpy(out, _.int64_data);
            return out;
        }
        if (_.raw_data.len % 8 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = _.raw_data.len / 8;
        let out = try alloc.alloc(i64, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 8;
            const bits = std.mem.readInt(u64, _.raw_data[off .. off + 8][0..8], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn extractInt32s(alloc: std.mem.Allocator): ![]i32 {
        if (_.data_type != .INT32) {
            return OnnxError.InvalidDataType;
        }
        if (_.int32_data.len > 0) {
            const out = try alloc.alloc(i32, _.int32_data.len);
            @memcpy(out, _.int32_data);
            return out;
        }
        if (_.raw_data.len % 4 != 0) {
            return OnnxError.LengthOutOfBounds;
        }
        const count = _.raw_data.len / 4;
        let out = try alloc.alloc(i32, count);
        let i: usize = 0;
        while (i < count) {
            const off = i * 4;
            const bits = std.mem.readInt(u32, _.raw_data[off .. off + 4][0..4], .little);
            out[i] = @bitCast(bits);
            i += 1;
        }
        return out;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) {
            alloc.free(_.name);
        }
        if (_.dims.len > 0) {
            alloc.free(_.dims);
        }
        if (_.raw_data.len > 0) {
            alloc.free(_.raw_data);
        }
        if (_.float_data.len > 0) {
            alloc.free(_.float_data);
        }
        if (_.int32_data.len > 0) {
            alloc.free(_.int32_data);
        }
        if (_.int64_data.len > 0) {
            alloc.free(_.int64_data);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
        }
    }
};

pub const OnnxOpsetImport = struct {
    domain: []const u8 = "",
    version: i64 = 0,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.domain.len > 0) {
            alloc.free(_.domain);
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

    pub fn findInitializer(name: []const u8): (*const OnnxTensor)? {
        let i: usize = 0;
        while (i < _.initializers.len) {
            if (std.mem.eql(u8, _.initializers[i].name, name)) {
                return &_.initializers[i];
            }
            i += 1;
        }
        return null;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) {
            alloc.free(_.name);
        }
        for (n of _.nodes) {
            let item = n;
            item.deinit(alloc);
        }
        if (_.nodes.len > 0) {
            alloc.free(_.nodes);
        }
        for (vi of _.inputs) {
            let item = vi;
            item.deinit(alloc);
        }
        if (_.inputs.len > 0) {
            alloc.free(_.inputs);
        }
        for (vo of _.outputs) {
            let item = vo;
            item.deinit(alloc);
        }
        if (_.outputs.len > 0) {
            alloc.free(_.outputs);
        }
        for (t of _.initializers) {
            let item = t;
            item.deinit(alloc);
        }
        if (_.initializers.len > 0) {
            alloc.free(_.initializers);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
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
    graph: OnnxGraph? = null,

    pub fn findInitializer(name: []const u8): (*const OnnxTensor)? {
        if (g := _.graph) {
            return g.findInitializer(name);
        }
        return null;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (op of _.opset_import) {
            let item = op;
            item.deinit(alloc);
        }
        if (_.opset_import.len > 0) {
            alloc.free(_.opset_import);
        }
        if (_.producer_name.len > 0) {
            alloc.free(_.producer_name);
        }
        if (_.producer_version.len > 0) {
            alloc.free(_.producer_version);
        }
        if (_.domain.len > 0) {
            alloc.free(_.domain);
        }
        if (_.doc_string.len > 0) {
            alloc.free(_.doc_string);
        }
        if (*g := _.graph) {
            g.deinit(alloc);
        }
    }
};
