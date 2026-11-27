[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# redis

Zero-allocation Redis / Valkey / Sentinel connection URL parser (redis://, rediss://, unix://) and password-redacting formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`redis`](https://github.com/redis-rs/redis-rs)（`BSD-3-Clause`）（原库名 `redis`，CP 包名 `redis`）。

| Metadata | Value |
| --- | --- |
| Package | `redis` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add redis
```

```cp
import {
    RedisConfig,
    ruStartsWithIgnoreCase,
    ruParseStrictU16,
    ruWrite,
    ruWriteU16,
    parseRedisUrl,
    formatRedisUrl,
} from "redis";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RedisConfig` | `class` | Fields: `tls: bool, is_unix: bool, host: string, port: u16, username: string, password: string, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ruStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | Invokes `ruStartsWithIgnoreCase` and returns `bool`. |
| `ruParseStrictU16` | `(s: string): u16?` | Invokes `ruParseStrictU16` and returns `u16?`. |
| `ruWrite` | `(out: []u8, pos: &mut usize, src: string): bool` | Invokes `ruWrite` and returns `bool`. |
| `ruWriteU16` | `(out: []u8, pos: &mut usize, val: u16): bool` | Invokes `ruWriteU16` and returns `bool`. |
| `parseRedisUrl` | `(url: string): RedisConfig` | Parses input via `parseRedisUrl` and returns `RedisConfig`. |
| `formatRedisUrl` | `(cfg: RedisConfig, redact_password: bool, out: []u8): usize` | Serializes/formats data via `formatRedisUrl` returning `usize`. |

## Minimal Example

```cp
import { parseRedisUrl } from "redis";

pub fn main() {
    let result = parseRedisUrl("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
