[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vec-map

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vec_map` ([https://github.com/contain-rs/vec-map](https://github.com/contain-rs/vec-map), `MIT OR Apache-2.0`)

Zero-heap-allocation `O(1)` direct-indexed sparse map for small `usize` keys (`0..127`) backed by a fixed 128-slot array and a 128-bit presence bitmap in pure CP (inspired by `contain-rs/vec-map`).

## Features

- `VECMAP_CAP = 128`: fixed key domain `0..=127` with zero heap allocation
- `VecMapI64`:
  - `new()`, `len()`, `isEmpty()`, `containsKey(k)`
  - `get(k): i64?`, `insert(*_, k, v): i64?` (returns previous value if replaced), `remove(*_, k): i64?`, `clear(*_)`
  - `keys(out: []usize): usize` and `values(out: []i64): usize` in ascending key order
  - `retainGreaterThan(*_, min_val: i64): usize` (retains entries with `value > min_val`, returns remaining count)

## License

MIT OR Apache-2.0
