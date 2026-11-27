[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-format

Zero-heap D3/Python-style number format specifier parser and formatter for CP (clean-room ISC implementation of npm d3-format).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-format` ([https://github.com/d3/d3-format](https://github.com/d3/d3-format))（原库名 `d3-format` → CP 包名 `d3-format`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-format` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-format
```

```cp
import {
    FormatSpec,
    d3fIsAlign,
    d3fIsType,
    parseFormatSpec,
    d3fPow10F64,
    d3fPow10U64,
    d3fWriteU64Base,
    d3fWriteFixed,
} from "d3-format";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FormatSpec` | `class` | 字段: `valid: bool, fill: u8, @"align": u8, sign: u8, symbol: u8, zero: bool, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3fIsAlign` | `(ch: u8): bool` | 调用 `d3fIsAlign` 并返回 `bool`。 |
| `d3fIsType` | `(ch: u8): bool` | 调用 `d3fIsType` 并返回 `bool`。 |
| `parseFormatSpec` | `(spec: string): FormatSpec` | 通过 `parseFormatSpec` 解析输入数据，返回 `FormatSpec`。 |
| `d3fPow10F64` | `(exp: i32): f64` | 调用 `d3fPow10F64` 并返回 `f64`。 |
| `d3fPow10U64` | `(exp: i32): u64` | 调用 `d3fPow10U64` 并返回 `u64`。 |
| `d3fWriteU64Base` | `(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize` | 调用 `d3fWriteU64Base` 并返回 `usize`。 |
| `d3fWriteFixed` | `(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize` | 调用 `d3fWriteFixed` 并返回 `usize`。 |
| `d3fAppendSiSuffix` | `(exp3: i32, out: []u8, pos: usize): usize` | 调用 `d3fAppendSiSuffix` 并返回 `usize`。 |
| `d3fDisplayLen` | `(slice: &[]u8): usize` | 调用 `d3fDisplayLen` 并返回 `usize`。 |
| `d3fCopySlice` | `(dst: []u8, pos: usize, src: &[]u8): usize` | 调用 `d3fCopySlice` 并返回 `usize`。 |
| `formatNumber` | `(spec: string, value: f64, out: []u8): usize` | 通过 `formatNumber` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseFormatSpec } from "d3-format";

pub fn main() {
    let result = parseFormatSpec("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
