[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# id-arena

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:id-arena` ([https://github.com/fitzgen/id-arena](https://github.com/fitzgen/id-arena), `MIT OR Apache-2.0`)

Zero-heap-allocation typed handle arena allocator with `arena_id` provenance validation to prevent cross-arena handle misuse in pure CP (inspired by `fitzgen/id-arena`).

## Features

- `ARENA_CAP = 128`, `ArenaId { index: usize, arena_id: u32 }`
- `IdArenaI64` & `IdArenaU64`:
  - `new(arena_id: u32)`, `alloc(*_, val): ArenaId?`, `get(id): T?`, `set(*_, id, val): bool`
  - `len()`, `isEmpty()`, `capacity()`, `contains(id)`, `clear(*_)`
  - Rejects foreign `ArenaId` handles whose `arena_id` does not match or whose `index` is out of bounds

## License

MIT OR Apache-2.0
