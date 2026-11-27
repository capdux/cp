[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ucd-trie

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ucd-trie` ([https://github.com/BurntSushi/ucd-generate](https://github.com/BurntSushi/ucd-generate))

Pure CP, zero-allocation compressed two-level bitmap Trie for O(1) Unicode codepoint set membership queries (parity with Rust `ucd-trie`).

## Architecture

- **64-Codepoint Chunks**: `chunk_idx = cp >> 6`, `bit_idx = cp & 63`.
- **Shared Zero Block**: `chunks[0]` is permanently `0`, so sparse Unicode sets (e.g., ASCII + selected CJK blocks across the entire 65,536-codepoint BMP) achieve 95%+ chunk compression.
- **O(1) Lookup**: `contains` performs exactly two memory reads (`level1[cp >> 6]` and `chunks[cid]`) plus a single bit test.
- **Zero Heap Allocation**: Caller supplies `level1: []u16` page table and `chunks: []u64` bitmap pool.

## API

- `UcdTrie.new(): UcdTrie`
- `init(*_, level1: []u16, chunks: []u64): bool`
- `insertCodepoint(*_, level1: []u16, chunks: []u64, cp: u32): bool`
- `insertRange(*_, level1: []u16, chunks: []u64, lo: u32, hi: u32): bool`
- `contains(level1: []u16, chunks: []u64, cp: u32): bool`
- `countMembers(level1: []u16, chunks: []u64): u64`
- `dedupChunkCount(): usize`

## License

MIT
