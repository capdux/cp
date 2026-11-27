[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sqlstring

Zero-allocation ANSI SQL / PostgreSQL / MySQL string literal, identifier, and LIKE pattern escaper with placeholder counter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqlstring` ([https://github.com/mysqljs/sqlstring](https://github.com/mysqljs/sqlstring)) / `npm:pg-format` ([https://github.com/datalanche/node-pg-format](https://github.com/datalanche/node-pg-format))（原库名 `sqlstring` → CP 包名 `sqlstring`）

| Metadata | Value |
| --- | --- |
| Package | `sqlstring` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sqlstring
```

```cp
import {
    escapeQuoted,
    escapeStringAnsi,
    escapeIdentifierAnsi,
    escapeIdentifierMysql,
    escapeLike,
    isSafeUnquotedIdent,
    countPlaceholders,
} from "sqlstring";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `escapeQuoted` | `(input: string, quote: u8, allow_empty: bool, out: []u8): usize` | Invokes `escapeQuoted` and returns `usize`. |
| `escapeStringAnsi` | `(input: string, out: []u8): usize` | Invokes `escapeStringAnsi` and returns `usize`. |
| `escapeIdentifierAnsi` | `(ident: string, out: []u8): usize` | Invokes `escapeIdentifierAnsi` and returns `usize`. |
| `escapeIdentifierMysql` | `(ident: string, out: []u8): usize` | Invokes `escapeIdentifierMysql` and returns `usize`. |
| `escapeLike` | `(pattern: string, escape_char: u8, out: []u8): usize` | Invokes `escapeLike` and returns `usize`. |
| `isSafeUnquotedIdent` | `(ident: string): bool` | Checks predicate `isSafeUnquotedIdent` and returns `bool`. |
| `countPlaceholders` | `(sql: string): usize` | Invokes `countPlaceholders` and returns `usize`. |

## Minimal Example

```cp
import { isSafeUnquotedIdent } from "sqlstring";

pub fn main() {
    let result = isSafeUnquotedIdent("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
