# @cap/boxen-ui

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:boxen` ([https://github.com/sindresorhus/boxen](https://github.com/sindresorhus/boxen))（原库名 `boxen` → CP 包名 `@cap/boxen-ui`）

Zero-allocation terminal Unicode box layout renderer for CP — clean-room MIT parity with npm `boxen` v8. Composes `@cap/string-width` for accurate ANSI escape sequence and CJK double-width character alignment.

纯 CP 零堆分配终端 Unicode 边框盒子排版渲染器（对标 npm `boxen` v8），跨包复用 `@cap/string-width` 实现精确的 ANSI 样式码与 CJK 全角字符列宽对齐。

## Install / 安装

```bash
cap add @cap/boxen-ui
```

## Dependencies / 跨包复用

- `@cap/string-width`: 零堆分配计算终端可见列宽（自动忽略 ANSI CSI/SGR/OSC 转义码并正确计算 CJK / Emoji 双宽字符）

## API

### Border Styles / 边框风格常量

| Constant | Value | Glyphs |
| --- | --- | --- |
| `BORDER_SINGLE` | `0` | `┌─┐│└┘` |
| `BORDER_DOUBLE` | `1` | `╔═╗║╚╝` |
| `BORDER_ROUND` | `2` | `╭─╮│╰╯` |
| `BORDER_BOLD` | `3` | `┏━┓┃┗┛` |
| `BORDER_CLASSIC` | `4` | `+-+\|+-+` |

### Text Alignment / 对齐常量

| Constant | Value | Description |
| --- | --- | --- |
| `ALIGN_LEFT` | `0` | Left-align text inside the box |
| `ALIGN_CENTER` | `1` | Center-align text inside the box |
| `ALIGN_RIGHT` | `2` | Right-align text inside the box |

### Struct & Function / 核心配置与渲染函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `BoxenOptions` | `struct { border_style: u8, padding_x: usize, padding_y: usize, text_align: u8, title: string }` + `.defaultOpts()` | Box layout configuration |
| `renderBox` | `(text: string, opts: BoxenOptions, out: []u8): usize` | Render `text` into caller-supplied `out` buffer with zero heap allocations; returns bytes written |

## Usage / 使用示例

```cp
import {
    BORDER_ROUND,
    ALIGN_CENTER,
    BoxenOptions,
    renderBox,
} from "@cap/boxen-ui";

let buf: [512]u8 = undefined;
const opts = BoxenOptions {
    border_style: BORDER_ROUND,
    padding_x: 2,
    padding_y: 1,
    text_align: ALIGN_CENTER,
    title: "CP Lang",
};
const n = renderBox("Hello\n你好世界", opts, buf[0..512]);
```
