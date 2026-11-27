[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# textwrap

Zero-allocation text wrapping, ANSI-aware display width, CJK wide-char breaking, indent/dedent, and truncation engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))（原库名 `wrap-ansi` → CP 包名 `textwrap`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `textwrap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add textwrap
```

```cp
import {
    WrapOptions,
    DecodedChar,
    WrapCursor,
    appendByte,
    decodeUtf8,
    ansiSeqLen,
    isAnsiReset,
    codepointWidth,
} from "textwrap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WrapOptions` | `class` | 字段: `width: usize, break_words: bool, preserve_newlines: bool` · 方法: `defaults, withWidth` |
| `DecodedChar` | `class` | 字段: `cp: u32, len: usize` |
| `WrapCursor` | `class` | 字段: `pos: usize, col: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `decodeUtf8` | `(s: string, pos: usize): DecodedChar` | 通过 `decodeUtf8` 解析输入数据，返回 `DecodedChar`。 |
| `ansiSeqLen` | `(s: string, pos: usize): usize` | 调用 `ansiSeqLen` 并返回 `usize`。 |
| `isAnsiReset` | `(s: string, pos: usize, seq_len: usize): bool` | 执行 `isAnsiReset` 条件判定，返回 `bool`。 |
| `codepointWidth` | `(cp: u32): usize` | 调用 `codepointWidth` 并返回 `usize`。 |
| `isSpaceByte` | `(c: u8): bool` | 执行 `isSpaceByte` 条件判定，返回 `bool`。 |
| `displayWidth` | `(s: string): usize` | 调用 `displayWidth` 并返回 `usize`。 |
| `stripAnsi` | `(s: string, out: []u8): string` | 调用 `stripAnsi` 并返回 `string`。 |
| `writeHardBrokenToken` | `(text: string, tok_start: usize, tok_end: usize, width: usize, out: []u8, start_pos: usize, start_col: usize): WrapCursor` | 调用 `writeHardBrokenToken` 并返回 `WrapCursor`。 |
| `wrapSegment` | `(text: string, seg_start: usize, seg_end: usize, opts: WrapOptions, out: []u8, start_pos: usize): usize` | 调用 `wrapSegment` 并返回 `usize`。 |
| `wrapTextWith` | `(text: string, opts: WrapOptions, out: []u8): string` | 调用 `wrapTextWith` 并返回 `string`。 |
| `wrapText` | `(text: string, width: usize, out: []u8): string` | 调用 `wrapText` 并返回 `string`。 |
| `lineHasContent` | `(text: string, start: usize, end: usize): bool` | 调用 `lineHasContent` 并返回 `bool`。 |
| `indent` | `(text: string, prefix: string, out: []u8): string` | 调用 `indent` 并返回 `string`。 |
| `dedent` | `(text: string, out: []u8): string` | 调用 `dedent` 并返回 `string`。 |
| `truncate` | `(text: string, max_width: usize, ellipsis: string, out: []u8): string` | 调用 `truncate` 并返回 `string`。 |

## 最小调用示例

```cp
import { decodeUtf8 } from "textwrap";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
