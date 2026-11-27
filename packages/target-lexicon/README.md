[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# target-lexicon

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:target-lexicon` ([https://github.com/bytecodealliance/target-lexicon](https://github.com/bytecodealliance/target-lexicon))（原库名 `target-lexicon` → CP 包名 `target-lexicon`）

Zero-heap LLVM / Rust / Zig target triple parser and platform metadata resolver in pure CP (inspired by Rust `target-lexicon`, 479M+ downloads).

Supports 2-part (`wasm32-wasi`), 3-part (`aarch64-apple-darwin`, Zig-style `x86_64-linux-musl`), and 4-part (`x86_64-unknown-linux-gnu`, `x86_64-pc-windows-msvc`) triples, resolving pointer width, endianness, executable suffix, and shared library suffix.

## Install

```bash
cap add target-lexicon
```

## Usage

```cp
import { parseTargetTriple, exeSuffix, dylibSuffix } from "target-lexicon";

const t = parseTargetTriple("x86_64-unknown-linux-gnu");
// t.pointer_width == 64, t.is_little_endian == true, dylibSuffix(t) == ".so"
```
