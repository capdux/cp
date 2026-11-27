[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/redent

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:redent` ([https://github.com/sindresorhus/redent](https://github.com/sindresorhus/redent))

Zero-allocation strip-and-reindent multiline string utility for CP — clean-room MIT parity with npm `redent`. Composes `@cap/strip-indent` and `@cap/indent-string` with zero duplication.

纯 CP 零堆分配多行文本重缩进库（对标 npm `redent`），组合复用 `@cap/strip-indent`（剥离公共最小缩进）与 `@cap/indent-string`（重新按指定缩进单元与次数缩进）。

## Install / 安装

```bash
cap add @cap/redent
```

## Dependencies / 跨包复用

- `@cap/strip-indent`: 剥离公共最小前导空白
- `@cap/indent-string`: 对各行重新施加自定义缩进单元与空行控制

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `redent` | `(s: string, count: usize, out: []u8): usize` | Strips common indentation and re-indents non-empty lines by `count` spaces |
| `redentWithIndent` | `(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize` | Strips common indentation and re-indents with custom `indent` unit and blank-line policy |

## Usage / 使用示例

```cp
import { redent, redentWithIndent } from "@cap/redent";

let buf: [128]u8 = undefined;
const n1 = redent("\n    foo\n      bar\n", 2, buf[0..128]); // "\n  foo\n    bar\n"
const n2 = redentWithIndent("    a\n\n    b", 1, "\t", true, buf[0..128]); // "\ta\n\t\n\tb"
```
