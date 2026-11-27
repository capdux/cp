[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/braces

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:braces` ([https://github.com/micromatch/braces](https://github.com/micromatch/braces))

Zero-allocation Bash/Glob brace expansion and sequence range generator for CP — clean-room MIT parity with npm `braces` + `fill-range` (~500M/wk).

纯 CP 零堆分配 Bash/Glob 花括号展开与序列范围生成器，使用固定输出缓冲区与零拷贝偏移索引，内置展开上限防爆保护。

## Install / 安装

```bash
cap add @cap/braces
```

## Features / 核心特性

- **Comma Set Expansion**: `a/{b,c,d}/e` → `a/b/e`, `a/c/e`, `a/d/e`, including empty alternatives (`a{,b}` → `a`, `ab`).
- **Numeric & Alpha Sequences (`fill-range`)**: `{1..5}`, descending `{10..2..2}`, zero-padded `{01..03}`, negative `{-2..1}`, letters `{a..e}`, `{Z..W}`, `{a..g..2}`.
- **Cartesian Product & Nesting**: `{x,y}{1..2}` → `x1, x2, y1, y2`; `src/{a,b/{c,d}}.cp` → `src/a.cp, src/b/c.cp, src/b/d.cp`.
- **Explosion Guard**: Caps output at 64 items (`truncated: true`) and passes through unclosed/non-expandable braces verbatim.

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BraceResult` | `struct { buf: []u8, starts: [64]usize, lens: [64]usize, count: usize, truncated: bool }` | Zero-allocation expansion result with `itemAt(idx) -> string` |
| `expandBraces` | `(pattern: string, out: []u8) -> BraceResult` | Expand Bash/Glob brace expressions into `out` |
| `fillRange` | `(start: string, end: string, step: i32, out: []u8) -> BraceResult` | Generate numeric or alphabetic sequence with optional step |

## Usage / 使用示例

```cp
import { expandBraces, fillRange } from "@cap/braces";

let buf: [1024]u8 = undefined;
const res = expandBraces("src/{a,b/{c,d}}.cp", buf[0..1024]);
// res.count == 3: "src/a.cp", "src/b/c.cp", "src/b/d.cp"

let rbuf: [256]u8 = undefined;
const seq = fillRange("01", "03", 1, rbuf[0..256]);
// seq.count == 3: "01", "02", "03"
```
