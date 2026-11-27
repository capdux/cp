[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bumpalo

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bumpalo` ([https://github.com/fitzgen/bumpalo](https://github.com/fitzgen/bumpalo))

Zero-allocation alignment-aware bump arena allocator (`BumpArena`, `BumpSlice`, `BUMP_CAPACITY`) with strict power-of-two alignment, overflow protection, and `checkpoint`/`rewind` for CP — clean-room MIT parity with Rust `bumpalo` v3 (850M+ downloads).

纯 CP 零堆分配对齐感知线性 Bump Arena 分配器，适用于编译器 AST 临时节点、短生命周期请求缓冲、解析器帧内存与高性能批处理场景。

## Install / 安装

```bash
cap add @cap/bumpalo
```

## API

### Constants & Handle

| Symbol | Signature | Description |
| --- | --- | --- |
| `BUMP_CAPACITY` | `const usize = 4096` | Fixed static backing buffer size (4 KiB) |
| `BumpSlice` | `{ offset: usize, len: usize }` | Allocated slice handle with `empty()`, `isEmpty()`, `end()` |

### `BumpArena`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> BumpArena` | Initializes a zeroed 4096-byte bump arena |
| `capacity` / `allocatedBytes` / `remaining` / `allocationCount` | `() -> usize` | Inspect arena capacity, cursor, free bytes, and allocation count |
| `reset` | `(*_)` | O(1) reset of cursor and allocation count |
| `allocAligned` | `(*_, size: usize, alignment: usize) -> BumpSlice?` | Allocates zeroed memory aligned to `1/2/4/8/16/32/64` bytes |
| `allocBytes` | `(*_, size: usize) -> BumpSlice?` | Allocates 1-byte-aligned zeroed bytes |
| `allocSliceCopy` | `(*_, src: []const u8) -> BumpSlice?` | Copies byte slice into the arena |
| `allocStr` | `(*_, s: string) -> BumpSlice?` | Copies UTF-8 string bytes into the arena |
| `readByte` / `writeByte` | `(handle, idx[, val]) -> u8? / bool` | Bounds-checked & cursor-validated byte access |
| `matchesStr` | `(handle: BumpSlice, expected: string) -> bool` | Compares allocated slice against a string |
| `checkpoint` / `rewind` | `() -> usize` / `(*_, mark: usize) -> bool` | Captures and restores arena cursor position |

## Usage / 使用示例

```cp
import { BumpArena, BumpSlice } from "@cap/bumpalo";

let bump = BumpArena.new();
let mark = bump.checkpoint();
let s = bump.allocStr("hello bump") ?? BumpSlice.empty();
const ok = bump.matchesStr(s, "hello bump"); // true
* = bump.rewind(mark);
```
