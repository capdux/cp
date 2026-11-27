[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mysql2

Zero-heap MySQL / MariaDB / TiDB connection string parser (mysql:// URI & Go-style user:pass@tcp(host:port)/db) with password redaction for CP (compatible with npm mysql2 & Rust mysql)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mysql2` ([https://github.com/sidorares/node-mysql2](https://github.com/sidorares/node-mysql2)) / `crates.io:mysql` ([https://github.com/blackbeam/rust-mysql-simple](https://github.com/blackbeam/rust-mysql-simple))（原库名 `mysql2` → CP 包名 `mysql2`）

| Metadata | Value |
| --- | --- |
| Package | `mysql2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mysql2
```

```cp
import {
    MysqlConfig,
    eqBytes,
    startsWith,
    parsePort,
    indexOf,
    lastIndexOf,
    applyQueryParams,
    splitHostPort,
} from "mysql2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MysqlConfig` | `class` | Fields: `tls: bool, host: string, port: u16, user: string, password: string, database: string, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `eqBytes` | `(a: string, b: string): bool` | Invokes `eqBytes` and returns `bool`. |
| `startsWith` | `(s: string, prefix: string): bool` | Invokes `startsWith` and returns `bool`. |
| `parsePort` | `(s: string): i32` | Parses input via `parsePort` and returns `i32`. |
| `indexOf` | `(s: string, ch: u8): i64` | Invokes `indexOf` and returns `i64`. |
| `lastIndexOf` | `(s: string, ch: u8): i64` | Invokes `lastIndexOf` and returns `i64`. |
| `applyQueryParams` | `(query: string, cfg: &mut MysqlConfig)` | Executes `applyQueryParams` in-place. |
| `splitHostPort` | `(hp: string, out_host: &mut string, out_port: &mut u16): bool` | Invokes `splitHostPort` and returns `bool`. |
| `parseUri` | `(uri: string): MysqlConfig` | Parses input via `parseUri` and returns `MysqlConfig`. |
| `parseGoDsn` | `(dsn: string): MysqlConfig` | Parses input via `parseGoDsn` and returns `MysqlConfig`. |
| `parse` | `(input: string): MysqlConfig` | Parses input via `parse` and returns `MysqlConfig`. |
| `appendStr` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendU16` | `(dst: []u8, pos: usize, val: u16): usize` | Invokes `appendU16` and returns `usize`. |
| `formatUri` | `(cfg: MysqlConfig, redact_password: bool, out: []u8): usize` | Serializes/formats data via `formatUri` returning `usize`. |

## Minimal Example

```cp
import { parsePort } from "mysql2";

pub fn main() {
    let result = parsePort("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
