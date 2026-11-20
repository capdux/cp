# `cap:onnx` — ONNX 计算图与权重模型二进制解析/构建标准库

## 1. 概述

`cap:onnx`（动态挂载为 `cap.onnx`）是纯 CP 实现的工业级 ONNX（Open Neural Network Exchange）`.onnx` 模型二进制编解码与计算图校验标准库。

- **零外部依赖**：内置确定性 Protobuf Wire 读写原语（`PbWireReader` / `PbWireWriter`），支持 Varint、Fixed32/64 与 Length-Delimited 切片读取。
- **纵深防炸弹**：内置 `MAX_PROTO_DEPTH = 32` 递归嵌套深度限制、Varint 溢出检测与长度越界拦截。
- **完整 IR 表达**：覆盖 `ModelProto`、`GraphProto`、`NodeProto`、`TensorProto`、`ValueInfoProto`、`AttributeProto` 与 `OperatorSetIdProto`。
- **计算图拓扑校验**：`validateGraphAcyclic` 基于三色 DFS 与输出名冲突检测，精确拦截循环依赖（`CyclicGraph`）与重名输出（`DuplicateOutput`）。

---

## 2. 核心类型

- **`OnnxError`**：`TruncatedVarint | VarintOverflow | UnexpectedEof | InvalidWireType | InvalidFieldNumber | MaxDepthExceeded | LengthOutOfBounds | InvalidUtf8 | InvalidDataType | MissingGraph | CyclicGraph | DuplicateOutput | OutOfMemory`
- **`PbWireReader` / `PbWireWriter`**：确定性 Protobuf Wire 层读写器，支持 `readVarint`、`readI64`、`readF32`、`readF64`、`readBytes`、`readTag`、`enterSubmessage`、`skipField` 等原语。
- **`OnnxDataType`**：标准 `TensorProto.DataType` 枚举（`FLOAT = 1`、`UINT8 = 2`、`INT8 = 3`、`INT32 = 6`、`INT64 = 7`、`BOOL = 9`、`FLOAT16 = 10`、`DOUBLE = 11`、`BFLOAT16 = 16` 等）。
- **`OnnxDim` / `OnnxValueInfo`**：张量维度（静态 `dim_value: i64` 与动态符号 `dim_param: []const u8`）与输入/输出类型描述。
- **`OnnxAttrValue` / `OnnxAttribute`**：支持 `FLOAT`、`INT`、`STRING`、`FLOATS`、`INTS`、`STRINGS` 属性。
- **`OnnxTensor`**：支持 `raw_data` 与 `float_data` / `int32_data` / `int64_data` packed repeated 编码，提供 `extractFloats`、`extractInt32s`、`extractInt64s` 与 `elementCount`。
- **`OnnxNode` / `OnnxGraph` / `OnnxModel` / `OnnxOpsetImport`**：计算图节点、图容器与顶层模型，提供 `findInitializer(name)` 与 `deinit(alloc)`。

---

## 3. API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `encodeOnnxModel` | `(alloc, model: *const OnnxModel): OnnxError![]u8` | 将 `OnnxModel` 序列化为 `.onnx` Protobuf 字节流 |
| `decodeOnnxModel` | `(alloc, bytes: []const u8): OnnxError!OnnxModel` | 从 `.onnx` 字节流解析 `OnnxModel` |
| `validateGraphAcyclic` | `(alloc, graph: *const OnnxGraph): OnnxError!void` | 校验计算图是否为合法 DAG 且无重名输出 |
| `OnnxModel.findInitializer` | `(self, name: []const u8): ?*const OnnxTensor` | 按名称检索权重初始化器 |
| `OnnxTensor.extractFloats` | `(self, alloc): OnnxError![]f32` | 提取 `FLOAT` 权重（兼容 `raw_data` 与 `float_data`） |
| `OnnxTensor.extractInt64s` | `(self, alloc): OnnxError![]i64` | 提取 `INT64` 权重（兼容 `raw_data` 与 `int64_data`） |

---

## 4. 示例

```cp
import {
    OnnxModel, OnnxGraph, OnnxNode, OnnxValueInfo, OnnxDim,
    OnnxTensor, OnnxAttribute, OnnxOpsetImport, OnnxDataType,
    encodeOnnxModel, decodeOnnxModel, validateGraphAcyclic,
} from "cap:onnx";

fn roundtrip(alloc: std.mem.Allocator): !void {
    let opsets = [_]OnnxOpsetImport{ .{ .domain = "", .version = 17 } };
    let dims = [_]OnnxDim{ .{ .dim_param = "batch" }, .{ .dim_value = 3 } };
    let inputs = [_]OnnxValueInfo{ .{ .name = "X", .elem_type = .FLOAT, .dims = &dims } };
    let outputs = [_]OnnxValueInfo{ .{ .name = "Y", .elem_type = .FLOAT, .dims = &dims } };
    let nodes = [_]OnnxNode{ .{
        .name = "relu0",
        .op_type = "Relu",
        .inputs = &[_][]const u8{"X"},
        .outputs = &[_][]const u8{"Y"},
    } };
    let model = OnnxModel{
        .ir_version = 8,
        .opset_import = &opsets,
        .graph = OnnxGraph{
            .name = "simple_relu",
            .nodes = &nodes,
            .inputs = &inputs,
            .outputs = &outputs,
        },
    };
    const bytes = try encodeOnnxModel(alloc, &model);
    defer alloc.free(bytes);

    let decoded = try decodeOnnxModel(alloc, bytes);
    defer decoded.deinit(alloc);

    if (decoded.graph) |*g| {
        try validateGraphAcyclic(alloc, g);
    }
}
```
