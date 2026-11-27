[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/openblas

OpenBLAS (v0.3.29) high-performance BLAS matrix computation (SGEMM/DGEMM/SDOT/DDOT) for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`OpenBLAS`](https://github.com/OpenMathLib/OpenBLAS)（`BSD-3-Clause`）（包名 `@cap/openblas`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/openblas` |
| 版本 | `0.3.29` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/openblas
```

```cp
import {
    BlasError,
    cblas_sgemm,
    cblas_dgemm,
    cblas_sdot,
    cblas_ddot,
    openblas_get_config,
    openblasGetConfig,
    sgemm,
} from "@cap/openblas";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `BlasError` | `InvalidDimensions, BufferTooSmall` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cblas_sgemm` | `(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f32, a: [*]&f32!, lda: i32, b: [*]&f32!, ldb: i32, beta: f32, c: [*]f32!, ldc: i32)` | 就地执行 `cblas_sgemm` 操作。 |
| `cblas_dgemm` | `(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f64, a: [*]&f64!, lda: i32, b: [*]&f64!, ldb: i32, beta: f64, c: [*]f64!, ldc: i32)` | 就地执行 `cblas_dgemm` 操作。 |
| `cblas_sdot` | `(n: i32, x: [*]&f32!, inc_x: i32, y: [*]&f32!, inc_y: i32): f32` | 调用 `cblas_sdot` 并返回 `f32`。 |
| `cblas_ddot` | `(n: i32, x: [*]&f64!, inc_x: i32, y: [*]&f64!, inc_y: i32): f64` | 调用 `cblas_ddot` 并返回 `f64`。 |
| `openblas_get_config` | `(): [*:0]&u8!` | 调用 `openblas_get_config` 并返回 `[*:0]&u8!`。 |
| `openblasGetConfig` | `(): string` | 调用 `openblasGetConfig` 并返回 `string`。 |
| `sgemm` | `(m: usize, n: usize, k: usize, alpha: f32, a: &[]f32, b: &[]f32, beta: f32, c: []f32): !void` | 调用 `sgemm` 并返回 `!void`。 |
| `dgemm` | `(m: usize, n: usize, k: usize, alpha: f64, a: &[]f64, b: &[]f64, beta: f64, c: []f64): !void` | 调用 `dgemm` 并返回 `!void`。 |
| `sdot` | `(x: &[]f32, y: &[]f32): !f32` | 调用 `sdot` 并返回 `!f32`。 |
| `ddot` | `(x: &[]f64, y: &[]f64): !f64` | 调用 `ddot` 并返回 `!f64`。 |

## 最小调用示例

```cp
import { openblas_get_config } from "@cap/openblas";

pub fn main() {
    let result = openblas_get_config();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
