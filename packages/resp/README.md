[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# resp

Zero-heap Redis RESP2 wire protocol streaming frame parser and command encoder.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:resp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `resp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add resp
```

```cp
import {
    RespFrame,
    IntResult,
    makeFrame,
    parseI64,
    findCrlf,
    parseFrame,
    appendByte,
    appendSlice,
} from "resp";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RespFrame` | `class` | Fields: `kind: u8, data_start: usize, data_len: usize, int_val: i64, array_len: i64, consumed: usize` |
| `IntResult` | `class` | Fields: `value: i64, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeFrame` | `(kind: u8): RespFrame` | Invokes `makeFrame` and returns `RespFrame`. |
| `parseI64` | `(s: &[]u8): IntResult` | Parses input via `parseI64` and returns `IntResult`. |
| `findCrlf` | `(buf: &[]u8, start: usize): i64` | Queries/retrieves data via `findCrlf` returning `i64`. |
| `parseFrame` | `(buf: &[]u8): RespFrame` | Parses input via `parseFrame` and returns `RespFrame`. |
| `appendByte` | `(dst: []u8, pos: usize, b: u8): i64` | Invokes `appendByte` and returns `i64`. |
| `appendSlice` | `(dst: []u8, pos: usize, s: &[]u8): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendI64` | `(dst: []u8, pos: usize, v: i64): i64` | Invokes `appendI64` and returns `i64`. |
| `encodeSimpleString` | `(s: &[]u8, dst: []u8): i64` | Serializes/formats data via `encodeSimpleString` returning `i64`. |
| `encodeInteger` | `(v: i64, dst: []u8): i64` | Serializes/formats data via `encodeInteger` returning `i64`. |
| `encodeNull` | `(dst: []u8): i64` | Serializes/formats data via `encodeNull` returning `i64`. |
| `encodeBulkString` | `(s: &[]u8, dst: []u8): i64` | Serializes/formats data via `encodeBulkString` returning `i64`. |
| `encodeCommand` | `(args: &[]&[]u8, dst: []u8): i64` | Serializes/formats data via `encodeCommand` returning `i64`. |

## Minimal Example

```cp
import { parseI64 } from "resp";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseI64(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
