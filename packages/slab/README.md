[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# slab

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slab` ([https://github.com/tokio-rs/slab](https://github.com/tokio-rs/slab))（原库名 `slab` → CP 包名 `slab`）

Clean-room MIT, zero-heap pre-allocated O(1) `SlabI64` allocator with intrusive free-list for CP — modeled on Rust's `slab` crate.

## API
- `SlabI64.new()` (capacity `SLAB_CAP = 128`)
- `insert`, `remove`, `get`, `set`, `contains`, `vacantKey`, `nextOccupied`, `clear`
- `len`, `capacity`, `isEmpty`, `isFull`
