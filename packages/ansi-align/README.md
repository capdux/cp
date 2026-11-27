[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ansi-align

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-align` ([https://github.com/nexdrew/ansi-align](https://github.com/nexdrew/ansi-align))

Zero-allocation ANSI-aware multiline text alignment for CP — clean-room ISC parity with npm `ansi-align`. Composes `@cap/string-width` and `@cap/widest-line` with zero duplication.

纯 CP 零堆分配 ANSI 感知多行文本对齐库（对标 npm `ansi-align`），复用 `@cap/string-width` 与 `@cap/widest-line` 计算视觉列宽，支持左对齐、居中、右对齐，正确处理 ANSI 颜色码、CJK 双宽字符、Emoji 与 `\n`/`\r\n` 换行。

## Install / 安装

```bash
cap add ansi-align
```

## Dependencies / 跨包复用

- `@cap/string-width`: 计算单行视觉列宽（忽略 ANSI 转义序列，支持 CJK/Emoji）
- `@cap/widest-line`: 计算多行文本的最大视觉列宽

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `AlignMode` | `enum { Left, Center, Right }` | Alignment mode |
| `alignAnsi` | `(text: string, mode: AlignMode, out: []u8): usize` | Aligns multiline text by visual width into `out`, returns bytes written |
| `alignCenter` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Center, out)` |
| `alignRight` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Right, out)` |
| `alignLeft` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Left, out)` |

## Usage / 使用示例

```cp
import { AlignMode, alignAnsi, alignCenter, alignRight } from "ansi-align";

let buf: [256]u8 = undefined;
const n1 = alignCenter("hi\nhello!", buf[0..256]); // "  hi\nhello!"
const n2 = alignRight("\x1b[32m中文\x1b[0m\n12345678", buf[0..256]);
```
