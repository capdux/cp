[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/pcg32

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pcg-random` ([https://github.com/ThomWright/pcg-random](https://github.com/ThomWright/pcg-random))（原库名 `pcg-random` → CP 包名 `@cap/pcg32`）

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG in pure CP (Rust `rand_pcg` / npm `pcg-random` parity).

## Features

- `Pcg32.new(seed, stream)` & `Pcg32.fromSeed(seed)` with official PCG-XSH-RR seeding
- `nextU32()`, `nextU64()`, `nextF64()` (`[0.0, 1.0)`), unbiased `nextBounded(bound)`
- `advance(delta)`: O(log n) fast jump-ahead
- `fillBytes(out)`: zero-heap random byte buffer filling

## Usage

```cp
import { Pcg32 } from "@cap/pcg32";

let rng = Pcg32.fromSeed(42);
const r = rng.nextU32();
const dice = rng.nextBounded(6);
```
