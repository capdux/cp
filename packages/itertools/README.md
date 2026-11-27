[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# itertools

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:itertools` ([https://github.com/rust-itertools/itertools](https://github.com/rust-itertools/itertools))

Pure CP, zero-allocation iterator combinators and sequence algorithms (clean-room MIT parity with Rust `itertools`).

## Features

- **Reductions & Predicates**: `minmax` (single-pass min/max), `isSorted`, `isSortedDesc`, `allEqual`
- **Deduplication**: `uniqueI64` (order-preserving first-seen) and `dedupConsecutive` (run-length collapsing)
- **Sequence Combinators**: `interleave`, `intersperse`, `mergeSorted`, `partitionByThreshold`
- **String Joining**: `joinStrings` (zero-heap slice joining into caller buffer)
- **Zero-Heap Indexing**: `cartesianProductCount`, `nthCartesianPair` (O(1) random access), `chunkCount`, `windowCount`

## API

- `struct MinMaxResult { empty: bool, min: i64, max: i64 }`
- `struct PartitionResult { true_count: usize, false_count: usize }`
- `struct CartesianPair { found: bool, first: i64, second: i64 }`
- `minmax(slice: []const i64): MinMaxResult`
- `isSorted(slice: []const i64): bool`
- `isSortedDesc(slice: []const i64): bool`
- `allEqual(slice: []const i64): bool`
- `uniqueI64(src: []const i64, out: []i64): usize`
- `dedupConsecutive(src: []const i64, out: []i64): usize`
- `interleave(a: []const i64, b: []const i64, out: []i64): usize`
- `intersperse(src: []const i64, sep: i64, out: []i64): usize`
- `mergeSorted(a: []const i64, b: []const i64, out: []i64): usize`
- `joinStrings(items: []const string, sep: string, out: []u8): usize`
- `partitionByThreshold(src: []const i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult`
- `cartesianProductCount(a_len: usize, b_len: usize): usize`
- `nthCartesianPair(a: []const i64, b: []const i64, idx: usize): CartesianPair`
- `chunkCount(total_len: usize, chunk_size: usize): usize`
- `windowCount(total_len: usize, win_size: usize): usize`

## License

MIT
