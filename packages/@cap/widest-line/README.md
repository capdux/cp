# @cap/widest-line

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:widest-line` ([https://github.com/sindresorhus/widest-line](https://github.com/sindresorhus/widest-line))

Zero-allocation widest visual terminal line width calculator for CP — clean-room MIT parity with npm `widest-line`. Composes `@cap/string-width` with zero duplication.

纯 CP 零堆分配多行文本最大可见列宽计算库（对标 npm `widest-line`），复用 `@cap/string-width` 处理 ANSI 转义序列、CJK 全角宽字符与 Emoji 组合序列，并额外提供逐行宽度采集与制表符对齐宽度计算。

## Install / 安装

```bash
cap add @cap/widest-line
```

## Dependencies / 跨包复用

- `@cap/string-width`: 零堆分配单行终端可见列宽计算（底层组合 `@cap/ansi-strip` 与 `@cap/unicode-width`）

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `widestLine` | `(s: string): usize` | Visual column width of the widest line across `\n`/`\r\n` |
| `widestLineWithOptions` | `(s: string, opts: StringWidthOptions): usize` | Widest line width with custom `StringWidthOptions` |
| `lineWidths` | `(s: string, out: []usize): usize` | Writes each line's visual width into `out`, returns total line count (`0` for `""`) |
| `lineWidthsWithOptions` | `(s: string, opts: StringWidthOptions, out: []usize): usize` | Per-line visual widths with custom `StringWidthOptions` |
| `widestLineWithTab` | `(s: string, tab_width: usize): usize` | Widest line width expanding `\t` to `tab_width`-aligned tab stops |
| `countLines` | `(s: string): usize` | Counts lines in `s` (`0` for empty string) |

## Usage / 使用示例

```cp
import { widestLine, lineWidths, widestLineWithTab } from "@cap/widest-line";

const w1 = widestLine("hi\n你好世界\r\n\x1b[32mgreen\x1b[0m"); // 8
const w2 = widestLineWithTab("a\tb\n1234\tx", 4);            // 9

let widths: [4]usize = undefined;
const n = lineWidths("ab\n中文\n🚀", widths[0..4]); // n = 3, widths = [2, 4, 2]
```
