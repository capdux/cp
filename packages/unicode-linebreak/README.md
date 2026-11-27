[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/unicode-linebreak

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-linebreak` ([https://github.com/axelf4/unicode-linebreak](https://github.com/axelf4/unicode-linebreak))

Zero-allocation pure CP UAX #14 Unicode line break opportunity detector — clean-room parity with `unicode-linebreak` (138.7M+ downloads on crates.io, Apache-2.0).

纯 CP 零堆分配 UAX #14 Unicode 断行点检测器，支持 `\n`/`\r\n` 强制断行、空格/连字符后断行、CJK 汉字间允许断行、禁止在 U+00A0 不间断空格与左括号 `(`/`[` 后及右标点 `)`/`,`/`.` 前断行，末尾必出 `Mandatory` 断行。

## Install / 安装

```bash
cap add @cap/unicode-linebreak
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BreakKind` | `enum { Mandatory, Allowed }` | Mandatory vs allowed line break |
| `LineBreak` | `struct { offset: usize; kind: BreakKind; }` | Byte offset and kind of a break opportunity |
| `findLineBreaks` | `(utf8: string, out: []LineBreak): usize` | Write UAX #14 break points into `out` |
| `countLineBreaks` | `(utf8: string): usize` | Count total UAX #14 break points in `utf8` |

## Usage / 使用示例

```cp
import { LineBreak, BreakKind, findLineBreaks, countLineBreaks } from "@cap/unicode-linebreak";

let breaks: [16]LineBreak = undefined;
const n = findLineBreaks("hello world", breaks[0..16]);
// breaks[0] = { offset: 6, kind: Allowed }, breaks[1] = { offset: 11, kind: Mandatory }
```
