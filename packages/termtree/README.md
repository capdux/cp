[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/termtree

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:termtree` ([https://github.com/rust-cli/termtree](https://github.com/rust-cli/termtree))

Zero-allocation CLI tree structure renderer with Unicode & ASCII glyph palettes for CP — clean-room MIT parity with Rust `termtree` (233M+ downloads on crates.io).

纯 CP 零堆分配命令行树状结构渲染器（定容 64 节点静态池 + 非递归先序 DFS 渲染），内置 Unicode 框线与 ASCII 兼容字符集，适用于依赖树、目录树、AST 调试视图与 CLI 诊断输出。

## Install / 安装

```bash
cap add @cap/termtree
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `GlyphPalette` | `unicode()`, `ascii()` | Box-drawing glyph sets (`├── `, `└── `, `│   `, `    `) |
| `TermTreeArena` | `init()`, `newRoot(*_, label) -> usize`, `addChild(*_, parent, label) -> i32` | 64-node fixed-capacity tree arena |
| `render` | `(root: usize, out: []u8) -> usize` | Renders tree using default Unicode glyphs |
| `renderWithGlyphs` | `(root: usize, g: GlyphPalette, out: []u8) -> usize` | Renders tree using a custom `GlyphPalette` |
| `nodeCount` | `() -> usize` | Total allocated nodes in arena |
| `maxDepth` | `(root: usize) -> usize` | Maximum tree depth starting at `root` (1-based) |

## Usage / 使用示例

```cp
import { TermTreeArena, GlyphPalette } from "@cap/termtree";

let tree = TermTreeArena.init();
const root = tree.newRoot("root");
const a = tree.addChild(root, "alpha");
* = tree.addChild(usize(a), "a1");
* = tree.addChild(root, "beta");

let buf: [512]u8 = undefined;
const n = tree.render(root, buf[0..512]);
const m = tree.renderWithGlyphs(root, GlyphPalette.ascii(), buf[0..512]);
```
