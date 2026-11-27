[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# tinyvec

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinyvec` ([https://github.com/Lokathor/tinyvec](https://github.com/Lokathor/tinyvec))

Clean-room MIT, 100%-safe zero-heap inline stack vectors (`TinyVecI64`, `TinyVecU32`) for CP — modeled on Rust's `tinyvec` crate.

## API
- `TinyVecI64` (`new`, `len`, `capacity`, `isEmpty`, `isFull`, `get`, `first`, `last`, `push`, `pop`, `insert`, `remove`, `swapRemove`, `truncate`, `clear`, `reverse`, `dedup`, `extendFromSlice`, `binarySearch`, `sum`)
- `TinyVecU32` (`new`, `len`, `isEmpty`, `push`, `pop`, `get`, `swapRemove`, `clear`)
