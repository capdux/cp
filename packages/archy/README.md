[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# archy

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:archy` ([https://github.com/substack/node-archy](https://github.com/substack/node-archy))（原库名 `archy` → CP 包名 `archy`）

Zero-allocation Unicode & ASCII dependency/directory tree renderer for CP — clean-room MIT parity with npm `archy` (35M/wk), `treeify`, and Rust `ptree`.

## Install

```bash
cap add archy
```

## Features

- **3 Styles**: `TreeStyle.Unicode` (`├──`/`└──`/`│`), `TreeStyle.Rounded` (`├──`/`╰──`/`│`), `TreeStyle.Ascii` (`|--`/`\`--`/`|`).
- **Zero Heap Allocation**: stack-based `TreeBuilder` supporting up to 128 nodes & 32 nesting levels.
- **Ideal for CLI Tools**: `cap tree`, dependency graphs, file explorers, and AI agent task trees.

## Usage

```cp
import { TreeBuilder, TreeStyle, renderTree } from "archy";

let t = TreeBuilder.init("my-agent@1.0.0");
const deps = t.addChild(0, "lru-cache@1.0.0") ?? 0;
* = t.addChild(deps, "validator@1.0.0");
* = t.addChild(0, "fast-redact@1.0.0");

let out: [1024]u8 = undefined;
const text = renderTree(&t, out[0..1024]);
```
