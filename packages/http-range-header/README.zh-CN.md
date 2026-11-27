[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http-range-header

Zero-heap RFC 9110 HTTP Range header parser, range validator, and Content-Range formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-range-header` ([https://github.com/MarcusGrass/http-range-header](https://github.com/MarcusGrass/http-range-header))

| 元信息 | 值 |
| --- | --- |
| 包名 | `http-range-header` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http-range-header
```

```cp
import {
    RangePosition,
    SyntacticByteRange,
    ValidatedByteRange,
    hrhIsSpace,
    hrhParseU64,
    parseRangeHeader,
    validateRanges,
    formatContentRange,
} from "http-range-header";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `RangePosition` | `StartAndEnd, StartOnly, Suffix` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SyntacticByteRange` | `class` | 字段: `kind: RangePosition, start: u64, end: u64` |
| `ValidatedByteRange` | `class` | 字段: `start: u64, end: u64` · 方法: `byteLen` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hrhIsSpace` | `(b: u8): bool` | 调用 `hrhIsSpace` 并返回 `bool`。 |
| `hrhParseU64` | `(s: string, start: usize, end: usize): u64?` | 调用 `hrhParseU64` 并返回 `u64?`。 |
| `parseRangeHeader` | `(header: string, out: []SyntacticByteRange): usize` | 通过 `parseRangeHeader` 解析输入数据，返回 `usize`。 |
| `validateRanges` | `(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64` | 校验输入合法性，返回 `i64`。 |
| `formatContentRange` | `(start: u64, end: u64, total: u64, out: []u8): usize` | 通过 `formatContentRange` 编码或格式化输出，返回 `usize`。 |
| `formatUnsatisfiableContentRange` | `(total: u64, out: []u8): usize` | 通过 `formatUnsatisfiableContentRange` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseRangeHeader } from "http-range-header";

pub fn main() {
    let result = parseRangeHeader("example", /* out: []SyntacticByteRange */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
