[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# foldhash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:foldhash` ([https://github.com/orlp/foldhash](https://github.com/orlp/foldhash))

Zero-allocation 64×64→128-bit folded-multiply non-cryptographic hasher with Fast & Quality modes for CP — clean-room MIT parity with Rust `foldhash` (895M downloads, default hasher in `hashbrown` 0.15+).

纯 CP 零堆分配高速抗碰撞非加密哈希引擎，基于纯 64 位整数拆半实现的精确 128-bit 折叠乘法 `foldedMultiply(x, y) = lo ^ hi`，提供 Fast（内存哈希表极速模式）与 Quality（双轮雪崩高熵模式）。

## Install / 安装

```bash
cap add foldhash
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `foldedMultiply` | `(x: u64, y: u64) -> u64` | Exact 64×64→128-bit multiply returning `lo ^ hi` |
| `FoldHasher` | `initFast(seed)`, `initQuality(seed)`, `writeU8`, `writeU32`, `writeU64`, `writeBytes`, `finish() -> u64` | Streaming sponge-buffered hasher |
| `hashBytesFast` | `(data: []const u8, seed: u64) -> u64` | One-shot Fast byte-slice hash |
| `hashBytesQuality` | `(data: []const u8, seed: u64) -> u64` | One-shot Quality byte-slice hash |
| `hashU64` | `(v: u64, seed: u64) -> u64` | One-shot `u64` hash |

## Usage / 使用示例

```cp
import { FoldHasher, hashBytesFast, hashBytesQuality, foldedMultiply } from "foldhash";

const h1 = hashBytesFast("hello world", 42 as u64);
const h2 = hashBytesQuality("hello world", 42 as u64);
```
