[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mongodb-connection-string-url

Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser, option extractor, and password-redacting formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mongodb-connection-string-url` ([https://github.com/mongodb-js/devtools-shared](https://github.com/mongodb-js/devtools-shared)) / `crates.io:mongodb` ([https://github.com/mongodb/mongo-rust-driver](https://github.com/mongodb/mongo-rust-driver))（原库名 `mongodb-connection-string-url` → CP 包名 `mongodb-connection-string-url`）

| Metadata | Value |
| --- | --- |
| Package | `mongodb-connection-string-url` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mongodb-connection-string-url
```

```cp
import {
    MongoHost,
    MongoUri,
    muStartsWithIgnoreCase,
    muParsePort,
    muWrite,
    muWriteU16,
    muEmptyResult,
    parseMongoUri,
} from "mongodb-connection-string-url";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MongoHost` | `class` | Fields: `host: string, port: u16` |
| `MongoUri` | `class` | Fields: `is_srv: bool, username: string, password: string, hosts: [8]MongoHost, host_count: usize, database: string, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `muStartsWithIgnoreCase` | `(s: &[]u8, prefix: &[]u8): bool` | Invokes `muStartsWithIgnoreCase` and returns `bool`. |
| `muParsePort` | `(s: &[]u8, out: &mut u16): bool` | Invokes `muParsePort` and returns `bool`. |
| `muWrite` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `muWrite` and returns `bool`. |
| `muWriteU16` | `(out: []u8, pos: &mut usize, val: u16): bool` | Invokes `muWriteU16` and returns `bool`. |
| `muEmptyResult` | `(): MongoUri` | Invokes `muEmptyResult` and returns `MongoUri`. |
| `parseMongoUri` | `(uri: string): MongoUri` | Parses input via `parseMongoUri` and returns `MongoUri`. |
| `getQueryOption` | `(uri: string, key: string): string` | Queries/retrieves data via `getQueryOption` returning `string`. |
| `formatMongoUri` | `(cfg: MongoUri, redact_password: bool, out: []u8): usize` | Serializes/formats data via `formatMongoUri` returning `usize`. |

## Minimal Example

```cp
import { parseMongoUri } from "mongodb-connection-string-url";

pub fn main() {
    let result = parseMongoUri("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
