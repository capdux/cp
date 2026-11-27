[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ryu

Pure CP zero-allocation fast float-to-string formatter (Rust ryu v1 parity, reuses itoa)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ryu` ([https://github.com/dtolnay/ryu](https://github.com/dtolnay/ryu))（原库名 `ryu` → CP 包名 `ryu`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ryu` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ryu
```

```cp
import {
    RyuDecimal,
    ryuPow10U64,
    ryuScalePow10,
    ryuStripTrailingZeros,
    ryuRoundSig,
    ryuExtractShortestF64,
    ryuExtractShortestF32,
    ryuCopySlice,
} from "ryu";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RyuDecimal` | `class` | 字段: `sig: u64, exp10_msb: i32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ryuPow10U64` | `(n: u8): u64` | 调用 `ryuPow10U64` 并返回 `u64`。 |
| `ryuScalePow10` | `(x: f64, p_in: i32): f64` | 调用 `ryuScalePow10` 并返回 `f64`。 |
| `ryuStripTrailingZeros` | `(sig_in: u64): u64` | 调用 `ryuStripTrailingZeros` 并返回 `u64`。 |
| `ryuRoundSig` | `(m17: u64, drop_digits: u8): u64` | 调用 `ryuRoundSig` 并返回 `u64`。 |
| `ryuExtractShortestF64` | `(abs_val: f64): RyuDecimal` | 调用 `ryuExtractShortestF64` 并返回 `RyuDecimal`。 |
| `ryuExtractShortestF32` | `(abs_f32: f32): RyuDecimal` | 调用 `ryuExtractShortestF32` 并返回 `RyuDecimal`。 |
| `ryuCopySlice` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `ryuCopySlice` 并返回 `bool`。 |
| `ryuPushByte` | `(dst: []u8, pos: &mut usize, b: u8): bool` | 调用 `ryuPushByte` 并返回 `bool`。 |
| `ryuRenderDecimal` | `(neg: bool, dec_val: RyuDecimal, out: []u8): usize` | 调用 `ryuRenderDecimal` 并返回 `usize`。 |
| `ryuRoundAsciiDigits` | `(digits: &[]u8, keep: usize, rounded: []u8): bool` | 调用 `ryuRoundAsciiDigits` 并返回 `bool`。 |
| `isNanF64` | `(val: f64): bool` | 执行 `isNanF64` 条件判定，返回 `bool`。 |
| `isInfF64` | `(val: f64): bool` | 执行 `isInfF64` 条件判定，返回 `bool`。 |
| `isFiniteF64` | `(val: f64): bool` | 执行 `isFiniteF64` 条件判定，返回 `bool`。 |
| `formatF64` | `(val: f64, out: []u8): usize` | 通过 `formatF64` 编码或格式化输出，返回 `usize`。 |
| `formatF64Finite` | `(val: f64, out: []u8): usize` | 通过 `formatF64Finite` 编码或格式化输出，返回 `usize`。 |
| `formatF32` | `(val: f32, out: []u8): usize` | 通过 `formatF32` 编码或格式化输出，返回 `usize`。 |
| `formatFixedF64` | `(val: f64, decimals: u8, out: []u8): usize` | 通过 `formatFixedF64` 编码或格式化输出，返回 `usize`。 |
| `formatExpF64` | `(val: f64, precision: u8, out: []u8): usize` | 通过 `formatExpF64` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isNanF64 } from "ryu";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
