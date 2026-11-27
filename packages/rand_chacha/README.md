[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rand_chacha

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_chacha` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand)) / `crates.io:ppv-lite86` ([https://github.com/cryptocorrosion/cryptocorrosion](https://github.com/cryptocorrosion/cryptocorrosion))（原库名 `rand_chacha` → CP 包名 `rand_chacha`）

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and seekable CSPRNG for CP.

## Features

- **Zero heap allocation**: fixed 64-byte block buffer with overflow-safe 32-bit modular arithmetic.
- **RFC 8439 compliant**: `chachaBlock` verified against RFC 8439 Section 2.3.2 official test vector; supports 8, 12, and 20 rounds.
- **Seekable CSPRNG**: `ChaCha20Rng` with `fromSeed`, `fromU64`, `nextU32`, `nextU64`, `nextF64`, `fillBytes`, and `setWordPos`.

## Usage

```cp
import { ChaCha20Rng } from "rand_chacha";

let rng = ChaCha20Rng.fromU64(42 as u64);
const x = rng.nextU64();
const u = rng.nextF64(); // [0.0, 1.0)
```
