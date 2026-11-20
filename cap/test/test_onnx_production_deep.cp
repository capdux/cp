// Source: cp/cap/test/test_onnx_production_deep.cp (Production deep tests for cap:onnx & cap.onnx)
const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    MAX_PROTO_DEPTH,
    OnnxError,
    PbWireReader,
    OnnxDataType,
    OnnxDim,
    OnnxValueInfo,
    OnnxAttribute,
    OnnxNode,
    OnnxTensor,
    OnnxOpsetImport,
    OnnxGraph,
    OnnxModel,
    encodeOnnxModel,
    decodeOnnxModel,
    validateGraphAcyclic,
} from "cap:onnx";

const Gpa = std.heap.DebugAllocator(.{});

fn case1Roundtrip(): !void {
    let gpa = Gpa{};
    defer {
        if (gpa.deinit() == .leak) {
            @panic("memory leak detected");
        }
    }
    const alloc = gpa.allocator();

    const opsets: [1]OnnxOpsetImport = [
        OnnxOpsetImport{ .domain = "", .version = 17 },
    ];

    const in_dims: [4]OnnxDim = [
        OnnxDim{ .dim_param = "batch" },
        OnnxDim{ .dim_value = 3 },
        OnnxDim{ .dim_value = 224 },
        OnnxDim{ .dim_value = 224 },
    ];
    const out_dims: [2]OnnxDim = [
        OnnxDim{ .dim_param = "batch" },
        OnnxDim{ .dim_value = 1000 },
    ];

    const inputs: [1]OnnxValueInfo = [
        OnnxValueInfo{ .name = "input_x", .elem_type = .FLOAT, .dims = &in_dims },
    ];
    const outputs: [1]OnnxValueInfo = [
        OnnxValueInfo{ .name = "logits", .elem_type = .FLOAT, .dims = &out_dims },
    ];

    const conv_pads: [4]i64 = [ 1, 1, 1, 1 ];
    const conv_strides: [2]i64 = [ 1, 1 ];
    const conv_attrs: [3]OnnxAttribute = [
        OnnxAttribute{ .name = "pads", .value = .{ .ints = &conv_pads } },
        OnnxAttribute{ .name = "strides", .value = .{ .ints = &conv_strides } },
        OnnxAttribute{ .name = "auto_pad", .value = .{ .string_val = "NOTSET" } },
    ];

    const conv_in: [2][]const u8 = [ "input_x", "conv1.weight" ];
    const conv_out: [1][]const u8 = ["conv1_out"];
    const relu_in: [1][]const u8 = ["conv1_out"];
    const relu_out: [1][]const u8 = ["relu1_out"];
    const mm_in: [2][]const u8 = [ "relu1_out", "fc.weight" ];
    const mm_out: [1][]const u8 = ["logits"];

    const nodes: [3]OnnxNode = [
        OnnxNode{
            .name = "conv1",
            .op_type = "Conv",
            .inputs = &conv_in,
            .outputs = &conv_out,
            .attributes = &conv_attrs,
        },
        OnnxNode{
            .name = "relu1",
            .op_type = "Relu",
            .inputs = &relu_in,
            .outputs = &relu_out,
        },
        OnnxNode{
            .name = "fc1",
            .op_type = "MatMul",
            .inputs = &mm_in,
            .outputs = &mm_out,
        },
    ];

    let raw_w: [8]u8 = undefined;
    std.mem.writeInt(u32, raw_w[0..4], @bitCast(@as(f32, 1.5)), .little);
    std.mem.writeInt(u32, raw_w[4..8], @bitCast(@as(f32, -2.25)), .little);

    const float_w: [3]f32 = [ 0.5, 0.25, -0.75 ];
    const conv_dims: [4]i64 = [ 2, 1, 1, 1 ];
    const fc_dims: [1]i64 = [3];

    const initializers: [2]OnnxTensor = [
        OnnxTensor{
            .name = "conv1.weight",
            .dims = &conv_dims,
            .data_type = .FLOAT,
            .raw_data = &raw_w,
        },
        OnnxTensor{
            .name = "fc.weight",
            .dims = &fc_dims,
            .data_type = .FLOAT,
            .float_data = &float_w,
        },
    ];

    const model = OnnxModel{
        .ir_version = 8,
        .opset_import = &opsets,
        .producer_name = "cp_onnx",
        .producer_version = "1.0.0",
        .model_version = 1,
        .graph = OnnxGraph{
            .name = "vision_backbone",
            .nodes = &nodes,
            .inputs = &inputs,
            .outputs = &outputs,
            .initializers = &initializers,
        },
    };

    const encoded = try encodeOnnxModel(alloc, &model);
    defer alloc.free(encoded);

    let decoded = try decodeOnnxModel(alloc, encoded);
    defer decoded.deinit(alloc);

    expect_eq(decoded.ir_version, 8);
    expect_eq(decoded.opset_import.len, 1);
    expect_eq(decoded.opset_import[0].version, 17);
    try std.testing.expectEqualStrings("cp_onnx", decoded.producer_name);

    const g = &(decoded.graph orelse return error.MissingGraph);
    expect_eq(g.nodes.len, 3);
    try std.testing.expectEqualStrings("Conv", g.nodes[0].op_type);
    try std.testing.expectEqualStrings("Relu", g.nodes[1].op_type);
    try std.testing.expectEqualStrings("MatMul", g.nodes[2].op_type);
    expect_eq(g.inputs[0].elem_type, OnnxDataType.FLOAT);

    switch (g.inputs[0].dims[0]) {
        .dim_param => |s| try std.testing.expectEqualStrings("batch", s),
        .dim_value => return error.TestUnexpectedResult,
    }
    try validateGraphAcyclic(alloc, g);
}

fn case2Weights(): !void {
    let gpa = Gpa{};
    defer {
        if (gpa.deinit() == .leak) {
            @panic("memory leak detected");
        }
    }
    const alloc = gpa.allocator();

    let raw_f32: [8]u8 = undefined;
    std.mem.writeInt(u32, raw_f32[0..4], @bitCast(@as(f32, 3.25)), .little);
    std.mem.writeInt(u32, raw_f32[4..8], @bitCast(@as(f32, -4.5)), .little);

    let raw_i64: [16]u8 = undefined;
    std.mem.writeInt(u64, raw_i64[0..8], @bitCast(@as(i64, 1024)), .little);
    std.mem.writeInt(u64, raw_i64[8..16], @bitCast(@as(i64, -2048)), .little);

    const float_w: [3]f32 = [ 1.0, 2.0, 3.0 ];
    const dims2: [1]i64 = [2];
    const dims3: [1]i64 = [3];

    const initializers: [3]OnnxTensor = [
        OnnxTensor{ .name = "w_raw_f32", .dims = &dims2, .data_type = .FLOAT, .raw_data = &raw_f32 },
        OnnxTensor{ .name = "w_packed_f32", .dims = &dims3, .data_type = .FLOAT, .float_data = &float_w },
        OnnxTensor{ .name = "w_raw_i64", .dims = &dims2, .data_type = .INT64, .raw_data = &raw_i64 },
    ];

    const model = OnnxModel{
        .ir_version = 8,
        .graph = OnnxGraph{
            .name = "weights_only",
            .initializers = &initializers,
        },
    };

    const bytes = try encodeOnnxModel(alloc, &model);
    defer alloc.free(bytes);

    let decoded = try decodeOnnxModel(alloc, bytes);
    defer decoded.deinit(alloc);

    const t1 = decoded.findInitializer("w_raw_f32") orelse return error.TestUnexpectedResult;
    const f1 = try t1.extractFloats(alloc);
    defer alloc.free(f1);
    expect_eq(f1.len, 2);
    expect_eq(f1[0], 3.25);
    expect_eq(f1[1], -4.5);

    const t2 = decoded.findInitializer("w_packed_f32") orelse return error.TestUnexpectedResult;
    const f2 = try t2.extractFloats(alloc);
    defer alloc.free(f2);
    expect_eq(f2.len, 3);
    expect_eq(f2[2], 3.0);

    const t3 = decoded.findInitializer("w_raw_i64") orelse return error.TestUnexpectedResult;
    const ints3 = try t3.extractInt64s(alloc);
    defer alloc.free(ints3);
    expect_eq(ints3.len, 2);
    expect_eq(ints3[0], 1024);
    expect_eq(ints3[1], -2048);

    try std.testing.expect(decoded.findInitializer("missing_weight") == null);
    expect_eq(OnnxDataType.FLOAT16.elementSizeBytes(), 2);
    expect_eq(OnnxDataType.BFLOAT16.elementSizeBytes(), 2);
    expect_eq(OnnxDataType.DOUBLE.elementSizeBytes(), 8);
    expect_eq(OnnxDataType.BOOL.elementSizeBytes(), 1);
}

fn case3Acyclic(): !void {
    let gpa = Gpa{};
    defer {
        if (gpa.deinit() == .leak) {
            @panic("memory leak detected");
        }
    }
    const alloc = gpa.allocator();

    const n1_in: [1][]const u8 = ["x"];
    const n1_out: [1][]const u8 = ["h1"];
    const n2_in: [1][]const u8 = ["h1"];
    const n2_out: [1][]const u8 = ["y"];
    const dag_nodes: [2]OnnxNode = [
        OnnxNode{ .name = "n1", .op_type = "Relu", .inputs = &n1_in, .outputs = &n1_out },
        OnnxNode{ .name = "n2", .op_type = "Sigmoid", .inputs = &n2_in, .outputs = &n2_out },
    ];
    const dag = OnnxGraph{ .name = "dag", .nodes = &dag_nodes };
    try validateGraphAcyclic(alloc, &dag);

    const sl_in: [1][]const u8 = ["a"];
    const sl_out: [1][]const u8 = ["a"];
    const self_loop_nodes: [1]OnnxNode = [
        OnnxNode{ .name = "self", .op_type = "Identity", .inputs = &sl_in, .outputs = &sl_out },
    ];
    const self_loop = OnnxGraph{ .name = "self_loop", .nodes = &self_loop_nodes };
    try std.testing.expectError(OnnxError.CyclicGraph, validateGraphAcyclic(alloc, &self_loop));

    const c1_in: [1][]const u8 = ["b_out"];
    const c1_out: [1][]const u8 = ["a_out"];
    const c2_in: [1][]const u8 = ["a_out"];
    const c2_out: [1][]const u8 = ["b_out"];
    const cycle_nodes: [2]OnnxNode = [
        OnnxNode{ .name = "A", .op_type = "Add", .inputs = &c1_in, .outputs = &c1_out },
        OnnxNode{ .name = "B", .op_type = "Mul", .inputs = &c2_in, .outputs = &c2_out },
    ];
    const cycle_graph = OnnxGraph{ .name = "cycle", .nodes = &cycle_nodes };
    try std.testing.expectError(OnnxError.CyclicGraph, validateGraphAcyclic(alloc, &cycle_graph));
}

fn case4WireGuards(): !void {
    let gpa = Gpa{};
    defer {
        if (gpa.deinit() == .leak) {
            @panic("memory leak detected");
        }
    }
    const alloc = gpa.allocator();

    const trunc_bytes: [2]u8 = [ 0x08, 0x80 ];
    try std.testing.expectError(OnnxError.TruncatedVarint, decodeOnnxModel(alloc, &trunc_bytes));

    const oob_bytes: [4]u8 = [ 0x12, 100, 'a', 'b' ];
    try std.testing.expectError(OnnxError.LengthOutOfBounds, decodeOnnxModel(alloc, &oob_bytes));

    const nested_payload: [2]u8 = [ 0x12, 0x00 ];
    let reader = PbWireReader.init(&nested_payload);
    * = try reader.readTag();
    try std.testing.expectError(OnnxError.MaxDepthExceeded, reader.enterSubmessage(MAX_PROTO_DEPTH));
}

fn case5CapMount(): !void {
    let gpa = Gpa{};
    defer {
        if (gpa.deinit() == .leak) {
            @panic("memory leak detected");
        }
    }
    const alloc = gpa.allocator();

    let writer = cap.onnx.PbWireWriter.init(alloc);
    defer writer.deinit();
    try writer.writeI64Field(1, 9);
    try writer.writeString(2, "cap_mount_test");
    const raw = try writer.toOwnedSlice();
    defer alloc.free(raw);

    let model = try cap.onnx.decodeOnnxModel(alloc, raw);
    defer model.deinit(alloc);

    expect_eq(model.ir_version, 9);
    try std.testing.expectEqualStrings("cap_mount_test", model.producer_name);
}

fn main() {
    test("1. onnx model roundtrip with opset 17, dynamic dims, conv/relu/matmul and weights", () => {
        case1Roundtrip() catch |e| { std.debug.panic("test failed: {}", .{e}); };
    });

    test("2. onnx findInitializer and multi-dtype weight extraction", () => {
        case2Weights() catch |e| { std.debug.panic("test failed: {}", .{e}); };
    });

    test("3. onnx validateGraphAcyclic accepts DAG and rejects self-loop and multi-node cycle", () => {
        case3Acyclic() catch |e| { std.debug.panic("test failed: {}", .{e}); };
    });

    test("4. onnx protobuf wire boundary guards: truncated varint, out-of-bounds length, max depth", () => {
        case4WireGuards() catch |e| { std.debug.panic("test failed: {}", .{e}); };
    });

    test("5. cap.onnx dynamic mount and zero-leak allocator verification", () => {
        case5CapMount() catch |e| { std.debug.panic("test failed: {}", .{e}); };
    });
}
