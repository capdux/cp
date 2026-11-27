[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/glob-parent

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-parent` ([https://github.com/gulpjs/glob-parent](https://github.com/gulpjs/glob-parent))

Zero-allocation glob static parent directory extractor, glob detector, and escaper for CP — clean-room MIT parity with npm `glob-parent` + `is-glob` (~170M/wk).

纯 CP 零堆分配 Glob 静态前缀父目录提取、通配检测与安全转义引擎，适用于文件监听、构建工具与目录遍历器。

## Install / 安装

```bash
cap add @cap/glob-parent
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `GlobSplit` | `struct { base: string, glob: string, is_glob: bool }` | Split result with static base directory and dynamic glob suffix |
| `isGlob` | `(pattern: string) -> bool` | Detect `*`, `?`, `[...]`, `{...,...}`, `{a..b}`, and extglobs (`!(...)`, `+(...)`, `@(...)`) while respecting escapes |
| `globParent` | `(pattern: string) -> string` | Extract static parent directory before the first glob segment |
| `splitGlobBase` | `(pattern: string) -> GlobSplit` | Split pattern into `{ base, glob, is_glob }` |
| `escapeGlob` | `(raw: string, out: []u8) -> string` | Escape `*`, `?`, `[`, `]`, `{`, `}`, `(`, `)`, `!`, `\` into `out` |

## Usage / 使用示例

```cp
import { isGlob, globParent, splitGlobBase, escapeGlob } from "@cap/glob-parent";

const ok = isGlob("src/**/*.cp");                 // true
const parent = globParent("/a/b/{c,d}/e");        // "/a/b"
const sp = splitGlobBase("src/components/*.cp");  // base: "src/components", glob: "*.cp", is_glob: true

let buf: [128]u8 = undefined;
const escaped = escapeGlob("a[0]*.cp", buf[0..128]); // "a\\[0\\]\\*.cp"
```
