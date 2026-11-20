# `cap:quant` — AI Embedding 向量量化标准库

## 1. 概述

`cap:quant`（亦可通过 `cap.quant` 动态挂载访问）为 CP 语言提供工业级、零外部依赖的 AI Embedding 向量量化引擎，支持三类核心压缩与检索算法：

- **Scalar Int8 量化**：对称量化（`[-127, 127]`）与仿射非对称量化（`u8` `0..255`），提供基于 `i32` 整数累加的 `dotInt8` 高速内积近似（4 倍内存压缩）。
- **Binary 1-bit 二值量化**：将 `f32` 向量按 `>= 0.0` 压缩为 `u64` 位图（32 倍内存压缩），结合硬件级 `@popCount` 实现单周期级 `hammingDistance` 与 `binaryCosineApprox` 检索。
- **Product Quantization (PQ) 乘积量化**：将 `D` 维向量切分为 `M` 个子空间（`D % M == 0`），通过确定性 K-Means 训练 `K <= 256` 质心码本，生成 `M` 字节紧凑编码及 $O(M)$ 复杂度的非对称距离查表（ADC）。

---

## 2. 核心类型

| 类型名 | 说明 | 关键字段 / 方法 |
| --- | --- | --- |
| `QuantError` | 量化错误集合 | `DimensionMismatch`, `InvalidSubspaceCount`, `EmptyInput`, `InvalidCodebook`, `OutOfMemory` |
| `ScalarQuantParams` | 标量量化参数 | `scale: f32`, `zero_point: i32` |
| `Int8Quantized` | 对称 Int8 量化结果 | `data: []i8`, `params: ScalarQuantParams`, `deinit(*_, alloc)` |
| `AffineQuantized` | 仿射 UInt8 量化结果 | `data: []u8`, `params: ScalarQuantParams`, `deinit(*_, alloc)` |
| `BinaryVector` | 1-bit 二值位图向量 | `bits: []u64`, `dim: usize`, `byteSize(): usize`, `deinit(*_, alloc)` |
| `PqCodebook` | PQ 子空间码本 | `dim: usize`, `m: usize`, `k: usize`, `sub_dim: usize`, `centroids: []f32`, `deinit(*_, alloc)` |

---

## 3. API 速查

### 标量量化
- `quantizeInt8Symmetric(alloc, vec: []const f32): QuantError!Int8Quantized`
- `quantizeInt8Affine(alloc, vec: []const f32): QuantError!AffineQuantized`
- `dequantizeInt8(alloc, data: []const i8, params: ScalarQuantParams): QuantError![]f32`
- `dequantizeAffine(alloc, data: []const u8, params: ScalarQuantParams): QuantError![]f32`
- `dotInt8(a: []const i8, scale_a: f32, b: []const i8, scale_b: f32): QuantError!f32`

### 二值量化
- `quantizeBinary(alloc, vec: []const f32): QuantError!BinaryVector`
- `hammingDistance(a: BinaryVector, b: BinaryVector): QuantError!u32`
- `binaryCosineApprox(a: BinaryVector, b: BinaryVector): QuantError!f32`

### 乘积量化 (PQ & ADC)
- `trainPqKMeans(alloc, vectors: []const []const f32, m: usize, k: usize, max_iters: usize): QuantError!PqCodebook`
- `encodePq(alloc, codebook: PqCodebook, vec: []const f32): QuantError![]u8`
- `buildAdcDistanceTable(alloc, codebook: PqCodebook, query: []const f32): QuantError![]f32`
- `adcDistance(table: []const f32, m: usize, k: usize, code: []const u8): QuantError!f32`

---

## 4. 使用示例

```cp
const std = @import("std");
import {
    quantizeInt8Symmetric,
    dotInt8,
    quantizeBinary,
    hammingDistance,
} from "cap:quant";

pub fn example(alloc: std.mem.Allocator): !void {
    const a: [4]f32 = [ 0.9, -0.4, 0.2, -0.8 ];
    const b: [4]f32 = [ 0.85, -0.35, 0.15, -0.75 ];

    let qa = try quantizeInt8Symmetric(alloc, &a);
    defer qa.deinit(alloc);
    let qb = try quantizeInt8Symmetric(alloc, &b);
    defer qb.deinit(alloc);

    const approx_dot = try dotInt8(qa.data, qa.params.scale, qb.data, qb.params.scale);
    _ = approx_dot;

    let ba = try quantizeBinary(alloc, &a);
    defer ba.deinit(alloc);
    let bb = try quantizeBinary(alloc, &b);
    defer bb.deinit(alloc);

    const hdist = try hammingDistance(ba, bb);
    _ = hdist;
}
```
