[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/strip-indent

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-indent` ([https://github.com/sindresorhus/strip-indent](https://github.com/sindresorhus/strip-indent))

Zero-allocation leading whitespace stripper for CP — clean-room MIT parity with npm `strip-indent` and `min-indent`.

纯 CP 零堆分配多行公共缩进剥离库（完整对标 npm `strip-indent` 与 `min-indent`），自动忽略空行与纯空白行计算最小缩进，保留 `\n` / `\r\n` 行尾。

## Install / 安装

```bash
cap add @cap/strip-indent
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `minIndent` | `(s: string): usize` | Minimum leading space/tab count across all non-empty lines (`0` if none) |
| `stripIndent` | `(s: string, out: []u8): usize` | Strips `minIndent(s)` leading spaces/tabs from each line into `out`, returns bytes written |
| `stripIndentBy` | `(s: string, n: usize, out: []u8): usize` | Strips `n` leading spaces/tabs from lines with at least `n` leading spaces/tabs |

## Usage / 使用示例

```cp
import { minIndent, stripIndent } from "@cap/strip-indent";

const src = "\n    foo\n      bar\n";
const m = minIndent(src); // 4
let buf: [64]u8 = undefined;
const n = stripIndent(src, buf[0..64]); // "\nfoo\n  bar\n"
```
