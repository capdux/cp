[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bigdecimal

Zero-heap exact base-10 financial arithmetic library with 18-digit precision, rounding, parsing, and formatting (clean-room port of Rust bigdecimal)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bigdecimal` ([https://github.com/akubera/bigdecimal-rs](https://github.com/akubera/bigdecimal-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bigdecimal` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bigdecimal
```

```cp
import {
    BigDecimal,
    bdecIsSpace,
    bdecIsDigit,
    bdecAbsU64,
    bdecSignedFromU64,
    bdecPow10U64,
    bdecDigitCount,
    bdecWriteByte,
} from "bigdecimal";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BigDecimal` | `class` | 字段: `digits: i64, scale: i32` · 方法: `zero, fromInt, new, normalize, neg, abs, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bdecIsSpace` | `(c: u8): bool` | 调用 `bdecIsSpace` 并返回 `bool`。 |
| `bdecIsDigit` | `(c: u8): bool` | 调用 `bdecIsDigit` 并返回 `bool`。 |
| `bdecAbsU64` | `(v: i64): u64` | 调用 `bdecAbsU64` 并返回 `u64`。 |
| `bdecSignedFromU64` | `(mag: u64, neg: bool): i64` | 调用 `bdecSignedFromU64` 并返回 `i64`。 |
| `bdecPow10U64` | `(exp: u32): u64` | 调用 `bdecPow10U64` 并返回 `u64`。 |
| `bdecDigitCount` | `(u: u64): i32` | 调用 `bdecDigitCount` 并返回 `i32`。 |
| `bdecWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `bdecWriteByte` 并返回 `usize`。 |
| `parseDecimal` | `(s: string): BigDecimal?` | 通过 `parseDecimal` 解析输入数据，返回 `BigDecimal?`。 |
| `formatDecimal` | `(val: BigDecimal, out: []u8): usize` | 通过 `formatDecimal` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseDecimal } from "bigdecimal";

pub fn main() {
    let result = parseDecimal("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
