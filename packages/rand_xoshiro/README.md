[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/xoshiro256`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xoshiro` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs))（原库名 `rand_xoshiro` → CP 包名 `@cap/xoshiro256`）

Zero-heap-allocation Blackman & Vigna `xoshiro256**` PRNG with `SplitMix64` seeding, unbiased `nextRange`, `2^128` jump substreams, and Fisher-Yates `shuffleU32` in pure CP.

## Features

- **`seedFromU64` & `splitmix64Next`**: Full 256-bit state expansion from any 64-bit seed (including `0`).
- **`nextU64` & `nextF64`**: Official `xoshiro256**` (`rotl(s[1] * 5, 7) * 9`) and 53-bit uniform `[0.0, 1.0)` floats.
- **`nextRange`**: Unbiased rejection-sampled bounded integers.
- **`jump`**: Official `2^128`-step polynomial jump for independent parallel substreams.
- **`shuffleU32`**: In-place Fisher-Yates shuffle.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    Xoshiro256,
    splitmix64Next,
    seedFromU64,
    nextU64,
    nextF64,
    nextRange,
    jump,
    shuffleU32,
} from "@cap/xoshiro256";
```

## License

MIT
