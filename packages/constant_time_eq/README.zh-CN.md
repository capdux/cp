[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# constant_time_eq

Zero-allocation constant-time cryptographic comparison, branchless selection/swap, and secure memory zeroization primitives (Rust subtle + zeroize parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:constant_time_eq` ([https://github.com/cesarb/constant_time_eq](https://github.com/cesarb/constant_time_eq))（原库名 `constant_time_eq` → CP 包名 `constant_time_eq`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `constant_time_eq` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add constant_time_eq
```

```cp
import {
    Choice,
    maskU8,
    maskU32,
    maskU64,
    ctIsZeroU8,
    ctEqU8,
    ctEqU32,
    ctEqU64,
} from "constant_time_eq";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Choice` | `class` | 字段: `val: u8` · 方法: `fromU8, fromBool, toBool, ctNot, ctAnd, ctOr, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `maskU8` | `(c: Choice): u8` | 调用 `maskU8` 并返回 `u8`。 |
| `maskU32` | `(c: Choice): u32` | 调用 `maskU32` 并返回 `u32`。 |
| `maskU64` | `(c: Choice): u64` | 调用 `maskU64` 并返回 `u64`。 |
| `ctIsZeroU8` | `(x: u8): Choice` | 调用 `ctIsZeroU8` 并返回 `Choice`。 |
| `ctEqU8` | `(a: u8, b: u8): Choice` | 调用 `ctEqU8` 并返回 `Choice`。 |
| `ctEqU32` | `(a: u32, b: u32): Choice` | 调用 `ctEqU32` 并返回 `Choice`。 |
| `ctEqU64` | `(a: u64, b: u64): Choice` | 调用 `ctEqU64` 并返回 `Choice`。 |
| `ctLtU64` | `(a: u64, b: u64): Choice` | 调用 `ctLtU64` 并返回 `Choice`。 |
| `ctGtU64` | `(a: u64, b: u64): Choice` | 调用 `ctGtU64` 并返回 `Choice`。 |
| `ctSelectU8` | `(c: Choice, when_true: u8, when_false: u8): u8` | 调用 `ctSelectU8` 并返回 `u8`。 |
| `ctSelectU32` | `(c: Choice, when_true: u32, when_false: u32): u32` | 调用 `ctSelectU32` 并返回 `u32`。 |
| `ctSelectU64` | `(c: Choice, when_true: u64, when_false: u64): u64` | 调用 `ctSelectU64` 并返回 `u64`。 |
| `ctSwapU64` | `(c: Choice, a: &mut u64, b: &mut u64)` | 就地执行 `ctSwapU64` 操作。 |
| `ctEqBytes` | `(a: string, b: string): Choice` | 调用 `ctEqBytes` 并返回 `Choice`。 |
| `ctEqFixed32` | `(a: [32]u8, b: [32]u8): Choice` | 调用 `ctEqFixed32` 并返回 `Choice`。 |
| `ctCopyIf` | `(c: Choice, dst: []u8, src: string)` | 就地执行 `ctCopyIf` 操作。 |
| `zeroizeBytes` | `(buf: []u8)` | 就地执行 `zeroizeBytes` 操作。 |
| `isAllZero` | `(buf: string): Choice` | 执行 `isAllZero` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isAllZero } from "constant_time_eq";

pub fn main() {
    let result = isAllZero("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
