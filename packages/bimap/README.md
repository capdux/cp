[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/bimap`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bimap` ([https://github.com/billyrieger/bimap-rs](https://github.com/billyrieger/bimap-rs))

Zero-heap bidirectional 1-to-1 map (`left <-> right`) in pure CP, modeled after Rust's `bimap` crate. Maintains strict bijective uniqueness across both left and right keys with zero dynamic allocations.

零堆分配纯 CP 双向映射表（Bijection Map），对标 Rust `bimap`，同时保证左键与右键的严格唯一性，适用于协议编号映射、ID 双向索引与符号表。

## Installation

```bash
cap add @cap/bimap
```

## API

| Method | Description |
| --- | --- |
| `BiMap.init()` | Create an empty bidirectional map (capacity `128`) |
| `insert(left, right)` | Insert `(left, right)`, evicting conflicting bindings to preserve bijection |
| `insertNoOverwrite(left, right)` | Insert only if neither `left` nor `right` already exists |
| `getByLeft(left)` / `getByRight(right)` | Lookup paired value (`-1` if absent) |
| `containsLeft(left)` / `containsRight(right)` | Check key membership |
| `removeByLeft(left)` / `removeByRight(right)` | Remove pair by either side |
| `len()` / `isEmpty()` / `capacity()` / `clear()` | Size and lifecycle helpers |

## License

MIT
