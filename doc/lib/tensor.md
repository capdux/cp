# `cap:tensor` — 工业级多维张量与 AI 算子标准库

## 1. 概述

`cap:tensor`（同时支持 `cap.tensor` 动态挂载）是 CP 语言的纯原生多维张量与 LLM Transformer 核心算子库。支持最多 4 维（`MAX_TENSOR_DIMS = 4`）的 `f32` 张量、基于 Stride 的零拷贝视图变换（转置、轴切片、连续 Reshape）、广播运算、Strided 矩阵乘法以及数值稳定的神经网络/大模型核心算子。

## 2. 核心类型

- **`MAX_TENSOR_DIMS: usize = 4`**：张量最大维度上限。
- **`TensorError`**：错误集合，包含 `DimensionMismatch`、`InvalidShape`、`NonContiguous`、`OutOfBounds`、`EmptyTensor`、`InvalidAxis`。
- **`Tensor`**：多维 Strided 张量结构体，包含 `data: []f32`、`shape: [4]usize`、`strides: [4]usize`、`ndim: usize`、`offset: usize`、`owns_data: bool`。

## 3. API 速查

### 构建与生命周期
- `Tensor.zeros(alloc, shape): Tensor!` / `Tensor.ones(alloc, shape): Tensor!` / `Tensor.full(alloc, shape, val): Tensor!`
- `Tensor.fromSlice(alloc, shape, src): Tensor!`
- `t.clone(alloc): Tensor!` / `t.deinit(alloc)`

### 视图、索引与变换
- `t.numel(): usize` / `t.shapeSlice(): []const usize` / `t.isContiguous(): bool`
- `t.at(indices): f32!` / `t.set(indices, val): void!`
- `t.transpose2d(): Tensor!`（零拷贝交换 2D Strides）
- `t.sliceAxis(axis, start, end): Tensor!`（零拷贝轴切片）
- `t.reshape(alloc, new_shape): Tensor!`（仅限连续张量，否则返回 `NonContiguous`）
- `t.contiguous(alloc): Tensor!`（将任意 Strided 视图物化为紧凑连续张量）

### 广播与线性代数
- `add(alloc, a, b): Tensor!` / `mul(alloc, a, b): Tensor!`（支持标量、向量对矩阵行广播及同形张量）
- `scale(alloc, a, factor): Tensor!`
- `matmul(alloc, a, b): Tensor!`（支持连续与 Strided 2D 矩阵乘 `[M, K] x [K, N] -> [M, N]`）

### Transformer 核心算子
- `softmaxRows(alloc, x): Tensor!`（数值稳定行归一化 `exp(x - max)`）
- `rmsNorm(alloc, x, weight, eps): Tensor!`
- `layerNorm(alloc, x, weight, bias, eps): Tensor!`
- `silu(alloc, x): Tensor!` / `gelu(alloc, x): Tensor!`
- `applyRope(alloc, x, pos, theta_base): Tensor!`

## 4. 示例

```cp
import { Tensor, matmul, rmsNorm, softmaxRows } from "cap:tensor";

const s_a: [2]usize = [2, 3];
const d_a: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
let a = Tensor.fromSlice(alloc, &s_a, &d_a)?;
defer a.deinit(alloc);

let a_t = a.transpose2d()?; // [3, 2] 零拷贝视图
let prod = matmul(alloc, a, a_t)?; // [2, 2]
defer prod.deinit(alloc);

let probs = softmaxRows(alloc, prod)?;
defer probs.deinit(alloc);
```
