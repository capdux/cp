[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pg-connection-string

Zero-heap PostgreSQL connection string parser and URI builder supporting both postgresql:// URIs and libpq key=value DSNs.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pg-connection-string` ([https://github.com/brianc/node-postgres](https://github.com/brianc/node-postgres)) / `crates.io:tokio-postgres` ([https://github.com/sfackler/rust-postgres](https://github.com/sfackler/rust-postgres))（原库名 `pg-connection-string` → CP 包名 `pg-connection-string`）

| Metadata | Value |
| --- | --- |
| Package | `pg-connection-string` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pg-connection-string
```

```cp
import {
    PgConfig,
    makeInvalid,
    isWs,
    parsePortStrict,
    isValidSslMode,
    parseUri,
    parseKeyValue,
    parse,
} from "pg-connection-string";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PgConfig` | `class` | Fields: `host: string, port: u16, user: string, password: string, database: string, sslmode: string, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeInvalid` | `(): PgConfig` | Invokes `makeInvalid` and returns `PgConfig`. |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `parsePortStrict` | `(s: string): i32` | Parses input via `parsePortStrict` and returns `i32`. |
| `isValidSslMode` | `(mode: string): bool` | Checks predicate `isValidSslMode` and returns `bool`. |
| `parseUri` | `(uri: string): PgConfig` | Parses input via `parseUri` and returns `PgConfig`. |
| `parseKeyValue` | `(dsn: string): PgConfig` | Parses input via `parseKeyValue` and returns `PgConfig`. |
| `parse` | `(input: string): PgConfig` | Parses input via `parse` and returns `PgConfig`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendU16` | `(out: []u8, pos: usize, v: u16): i64` | Invokes `appendU16` and returns `i64`. |
| `formatUri` | `(cfg: PgConfig, include_password: bool, out: []u8): usize` | Serializes/formats data via `formatUri` returning `usize`. |

## Minimal Example

```cp
import { parsePortStrict } from "pg-connection-string";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
