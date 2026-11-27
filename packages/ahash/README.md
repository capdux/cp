[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ahash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ahash` ([https://github.com/tkaitchuck/aHash](https://github.com/tkaitchuck/aHash))

Zero-allocation randomized high-speed non-cryptographic hasher with 4-key `RandomState`, folded-multiply mixing, and rotation rounds for CP — clean-room MIT parity with Rust `ahash` (946M downloads on crates.io).

纯 CP 零堆分配高速随机化非加密哈希器，采用 4-key `RandomState` + 64×64→128-bit 折叠乘加与数据相关旋转混淆轮，兼顾极速吞吐与 HashDoS 抗性。

## Install / 安装

```bash
cap add ahash
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RandomState` | `withSeeds(k0, k1, k2, k3)`, `defaultState()`, `buildHasher()`, `hashOneBytes(data)`, `hashOneU64(v)` | 4-key randomized hash state builder |
| `AHasher` | `init(k0, k1, k2, k3)`, `writeU8`, `writeU32`, `writeU64`, `writeBytes`, `finish() -> u64` | Streaming tail-buffered folded-multiply hasher |
| `hashBytes` | `(data: []const u8, k0: u64, k1: u64) -> u64` | One-shot byte-slice hash |
| `hashU64` | `(v: u64, k0: u64, k1: u64) -> u64` | One-shot `u64` hash |

## Usage / 使用示例

```cp
import { RandomState, AHasher, hashBytes } from "ahash";

const rs = RandomState.withSeeds(1 as u64, 2 as u64, 3 as u64, 4 as u64);
const h = rs.hashOneBytes("hello world");
```
