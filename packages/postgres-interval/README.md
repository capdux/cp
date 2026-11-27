[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# postgres-interval

Zero-allocation PostgreSQL INTERVAL parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-interval` ([https://github.com/bendrucker/postgres-interval](https://github.com/bendrucker/postgres-interval))

| Metadata | Value |
| --- | --- |
| Package | `postgres-interval` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add postgres-interval
```

```cp
import {
    PgInterval,
    PgivWriter,
    PgivIntRes,
    PgivFracRes,
    pgivInvalid,
    pgivZero,
    pgivSkipSpace,
    pgivParseUint,
} from "postgres-interval";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PgInterval` | `class` | Fields: `valid: bool, years: i32, months: i32, days: i32, hours: i32, minutes: i32, +2 more` |
| `PgivWriter` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `writeByte, putStr, putU64, writeI32, writeU64Pad2, writeMsTrimmed` |
| `PgivIntRes` | `class` | Fields: `ok: bool, val: i64, next: usize` |
| `PgivFracRes` | `class` | Fields: `ms: i32, next: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pgivInvalid` | `(): PgInterval` | Invokes `pgivInvalid` and returns `PgInterval`. |
| `pgivZero` | `(): PgInterval` | Invokes `pgivZero` and returns `PgInterval`. |
| `pgivSkipSpace` | `(s: string, start: usize): usize` | Invokes `pgivSkipSpace` and returns `usize`. |
| `pgivParseUint` | `(s: string, start: usize): PgivIntRes` | Invokes `pgivParseUint` and returns `PgivIntRes`. |
| `pgivParseFracMs` | `(s: string, start: usize): PgivFracRes` | Invokes `pgivParseFracMs` and returns `PgivFracRes`. |
| `pgivAbsI32ToU64` | `(v: i32): u64` | Invokes `pgivAbsI32ToU64` and returns `u64`. |
| `pgivParseIso` | `(s: string, start: usize, end: usize): PgInterval` | Invokes `pgivParseIso` and returns `PgInterval`. |
| `parsePgInterval` | `(input: string): PgInterval` | Parses input via `parsePgInterval` and returns `PgInterval`. |
| `toIsoDuration` | `(iv: PgInterval, out: []u8): usize` | Invokes `toIsoDuration` and returns `usize`. |
| `toPostgresString` | `(iv: PgInterval, out: []u8): usize` | Invokes `toPostgresString` and returns `usize`. |
| `totalSecondsApprox` | `(iv: PgInterval): i64` | Invokes `totalSecondsApprox` and returns `i64`. |

## Minimal Example

```cp
import { parsePgInterval } from "postgres-interval";

pub fn main() {
    let result = parsePgInterval("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
