# `cap:gguf` / `cap.gguf` 标准库参考手册

## 1. 概述

`lib/gguf/`（对外协议 `cap:gguf` 与 `cap.gguf`）是纯 CP 实现的工业级 GGUF v2/v3（llama.cpp 本地大模型权重与元数据二进制格式）解析、构建与块量化/反量化标准库。

- **零外部依赖**：纯 CP / Zig 标准库实现，无 C/C++ 绑定或系统调用依赖。
- **防畸形报文与资源耗尽保护**：内置最大嵌套深度（`GGUF_MAX_NESTING_DEPTH = 8`）、最大数组长度（`GGUF_MAX_ARRAY_LEN`）、最大字符串长度（`GGUF_MAX_STRING_LEN`）、KV/张量数量上限及张量偏移/字节区间溢出与越界校验。
- **零拷贝张量切片**：`GgufFile.tensorData()` 直接返回指向原始权重缓冲区的只读零拷贝切片。
- **块反量化**：原生支持 GGML 标准 `Q8_0`（34 字节/块 -> 32×`f32`）与 `Q4_0`（18 字节/块 -> 32×`f32`）量化与精确反量化。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `GgufError` | 错误集合：`InvalidMagic`, `UnsupportedVersion`, `InvalidAlignment`, `UnexpectedEof`, `InvalidValueType`, `InvalidString`, `NestingTooDeep`, `ArrayTooLarge`, `TooManyKvs`, `TooManyTensors`, `TooManyDims`, `InvalidTensorType`, `TensorOutOfBounds`, `OutOfMemory` |
| `GgufValueType` | 13 种 GGUF 元数据类型枚举（`U8=0` .. `F64=12`），提供 `fromU32()` |
| `GgmlType` | GGML 张量存储类型枚举（`F32`, `F16`, `Q4_0`, `Q8_0`, `BF16`, `Q4_K` 等），提供 `blockSize()`, `typeSize()`, `byteSize()` |
| `GgufValue` | 带标签联合体，承载 13 种 KV 值，提供 `valueType()`, `clone()`, `deinit()` |
| `GgufTensorInfo` | 张量描述符（`name`, `n_dims`, `dims`, `ggml_type`, `offset`），提供 `numElements()`, `byteSize()` |
| `GgufFile` | 已解析 GGUF 文件视图，提供 `deinit()`, `getKv()`, `getString()`, `getU32()`, `getU64()`, `getF32()`, `getBool()`, `getTensor()`, `tensorData()` |
| `GgufBuilder` | GGUF v3 二进制构建器，自动计算 `general.alignment` 对齐偏移与填充字节 |

## 3. API 速查

- **解析**：
  - `parseGguf(alloc: Allocator, bytes: []const u8): GgufError!GgufFile`
- **构建**：
  - `GgufBuilder.init(alloc: Allocator): GgufBuilder`
  - `builder.setAlignment(alignment: u64): GgufError!void`
  - `builder.addString(key, val)` / `addU32` / `addI32` / `addU64` / `addF32` / `addBool` / `addStringArray` / `addKv`
  - `builder.addTensor(name, dims, ggml_type, data): GgufError!void`
  - `builder.build(alloc: Allocator): GgufError![]u8`
  - `builder.deinit()`
- **量化与反量化**：
  - `dequantizeQ8_0Block(raw: []const u8): GgufError![32]f32`
  - `dequantizeQ4_0Block(raw: []const u8): GgufError![32]f32`
  - `quantizeQ8_0Block(src: *const [32]f32): [34]u8`
  - `quantizeQ4_0Block(src: *const [32]f32): [18]u8`

## 4. 示例

```typescript
import { GgufBuilder, GgmlType, parseGguf, dequantizeQ8_0Block } from "cap:gguf";

let builder = GgufBuilder.init(alloc);
defer builder.deinit();

try builder.addString("general.architecture", "llama");
try builder.addU32("llama.context_length", 4096);

const raw_bytes = try builder.build(alloc);
defer alloc.free(raw_bytes);

let file = try parseGguf(alloc, raw_bytes);
defer file.deinit(alloc);
```
