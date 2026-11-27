[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/micromatch

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:micromatch` ([https://github.com/micromatch/micromatch](https://github.com/micromatch/micromatch))

Zero-allocation glob matching suite for CP — clean-room MIT parity with npm `micromatch` v4. Composes `@cap/picomatch` and `@cap/braces` with zero duplication.

纯 CP 零堆分配顶级 Glob 匹配套件（对标 npm `micromatch` v4），在 `@cap/picomatch` 通配符引擎与 `@cap/braces` 数值/字母范围展开之上提供全套集合匹配、批量过滤、反向过滤与子路径包含 API。

## Install / 安装

```bash
cap add @cap/micromatch
```

## Dependencies / 跨包复用

- `@cap/picomatch`: 核心 glob 匹配引擎、选项系统与 `scanBase`
- `@cap/braces`: 数值/字母花括号范围展开（如 `{1..5}`、`{a..z}`）

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `mmIsMatch` | `(path: string, pattern: string, opts: PicoOptions): bool` | Match single path with brace range expansion + full picomatch glob features |
| `mmAll` | `(path: string, patterns: []const string, opts: PicoOptions): bool` | Return `true` iff `path` matches ALL patterns |
| `mmAny` | `(path: string, patterns: []const string, opts: PicoOptions): bool` | Return `true` iff `path` matches positive rules and is not excluded by `!` rules |
| `mmSome` | `(paths: []const string, patterns: []const string, opts: PicoOptions): bool` | Return `true` iff at least one path matches |
| `mmEvery` | `(paths: []const string, patterns: []const string, opts: PicoOptions): bool` | Return `true` iff every path matches |
| `mmFilter` | `(paths, patterns, opts, out_indices: []usize): usize` | Write indices of matching paths into `out_indices` |
| `mmNot` | `(paths, patterns, opts, out_indices: []usize): usize` | Write indices of non-matching paths into `out_indices` |
| `mmContains` | `(path: string, pattern: string, opts: PicoOptions): bool` | Sub-path / segment fragment containment match |

## Usage / 使用示例

```cp
import { PicoOptions, mmIsMatch, mmFilter, mmNot } from "@cap/micromatch";

const opts = PicoOptions.defaults();
const hit = mmIsMatch("file-2.cp", "file-{1..3}.cp", opts); // true

const paths: [3]string = ["src/a.cp", "src/a.test.cp", "README.md"];
const rules: [2]string = ["src/**/*.cp", "!**/*.test.cp"];
let idx: [4]usize = undefined;
const n = mmFilter(paths[0..3], rules[0..2], opts, idx[0..4]); // n == 1, idx[0] == 0
```
