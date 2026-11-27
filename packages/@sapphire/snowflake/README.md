[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @sapphire/snowflake

Zero-heap 64-bit Twitter/Discord Snowflake distributed ID generator, decomposer, clock-rollback guard, and decimal formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sapphire/snowflake` ([https://github.com/sapphiredev/utilities](https://github.com/sapphiredev/utilities)) / `crates.io:sonyflake` ([https://github.com/sony/sonyflake](https://github.com/sony/sonyflake))（原库名 `@sapphire/snowflake` → CP 包名 `@sapphire/snowflake`）

| Metadata | Value |
| --- | --- |
| Package | `@sapphire/snowflake` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @sapphire/snowflake
```

```cp
import {
    SnowflakeParts,
    SnowflakeGenerator,
    compose,
    decompose,
    initGenerator,
    nextId,
    formatId,
    parseId,
} from "@sapphire/snowflake";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SnowflakeParts` | `class` | Fields: `timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16` |
| `SnowflakeGenerator` | `class` | Fields: `epoch_ms: u64, datacenter_id: u8, worker_id: u8, last_ts_ms: u64, sequence: u16` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `compose` | `(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16): u64` | Invokes `compose` and returns `u64`. |
| `decompose` | `(id: u64, epoch_ms: u64): SnowflakeParts` | Invokes `decompose` and returns `SnowflakeParts`. |
| `initGenerator` | `(epoch_ms: u64, datacenter_id: u8, worker_id: u8): SnowflakeGenerator` | Constructs/initializes an instance via `initGenerator`. |
| `nextId` | `(gen: &mut SnowflakeGenerator, now_ms: u64): i64` | Invokes `nextId` and returns `i64`. |
| `formatId` | `(id: u64, out: []u8): usize` | Serializes/formats data via `formatId` returning `usize`. |
| `parseId` | `(s: string): i64` | Parses input via `parseId` and returns `i64`. |

## Minimal Example

```cp
import { parseId } from "@sapphire/snowflake";

pub fn main() {
    let result = parseId("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
