[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# h3o

Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index cell parser, constructor, hierarchy traversal, and area table (clean-room port of Rust h3o)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h3o` ([https://github.com/HydroniumLabs/h3o](https://github.com/HydroniumLabs/h3o))

| 元信息 | 值 |
| --- | --- |
| 包名 | `h3o` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add h3o
```

```cp
import {
    H3Cell,
    h3AllLow63,
    h3DigitShift,
    h3IsPentagonBaseCell,
    h3HexDigitVal,
    h3HexChar,
    h3ClearBits,
    parseH3Hex,
} from "h3o";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `H3Cell` | `class` | 字段: `bits: u64` · 方法: `fromParts, fromU64, resolution, baseCell, digitAt, isPentagon, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `h3AllLow63` | `(): u64` | 调用 `h3AllLow63` 并返回 `u64`。 |
| `h3DigitShift` | `(r: u8): u6` | 调用 `h3DigitShift` 并返回 `u6`。 |
| `h3IsPentagonBaseCell` | `(bc: u8): bool` | 调用 `h3IsPentagonBaseCell` 并返回 `bool`。 |
| `h3HexDigitVal` | `(c: u8): u8` | 调用 `h3HexDigitVal` 并返回 `u8`。 |
| `h3HexChar` | `(nibble: u8): u8` | 调用 `h3HexChar` 并返回 `u8`。 |
| `h3ClearBits` | `(val: u64, mask: u64): u64` | 调用 `h3ClearBits` 并返回 `u64`。 |
| `parseH3Hex` | `(hex: string): H3Cell?` | 通过 `parseH3Hex` 解析输入数据，返回 `H3Cell?`。 |
| `avgHexAreaKm2` | `(res: u8): f64` | 调用 `avgHexAreaKm2` 并返回 `f64`。 |

## 最小调用示例

```cp
import { parseH3Hex } from "h3o";

pub fn main() {
    let result = parseH3Hex("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
