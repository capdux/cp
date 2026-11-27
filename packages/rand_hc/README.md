[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/rand-hc

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_hc` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand), `MIT OR Apache-2.0`)

Zero-heap-allocation HC-128 cryptographic stream cipher PRNG implementing Hongjun Wu's eSTREAM HC-128 specification in pure CP (inspired by Rust `rand_hc`).

## Features

- `Hc128Rng`: full 4 KiB state (`p: [512]u32`, `q: [512]u32`, `counter: usize`) with zero heap allocation
- `fromSeed(seed: [32]u8): Hc128Rng`: 128-bit key + 128-bit IV expansion (`W[0..1280]`) and 1024-step warm-up
- `nextU32(*_): u32`, `nextU64(*_): u64` (little-endian pair), `fillBytes(*_, out: []u8)` (little-endian byte stream)

## License

MIT OR Apache-2.0
