// Source: cp/lib/onnx/codec.cp (ONNX Protobuf encoder/decoder & DAG cycle validator)
const std = @import("std");
import { OnnxError, PbWireReader, PbWireWriter } from "./wire.cp";
import {
    OnnxDataType,
    OnnxDim,
    OnnxValueInfo,
    OnnxAttrType,
    OnnxAttribute,
    OnnxNode,
    OnnxTensor,
    OnnxOpsetImport,
    OnnxGraph,
    OnnxModel,
} from "./ir.cp";

// --- Helpers ---

fn dupeSlice(alloc: std.mem.Allocator, src: []const u8): ![]const u8 {
    if (src.len == 0) {
        return "";
    }
    const dst = try alloc.alloc(u8, src.len);
    @memcpy(dst, src);
    return dst;
}

// --- Encoder ---

fn encodeDim(alloc: std.mem.Allocator, dim: OnnxDim): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    switch (dim) {
        .dim_value => |v| try w.writeI64Field(1, v),
        .dim_param => |s| {
            if (s.len > 0) {
                try w.writeString(2, s);
            }
        },
    }
    return w.toOwnedSlice();
}

fn encodeTensorShape(alloc: std.mem.Allocator, dims: []const OnnxDim): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    for (d of dims) {
        const d_bytes = try encodeDim(alloc, d);
        defer alloc.free(d_bytes);
        try w.writeBytes(1, d_bytes);
    }
    return w.toOwnedSlice();
}

fn encodeTypeProtoTensor(alloc: std.mem.Allocator, elem_type: OnnxDataType, dims: []const OnnxDim): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    if (elem_type != .UNDEFINED) {
        try w.writeI64Field(1, @intFromEnum(elem_type));
    }
    if (dims.len > 0) {
        const shape_bytes = try encodeTensorShape(alloc, dims);
        defer alloc.free(shape_bytes);
        try w.writeBytes(2, shape_bytes);
    }
    return w.toOwnedSlice();
}

fn encodeTypeProto(alloc: std.mem.Allocator, elem_type: OnnxDataType, dims: []const OnnxDim): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    const tt_bytes = try encodeTypeProtoTensor(alloc, elem_type, dims);
    defer alloc.free(tt_bytes);
    try w.writeBytes(1, tt_bytes);
    return w.toOwnedSlice();
}

fn encodeValueInfo(alloc: std.mem.Allocator, vi: *const OnnxValueInfo): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    if (vi.name.len > 0) {
        try w.writeString(1, vi.name);
    }
    if (vi.elem_type != .UNDEFINED or vi.dims.len > 0) {
        const tp_bytes = try encodeTypeProto(alloc, vi.elem_type, vi.dims);
        defer alloc.free(tp_bytes);
        try w.writeBytes(2, tp_bytes);
    }
    if (vi.doc_string.len > 0) {
        try w.writeString(3, vi.doc_string);
    }
    return w.toOwnedSlice();
}

fn encodeAttribute(alloc: std.mem.Allocator, attr: *const OnnxAttribute): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    if (attr.name.len > 0) {
        try w.writeString(1, attr.name);
    }
    if (attr.doc_string.len > 0) {
        try w.writeString(13, attr.doc_string);
    }
    switch (attr.value) {
        .none => {},
        .float_val => |f| try w.writeF32Field(2, f),
        .int_val => |i| try w.writeI64Field(3, i),
        .string_val => |s| try w.writeBytes(4, s),
        .floats => |fs| {
            for (f of fs) {
                try w.writeF32Field(7, f);
            }
        },
        .ints => |is| {
            for (i of is) {
                try w.writeI64Field(8, i);
            }
        },
        .strings => |ss| {
            for (s of ss) {
                try w.writeBytes(9, s);
            }
        },
    }
    const at = attr.attrType();
    if (at != .UNDEFINED) {
        try w.writeI64Field(20, @intFromEnum(at));
    }
    return w.toOwnedSlice();
}

fn encodeNode(alloc: std.mem.Allocator, node: *const OnnxNode): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    for (inp of node.inputs) {
        try w.writeString(1, inp);
    }
    for (out of node.outputs) {
        try w.writeString(2, out);
    }
    if (node.name.len > 0) {
        try w.writeString(3, node.name);
    }
    if (node.op_type.len > 0) {
        try w.writeString(4, node.op_type);
    }
    for (attr of node.attributes) {
        const ab = try encodeAttribute(alloc, &attr);
        defer alloc.free(ab);
        try w.writeBytes(5, ab);
    }
    if (node.doc_string.len > 0) {
        try w.writeString(6, node.doc_string);
    }
    if (node.domain.len > 0) {
        try w.writeString(7, node.domain);
    }
    return w.toOwnedSlice();
}

fn encodeTensor(alloc: std.mem.Allocator, t: *const OnnxTensor): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    for (d of t.dims) {
        try w.writeI64Field(1, d);
    }
    if (t.data_type != .UNDEFINED) {
        try w.writeI64Field(2, @intFromEnum(t.data_type));
    }
    for (f of t.float_data) {
        try w.writeF32Field(4, f);
    }
    for (i of t.int32_data) {
        try w.writeI64Field(5, @intCast(i));
    }
    if (t.int64_data.len > 0) {
        for (i of t.int64_data) {
            try w.writeI64Field(7, i);
        }
    }
    if (t.name.len > 0) {
        try w.writeString(8, t.name);
    }
    if (t.raw_data.len > 0) {
        try w.writeBytes(9, t.raw_data);
    }
    if (t.doc_string.len > 0) {
        try w.writeString(12, t.doc_string);
    }
    return w.toOwnedSlice();
}

fn encodeGraph(alloc: std.mem.Allocator, g: *const OnnxGraph): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    for (n of g.nodes) {
        const nb = try encodeNode(alloc, &n);
        defer alloc.free(nb);
        try w.writeBytes(1, nb);
    }
    if (g.name.len > 0) {
        try w.writeString(2, g.name);
    }
    for (t of g.initializers) {
        const tb = try encodeTensor(alloc, &t);
        defer alloc.free(tb);
        try w.writeBytes(5, tb);
    }
    if (g.doc_string.len > 0) {
        try w.writeString(10, g.doc_string);
    }
    for (vi of g.inputs) {
        const vib = try encodeValueInfo(alloc, &vi);
        defer alloc.free(vib);
        try w.writeBytes(11, vib);
    }
    for (vo of g.outputs) {
        const vob = try encodeValueInfo(alloc, &vo);
        defer alloc.free(vob);
        try w.writeBytes(12, vob);
    }
    return w.toOwnedSlice();
}

fn encodeOpsetImport(alloc: std.mem.Allocator, op: *const OnnxOpsetImport): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    if (op.domain.len > 0) {
        try w.writeString(1, op.domain);
    }
    if (op.version != 0) {
        try w.writeI64Field(2, op.version);
    }
    return w.toOwnedSlice();
}

pub fn encodeOnnxModel(alloc: std.mem.Allocator, model: *const OnnxModel): ![]u8 {
    let w = PbWireWriter.init(alloc);
    errdefer w.deinit();
    if (model.ir_version != 0) {
        try w.writeI64Field(1, model.ir_version);
    }
    if (model.producer_name.len > 0) {
        try w.writeString(2, model.producer_name);
    }
    if (model.producer_version.len > 0) {
        try w.writeString(3, model.producer_version);
    }
    if (model.domain.len > 0) {
        try w.writeString(4, model.domain);
    }
    if (model.model_version != 0) {
        try w.writeI64Field(5, model.model_version);
    }
    if (model.doc_string.len > 0) {
        try w.writeString(6, model.doc_string);
    }
    if (g := model.graph) {
        const gb = try encodeGraph(alloc, &g);
        defer alloc.free(gb);
        try w.writeBytes(7, gb);
    }
    for (op of model.opset_import) {
        const opb = try encodeOpsetImport(alloc, &op);
        defer alloc.free(opb);
        try w.writeBytes(8, opb);
    }
    return w.toOwnedSlice();
}

// --- Decoder ---

fn decodeDim(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxDim {
    let dim: OnnxDim = .{ .dim_value = 0 };
    errdefer {
        switch (dim) {
            .dim_param => |s| {
                if (s.len > 0) {
                    alloc.free(s);
                }
            },
            .dim_value => {},
        }
    }
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                dim = .{ .dim_value = try reader.readI64() };
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                const s = try reader.readBytes();
                dim = .{ .dim_param = try dupeSlice(alloc, s) };
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
    return dim;
}

fn decodeTensorShape(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): ![]OnnxDim {
    let dims = std.ArrayList(OnnxDim).empty;
    errdefer {
        for (d of dims.items) {
            switch (d) {
                .dim_param => |s| {
                    if (s.len > 0) {
                        alloc.free(s);
                    }
                },
                .dim_value => {},
            }
        }
        dims.deinit(alloc);
    }
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                const d = try decodeDim(alloc, &sub, depth + 1);
                try dims.append(alloc, d);
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
    return dims.toOwnedSlice(alloc);
}

fn decodeTypeProtoTensor(
    alloc: std.mem.Allocator,
    reader: *PbWireReader,
    depth: usize,
    vi: *OnnxValueInfo,
): !void {
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                vi.elem_type = OnnxDataType.fromI32(try reader.readI32());
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                vi.dims = try decodeTensorShape(alloc, &sub, depth + 1);
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
}

fn decodeTypeProto(
    alloc: std.mem.Allocator,
    reader: *PbWireReader,
    depth: usize,
    vi: *OnnxValueInfo,
): !void {
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try decodeTypeProtoTensor(alloc, &sub, depth + 1, vi);
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
}

fn decodeValueInfo(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxValueInfo {
    let vi = OnnxValueInfo{};
    errdefer vi.deinit(alloc);
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                vi.name = try dupeSlice(alloc, try reader.readBytes());
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try decodeTypeProto(alloc, &sub, depth + 1, &vi);
            },
            3 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                vi.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
    return vi;
}

fn decodeAttribute(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxAttribute {
    let attr = OnnxAttribute{};
    errdefer attr.deinit(alloc);

    let floats_list = std.ArrayList(f32).empty;
    errdefer floats_list.deinit(alloc);
    let ints_list = std.ArrayList(i64).empty;
    errdefer ints_list.deinit(alloc);
    let strings_list = std.ArrayList(string).empty;
    errdefer {
        for (s of strings_list.items) {
            if (s.len > 0) {
                alloc.free(s);
            }
        }
        strings_list.deinit(alloc);
    }

    let declared_type: i32 = 0;

    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                attr.name = try dupeSlice(alloc, try reader.readBytes());
            },
            2 => {
                if (tag.wire_type != .fixed32) {
                    return OnnxError.InvalidWireType;
                }
                attr.value = .{ .float_val = try reader.readF32() };
            },
            3 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                attr.value = .{ .int_val = try reader.readI64() };
            },
            4 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                attr.value = .{ .string_val = try dupeSlice(alloc, try reader.readBytes()) };
            },
            7 => {
                if (tag.wire_type == .fixed32) {
                    try floats_list.append(alloc, try reader.readF32());
                } else if (tag.wire_type == .length_delimited) {
                    const packed_bytes = try reader.readBytes();
                    let pr = PbWireReader.init(packed_bytes);
                    while (!pr.isEof()) {
                        try floats_list.append(alloc, try pr.readF32());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            8 => {
                if (tag.wire_type == .varint) {
                    try ints_list.append(alloc, try reader.readI64());
                } else if (tag.wire_type == .length_delimited) {
                    const packed_bytes = try reader.readBytes();
                    let pr = PbWireReader.init(packed_bytes);
                    while (!pr.isEof()) {
                        try ints_list.append(alloc, try pr.readI64());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            9 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                try strings_list.append(alloc, try dupeSlice(alloc, try reader.readBytes()));
            },
            13 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                attr.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            20 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                declared_type = try reader.readI32();
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }

    if (floats_list.items.len > 0 or declared_type == @intFromEnum(OnnxAttrType.FLOATS)) {
        attr.value = .{ .floats = try floats_list.toOwnedSlice(alloc) };
    } else {
        floats_list.deinit(alloc);
    }

    if (ints_list.items.len > 0 or declared_type == @intFromEnum(OnnxAttrType.INTS)) {
        attr.value = .{ .ints = try ints_list.toOwnedSlice(alloc) };
    } else {
        ints_list.deinit(alloc);
    }

    if (strings_list.items.len > 0 or declared_type == @intFromEnum(OnnxAttrType.STRINGS)) {
        attr.value = .{ .strings = try strings_list.toOwnedSlice(alloc) };
    } else {
        strings_list.deinit(alloc);
    }

    return attr;
}

fn decodeNode(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxNode {
    let node = OnnxNode{};
    errdefer node.deinit(alloc);

    let inputs = std.ArrayList(string).empty;
    errdefer {
        for (s of inputs.items) {
            if (s.len > 0) {
                alloc.free(s);
            }
        }
        inputs.deinit(alloc);
    }
    let outputs = std.ArrayList(string).empty;
    errdefer {
        for (s of outputs.items) {
            if (s.len > 0) {
                alloc.free(s);
            }
        }
        outputs.deinit(alloc);
    }
    let attrs = std.ArrayList(OnnxAttribute).empty;
    errdefer {
        for (*a of attrs.items) {
            a.deinit(alloc);
        }
        attrs.deinit(alloc);
    }

    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                try inputs.append(alloc, try dupeSlice(alloc, try reader.readBytes()));
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                try outputs.append(alloc, try dupeSlice(alloc, try reader.readBytes()));
            },
            3 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                node.name = try dupeSlice(alloc, try reader.readBytes());
            },
            4 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                node.op_type = try dupeSlice(alloc, try reader.readBytes());
            },
            5 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try attrs.append(alloc, try decodeAttribute(alloc, &sub, depth + 1));
            },
            6 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                node.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            7 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                node.domain = try dupeSlice(alloc, try reader.readBytes());
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }

    node.inputs = try inputs.toOwnedSlice(alloc);
    node.outputs = try outputs.toOwnedSlice(alloc);
    node.attributes = try attrs.toOwnedSlice(alloc);
    return node;
}

fn decodeTensor(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxTensor {
    let t = OnnxTensor{};
    errdefer t.deinit(alloc);

    let dims = std.ArrayList(i64).empty;
    errdefer dims.deinit(alloc);
    let floats = std.ArrayList(f32).empty;
    errdefer floats.deinit(alloc);
    let int32s = std.ArrayList(i32).empty;
    errdefer int32s.deinit(alloc);
    let int64s = std.ArrayList(i64).empty;
    errdefer int64s.deinit(alloc);

    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type == .varint) {
                    try dims.append(alloc, try reader.readI64());
                } else if (tag.wire_type == .length_delimited) {
                    let pr = PbWireReader.init(try reader.readBytes());
                    while (!pr.isEof()) {
                        try dims.append(alloc, try pr.readI64());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            2 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                t.data_type = OnnxDataType.fromI32(try reader.readI32());
            },
            4 => {
                if (tag.wire_type == .fixed32) {
                    try floats.append(alloc, try reader.readF32());
                } else if (tag.wire_type == .length_delimited) {
                    let pr = PbWireReader.init(try reader.readBytes());
                    while (!pr.isEof()) {
                        try floats.append(alloc, try pr.readF32());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            5 => {
                if (tag.wire_type == .varint) {
                    try int32s.append(alloc, try reader.readI32());
                } else if (tag.wire_type == .length_delimited) {
                    let pr = PbWireReader.init(try reader.readBytes());
                    while (!pr.isEof()) {
                        try int32s.append(alloc, try pr.readI32());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            7 => {
                if (tag.wire_type == .varint) {
                    try int64s.append(alloc, try reader.readI64());
                } else if (tag.wire_type == .length_delimited) {
                    let pr = PbWireReader.init(try reader.readBytes());
                    while (!pr.isEof()) {
                        try int64s.append(alloc, try pr.readI64());
                    }
                } else {
                    return OnnxError.InvalidWireType;
                }
            },
            8 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                t.name = try dupeSlice(alloc, try reader.readBytes());
            },
            9 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                t.raw_data = try dupeSlice(alloc, try reader.readBytes());
            },
            12 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                t.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }

    t.dims = try dims.toOwnedSlice(alloc);
    t.float_data = try floats.toOwnedSlice(alloc);
    t.int32_data = try int32s.toOwnedSlice(alloc);
    t.int64_data = try int64s.toOwnedSlice(alloc);
    return t;
}

fn decodeGraph(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxGraph {
    let g = OnnxGraph{};
    errdefer g.deinit(alloc);

    let nodes = std.ArrayList(OnnxNode).empty;
    errdefer {
        for (*n of nodes.items) {
            n.deinit(alloc);
        }
        nodes.deinit(alloc);
    }
    let inputs = std.ArrayList(OnnxValueInfo).empty;
    errdefer {
        for (*vi of inputs.items) {
            vi.deinit(alloc);
        }
        inputs.deinit(alloc);
    }
    let outputs = std.ArrayList(OnnxValueInfo).empty;
    errdefer {
        for (*vo of outputs.items) {
            vo.deinit(alloc);
        }
        outputs.deinit(alloc);
    }
    let inits = std.ArrayList(OnnxTensor).empty;
    errdefer {
        for (*t of inits.items) {
            t.deinit(alloc);
        }
        inits.deinit(alloc);
    }

    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try nodes.append(alloc, try decodeNode(alloc, &sub, depth + 1));
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                g.name = try dupeSlice(alloc, try reader.readBytes());
            },
            5 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try inits.append(alloc, try decodeTensor(alloc, &sub, depth + 1));
            },
            10 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                g.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            11 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try inputs.append(alloc, try decodeValueInfo(alloc, &sub, depth + 1));
            },
            12 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(depth);
                try outputs.append(alloc, try decodeValueInfo(alloc, &sub, depth + 1));
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }

    g.nodes = try nodes.toOwnedSlice(alloc);
    g.inputs = try inputs.toOwnedSlice(alloc);
    g.outputs = try outputs.toOwnedSlice(alloc);
    g.initializers = try inits.toOwnedSlice(alloc);
    return g;
}

fn decodeOpsetImport(alloc: std.mem.Allocator, reader: *PbWireReader, depth: usize): !OnnxOpsetImport {
    let op = OnnxOpsetImport{};
    errdefer op.deinit(alloc);
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                op.domain = try dupeSlice(alloc, try reader.readBytes());
            },
            2 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                op.version = try reader.readI64();
            },
            else => try reader.skipField(tag.wire_type, depth + 1),
        }
    }
    return op;
}

pub fn decodeOnnxModel(alloc: std.mem.Allocator, bytes: []const u8): !OnnxModel {
    let model = OnnxModel{ .ir_version = 0 };
    errdefer model.deinit(alloc);

    let opsets = std.ArrayList(OnnxOpsetImport).empty;
    errdefer {
        for (*op of opsets.items) {
            op.deinit(alloc);
        }
        opsets.deinit(alloc);
    }

    let reader = PbWireReader.init(bytes);
    while (tag := try reader.readTag()) {
        switch (tag.field_number) {
            1 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                model.ir_version = try reader.readI64();
            },
            2 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                model.producer_name = try dupeSlice(alloc, try reader.readBytes());
            },
            3 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                model.producer_version = try dupeSlice(alloc, try reader.readBytes());
            },
            4 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                model.domain = try dupeSlice(alloc, try reader.readBytes());
            },
            5 => {
                if (tag.wire_type != .varint) {
                    return OnnxError.InvalidWireType;
                }
                model.model_version = try reader.readI64();
            },
            6 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                model.doc_string = try dupeSlice(alloc, try reader.readBytes());
            },
            7 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(0);
                model.graph = try decodeGraph(alloc, &sub, 1);
            },
            8 => {
                if (tag.wire_type != .length_delimited) {
                    return OnnxError.InvalidWireType;
                }
                let sub = try reader.enterSubmessage(0);
                try opsets.append(alloc, try decodeOpsetImport(alloc, &sub, 1));
            },
            else => try reader.skipField(tag.wire_type, 1),
        }
    }

    model.opset_import = try opsets.toOwnedSlice(alloc);
    return model;
}

// --- Graph Validation (DAG Acyclic & Duplicate Output Check) ---

const VisitState = enum(u8) {
    unvisited = 0,
    visiting = 1,
    visited = 2,
};

fn dfsCheckCycle(
    node_idx: usize,
    nodes: []const OnnxNode,
    producer_map: *const std.StringHashMapUnmanaged(usize),
    states: []VisitState,
): !void {
    states[node_idx] = .visiting;
    const node = &nodes[node_idx];
    for (inp of node.inputs) {
        if (inp.len == 0) {
            continue;
        }
        if (prod_idx := producer_map.get(inp)) {
            if (states[prod_idx] == .visiting) {
                return OnnxError.CyclicGraph;
            }
            if (states[prod_idx] == .unvisited) {
                try dfsCheckCycle(prod_idx, nodes, producer_map, states);
            }
        }
    }
    states[node_idx] = .visited;
}

pub fn validateGraphAcyclic(alloc: std.mem.Allocator, graph: *const OnnxGraph): !void {
    if (graph.nodes.len == 0) {
        return;
    }
    let producer_map = std.StringHashMapUnmanaged(usize).empty;
    defer producer_map.deinit(alloc);

    let idx: usize = 0;
    while (idx < graph.nodes.len) {
        const node = &graph.nodes[idx];
        for (out of node.outputs) {
            if (out.len == 0) {
                continue;
            }
            const gop = try producer_map.getOrPut(alloc, out);
            if (gop.found_existing) {
                return OnnxError.DuplicateOutput;
            }
            gop.value_ptr.* = idx;
        }
        idx += 1;
    }

    const states = try alloc.alloc(VisitState, graph.nodes.len);
    defer alloc.free(states);
    @memset(states, .unvisited);

    let i: usize = 0;
    while (i < graph.nodes.len) {
        if (states[i] == .unvisited) {
            try dfsCheckCycle(i, graph.nodes, &producer_map, states);
        }
        i += 1;
    }
}
