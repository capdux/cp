[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# string-width

Zero-allocation visual terminal column width calculator (npm string-width v7 parity) composing ansi-strip and unicode-width for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-width` ([https://github.com/sindresorhus/string-width](https://github.com/sindresorhus/string-width))

| 元信息 | 值 |
| --- | --- |
| 包名 | `string-width` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add string-width
```

```cp
import {
    StringWidthOptions,
    SwDecoded,
    swDecodeUtf8,
    swIsRegionalIndicator,
    swIsSkinToneModifier,
    swIsTagChar,
    swBaseCharWidth,
    swMeasurePlain,
} from "string-width";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StringWidthOptions` | `class` | 字段: `ambiguous_is_narrow: bool, count_ansi_escape_codes: bool` · 方法: `defaults` |
| `SwDecoded` | `class` | 字段: `cp: u32, next_pos: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `swDecodeUtf8` | `(s: string, pos: usize): SwDecoded` | 调用 `swDecodeUtf8` 并返回 `SwDecoded`。 |
| `swIsRegionalIndicator` | `(cp: u32): bool` | 调用 `swIsRegionalIndicator` 并返回 `bool`。 |
| `swIsSkinToneModifier` | `(cp: u32): bool` | 调用 `swIsSkinToneModifier` 并返回 `bool`。 |
| `swIsTagChar` | `(cp: u32): bool` | 调用 `swIsTagChar` 并返回 `bool`。 |
| `swBaseCharWidth` | `(cp: u32, ambiguous_is_narrow: bool): usize` | 调用 `swBaseCharWidth` 并返回 `usize`。 |
| `swMeasurePlain` | `(s: string, ambiguous_is_narrow: bool): usize` | 调用 `swMeasurePlain` 并返回 `usize`。 |
| `swMeasureStrippedChunk` | `(chunk: string, ambiguous_is_narrow: bool): usize` | 调用 `swMeasureStrippedChunk` 并返回 `usize`。 |
| `stringWidthWithOptions` | `(input: string, opts: StringWidthOptions): usize` | 调用 `stringWidthWithOptions` 并返回 `usize`。 |
| `stringWidth` | `(input: string): usize` | 调用 `stringWidth` 并返回 `usize`。 |
| `maxLineWidthWithOptions` | `(multiline: string, opts: StringWidthOptions): usize` | 调用 `maxLineWidthWithOptions` 并返回 `usize`。 |
| `maxLineWidth` | `(multiline: string): usize` | 调用 `maxLineWidth` 并返回 `usize`。 |
| `swCopyBytes` | `(dst: []u8, pos: usize, src: string): usize` | 调用 `swCopyBytes` 并返回 `usize`。 |
| `swFillBytes` | `(dst: []u8, pos: usize, ch: u8, count: usize): usize` | 调用 `swFillBytes` 并返回 `usize`。 |
| `padEndVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | 调用 `padEndVisible` 并返回 `usize`。 |
| `padStartVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | 调用 `padStartVisible` 并返回 `usize`。 |

## 最小调用示例

```cp
import { swDecodeUtf8 } from "string-width";

pub fn main() {
    let result = swDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
