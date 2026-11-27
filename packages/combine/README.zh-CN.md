[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# combine

Zero-copy streaming parser combinators with line/column cursor tracking and backtracking (Rust combine v4 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:combine` ([https://github.com/Marwes/combine](https://github.com/Marwes/combine))

| 元信息 | 值 |
| --- | --- |
| 包名 | `combine` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add combine
```

```cp
import {
    CharPred,
    CursorMark,
    ParseCursor,
    cmbIsAlpha,
    cmbIsDigit,
    cmbIsSpace,
    cmbIsHex,
    cmbMatchPred,
} from "combine";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CharPred` | `Alpha, Digit, AlphaNum, Space, HexDigit, Ident, NonSpace` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CursorMark` | `class` | 字段: `pos: usize, line: usize, col: usize` |
| `ParseCursor` | `class` | 字段: `input: string, pos: usize, line: usize, col: usize` · 方法: `new, remaining, isEof, peek, advance, save, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmbIsAlpha` | `(ch: u8): bool` | 调用 `cmbIsAlpha` 并返回 `bool`。 |
| `cmbIsDigit` | `(ch: u8): bool` | 调用 `cmbIsDigit` 并返回 `bool`。 |
| `cmbIsSpace` | `(ch: u8): bool` | 调用 `cmbIsSpace` 并返回 `bool`。 |
| `cmbIsHex` | `(ch: u8): bool` | 调用 `cmbIsHex` 并返回 `bool`。 |
| `cmbMatchPred` | `(ch: u8, kind: CharPred): bool` | 调用 `cmbMatchPred` 并返回 `bool`。 |
| `cmbStartsWithAt` | `(hay: string, pos: usize, needle: string): bool` | 调用 `cmbStartsWithAt` 并返回 `bool`。 |
| `tag` | `(cursor: &mut ParseCursor, lit: string): bool` | 调用 `tag` 并返回 `bool`。 |
| `charByte` | `(cursor: &mut ParseCursor, expected: u8): bool` | 调用 `charByte` 并返回 `bool`。 |
| `takeWhile` | `(cursor: &mut ParseCursor, pred_kind: CharPred): string` | 调用 `takeWhile` 并返回 `string`。 |
| `takeWhile1` | `(cursor: &mut ParseCursor, pred_kind: CharPred): string?` | 调用 `takeWhile1` 并返回 `string?`。 |
| `takeUntil` | `(cursor: &mut ParseCursor, delim: string): string?` | 调用 `takeUntil` 并返回 `string?`。 |
| `skipSpaces` | `(cursor: &mut ParseCursor): usize` | 调用 `skipSpaces` 并返回 `usize`。 |
| `parseU64` | `(cursor: &mut ParseCursor): u64?` | 通过 `parseU64` 解析输入数据，返回 `u64?`。 |
| `parseI64` | `(cursor: &mut ParseCursor): i64?` | 通过 `parseI64` 解析输入数据，返回 `i64?`。 |
| `parseQuotedString` | `(cursor: &mut ParseCursor): string?` | 通过 `parseQuotedString` 解析输入数据，返回 `string?`。 |
| `delimited` | `(cursor: &mut ParseCursor, open: string, close: string): string?` | 调用 `delimited` 并返回 `string?`。 |
| `separatedList` | `(cursor: &mut ParseCursor, sep: string, out: []string): usize` | 调用 `separatedList` 并返回 `usize`。 |

## 最小调用示例

```cp
import { cmbStartsWithAt } from "combine";

pub fn main() {
    let result = cmbStartsWithAt("example", 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
