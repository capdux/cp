# @cap/string-width

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-width` ([https://github.com/sindresorhus/string-width](https://github.com/sindresorhus/string-width))

Zero-allocation visual terminal column width calculator for CP — clean-room MIT parity with npm `string-width` v7. Composes `@cap/ansi-strip` and `@cap/unicode-width` with zero duplication.

纯 CP 零堆分配终端可见列宽计算库（对标 npm `string-width` v7），复用 `@cap/ansi-strip`（剥离 ANSI CSI/SGR/OSC 转义序列）与 `@cap/unicode-width`（计算 Unicode 15.1 码点与东亚全角/模糊宽度）。

## Install / 安装

```bash
cap add @cap/string-width
```

## Dependencies / 跨包复用

- `@cap/ansi-strip`: 零堆分配剥离 ANSI SGR/CSI 样式码与 OSC 8 终端超链接
- `@cap/unicode-width`: 零堆分配计算 ASCII、CJK 全角、Emoji、组合音标与东亚模糊宽字符列宽

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `StringWidthOptions` | `struct { ambiguous_is_narrow: bool, count_ansi_escape_codes: bool }` + `.defaults()` | Configuration options for width measurement |
| `stringWidth` | `(input: string): usize` | Visual column width ignoring ANSI codes (handles CJK, Emoji ZWJ, skin-tone modifiers, `U+FE0F`/`U+FE0E`, flag RI pairs, combining marks) |
| `stringWidthWithOptions` | `(input: string, opts: StringWidthOptions): usize` | Visual column width with custom `ambiguous_is_narrow` & `count_ansi_escape_codes` |
| `maxLineWidth` | `(multiline: string): usize` | Maximum visual column width across `\n`/`\r\n`-separated lines |
| `maxLineWidthWithOptions` | `(multiline: string, opts: StringWidthOptions): usize` | Maximum line width with custom options |
| `padEndVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | Right-pad to `target_cols` visual columns preserving ANSI/CJK alignment |
| `padStartVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | Left-pad to `target_cols` visual columns preserving ANSI/CJK alignment |

## Usage / 使用示例

```cp
import { stringWidth, maxLineWidth, padEndVisible } from "@cap/string-width";

const w1 = stringWidth("你好世界"); // 8
const w2 = stringWidth("\x1b[31mhello\x1b[0m"); // 5
const w3 = maxLineWidth("hi\n你好世界\nok"); // 8

let buf: [64]u8 = undefined;
const n = padEndVisible("\x1b[32m中文\x1b[0m", 8, 32, buf[0..64]); // pads 4 trailing spaces
```
