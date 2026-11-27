[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# subtle

Zero-allocation branchless constant-time cryptographic primitives (Choice, ctEq, ctLt, ctSelect, ctCondSwap, zeroizeBytes) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:subtle` ([https://github.com/dalek-cryptography/subtle](https://github.com/dalek-cryptography/subtle)) / `npm:tsscmp` ([https://github.com/suryagh/tsscmp](https://github.com/suryagh/tsscmp)) / `npm:safe-compare` ([https://github.com/vadimdemedes/safe-compare](https://github.com/vadimdemedes/safe-compare))（原库名 `subtle` → CP 包名 `subtle`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `subtle` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add subtle
```

```cp
import {
    Choice,
    mask8,
    mask32,
    mask64,
    ctIsZeroU8,
    ctIsZeroU32,
    ctIsZeroU64,
    ctEqU8,
} from "subtle";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Choice` | `class` | 字段: `val: u8` · 方法: `fromU8, toBool, not, and, or, xor` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mask8` | `(c: Choice): u8` | 调用 `mask8` 并返回 `u8`。 |
| `mask32` | `(c: Choice): u32` | 调用 `mask32` 并返回 `u32`。 |
| `mask64` | `(c: Choice): u64` | 调用 `mask64` 并返回 `u64`。 |
| `ctIsZeroU8` | `(x: u8): Choice` | 调用 `ctIsZeroU8` 并返回 `Choice`。 |
| `ctIsZeroU32` | `(x: u32): Choice` | 调用 `ctIsZeroU32` 并返回 `Choice`。 |
| `ctIsZeroU64` | `(x: u64): Choice` | 调用 `ctIsZeroU64` 并返回 `Choice`。 |
| `ctEqU8` | `(a: u8, b: u8): Choice` | 调用 `ctEqU8` 并返回 `Choice`。 |
| `ctEqU32` | `(a: u32, b: u32): Choice` | 调用 `ctEqU32` 并返回 `Choice`。 |
| `ctEqU64` | `(a: u64, b: u64): Choice` | 调用 `ctEqU64` 并返回 `Choice`。 |
| `ctEqBytes` | `(a: &[]u8, b: &[]u8): Choice` | 调用 `ctEqBytes` 并返回 `Choice`。 |
| `ctLtU64` | `(a: u64, b: u64): Choice` | 调用 `ctLtU64` 并返回 `Choice`。 |
| `ctGtU64` | `(a: u64, b: u64): Choice` | 调用 `ctGtU64` 并返回 `Choice`。 |
| `ctSelectU8` | `(a: u8, b: u8, c: Choice): u8` | 调用 `ctSelectU8` 并返回 `u8`。 |
| `ctSelectU32` | `(a: u32, b: u32, c: Choice): u32` | 调用 `ctSelectU32` 并返回 `u32`。 |
| `ctSelectU64` | `(a: u64, b: u64, c: Choice): u64` | 调用 `ctSelectU64` 并返回 `u64`。 |
| `ctSelectBytes` | `(dst: []u8, a: &[]u8, b: &[]u8, c: Choice): usize` | 调用 `ctSelectBytes` 并返回 `usize`。 |
| `ctCondSwapU64` | `(a: &mut u64, b: &mut u64, c: Choice)` | 就地执行 `ctCondSwapU64` 操作。 |
| `zeroizeBytes` | `(buf: []u8)` | 就地执行 `zeroizeBytes` 操作。 |

## 最小调用示例

```cp
import { mask8 } from "subtle";

pub fn main() {
    let result = mask8(/* c: Choice */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
