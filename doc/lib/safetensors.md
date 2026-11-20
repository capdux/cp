# `cap:safetensors` — Safetensors 模型权重格式与 BF16/FP16 转换库

## 概述

`cap:safetensors`（以及动态挂载入口 `cap.safetensors`）提供纯 CP 实现的工业级 HuggingFace Safetensors 二进制格式解析、零拷贝张量视图读取、序列化构建，以及符合 IEEE-754 标准的 BF16 / FP16 位级数值转换。

解析器内置多层安全边界校验：限制最大头部长度（100 MiB）防资源耗尽、校验 `data_offsets` 越界、`shape` 乘积防溢出与字节长度一致性检查，以及张量数据区间重叠（Overlap）拦截。

## 核心类型

- **`Dtype`**：支持全部标准数据类型 `F32`, `F64`, `F16`, `BF16`, `I8`, `U8`, `I16`, `U16`, `I32`, `U32`, `I64`, `U64`, `BOOL`。
- **`SafetensorsError`**：涵盖 `InvalidHeaderLength`, `HeaderTooLarge`, `InvalidJson`, `InvalidDtype`, `InvalidShape`, `InvalidOffsets`, `OutOfBounds`, `ShapeSizeMismatch`, `OverlappingTensors`, `TensorNotFound`, `UnsupportedConversion`, `DuplicateTensorName`, `OutOfMemory`。
- **`TensorInfo` / `TensorView`**：分别描述张量元信息与直接切片到原始缓冲区内部的零拷贝只读视图（`data: []const u8`）。
- **`SafetensorsFile`**：解析后的模型文件句柄，提供 `getTensor`、`getMetadata`、`tensorAsF32` 与 `deinit`。
- **`SafetensorsBuilder`**：用于组装多张量与 `__metadata__` 并生成 8 字节对齐 JSON 头的序列化构建器。

## API 速查

| 函数 / 方法 | 说明 |
| --- | --- |
| `dtypeSize(dt: Dtype): usize` | 返回指定数据类型的单元素字节宽度 |
| `parseDtype(name: []const u8): Dtype?` | 将字符串（如 `"BF16"`）解析为 `Dtype` |
| `bf16ToF32(bits: u16): f32` / `f32ToBf16(val: f32): u16` | BF16 与 F32 位级互转（含 round-to-nearest-even） |
| `f16ToF32(bits: u16): f32` / `f32ToF16(val: f32): u16` | IEEE-754 FP16 与 F32 互转（含次正规数/Inf/NaN） |
| `parseSafetensors(alloc, bytes): !SafetensorsFile` | 安全解析 Safetensors 字节流并返回零拷贝视图集合 |
| `file.getTensor(name): TensorView?` | 按名称获取张量零拷贝视图 |
| `file.tensorAsF32(alloc, name): ![]f32` | 自动将 `F32`/`BF16`/`F16` 张量转换为 `[]f32` |
| `builder.addMetadata(key, value)` | 添加 `__metadata__` 键值对 |
| `builder.addTensorF32` / `addTensorBF16FromF32` / `addTensorF16FromF32` / `addTensorI32` | 添加对应类型的张量数据 |
| `builder.build(alloc): ![]u8` | 生成完整的 Safetensors 二进制文件字节流 |

## 示例

```cp
import { SafetensorsBuilder, parseSafetensors } from "cap:safetensors";

let builder = SafetensorsBuilder.init(alloc);
defer builder.deinit();

try builder.addMetadata("format", "pt");
try builder.addTensorBF16FromF32("weight", &[_]usize{ 2, 2 }, &[_]f32{ 1.0, -2.5, 3.25, 0.0 });
const bytes = try builder.build(alloc);
defer alloc.free(bytes);

let sf = try parseSafetensors(alloc, bytes);
defer sf.deinit();
const f32_weights = try sf.tensorAsF32(alloc, "weight");
defer alloc.free(f32_weights);
```
