[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# indexmap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indexmap` ([https://github.com/indexmap-rs/indexmap](https://github.com/indexmap-rs/indexmap))

Pure CP, zero-allocation insertion-order-preserving hash map (clean-room MIT parity with Rust `indexmap` v2).

## Dependencies

- `@sindresorhus/fnv1a` — Reuses `fnv1a64` for open-addressing hash indexing with zero duplication.

## Architecture

Follows `indexmap`'s signature two-array layout:
- **Dense insertion-order entry array**: O(1) indexed access (`getIndex`, `first`, `last`) and cache-friendly iteration in exact insertion order.
- **Open-addressing hash index table**: O(1) key lookup (`get`, `getIndexOf`, `containsKey`), in-place value update preserving order, O(1) `swapRemove`, and order-preserving `shiftRemove`.

## API

- `IndexMapU64` (capacity 64, 128 hash buckets):
  - `new()`, `len()`, `capacity()`, `isEmpty()`, `clear(*_)`
  - `insert(*_, key: u64, val: i64): InsertResult`
  - `get(key: u64): GetValResult`, `getIndexOf(key: u64): i64`, `containsKey(key: u64): bool`
  - `getIndex(idx: usize): GetEntryResult`, `first()`, `last()`
  - `swapRemove(*_, key: u64): GetValResult`, `shiftRemove(*_, key: u64): GetValResult`, `pop(*_): GetEntryResult`
- `IndexMapStr` (capacity 32, 64 hash buckets, keys up to 64 bytes):
  - Same full API for `string -> i64` ordered mappings.

## License

MIT
