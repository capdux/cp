[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# syslog

Zero-heap RFC 5424 Syslog message parser and formatter with PRI facility/severity codec, structured data skipping, and NILVALUE handling.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:syslog` ([https://github.com/Geal/rust-syslog](https://github.com/Geal/rust-syslog))

| Metadata | Value |
| --- | --- |
| Package | `syslog` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add syslog
```

```cp
import {
    SyslogMessage,
    TokenSpan,
    encodePri,
    decodeFacility,
    decodeSeverity,
    readSpToken,
    parseRfc5424,
    appendSlice,
} from "syslog";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SyslogMessage` | `class` | Fields: `facility: u8, severity: u8, version: u8, ts_start: usize, ts_len: usize, host_start: usize, +10 more` |
| `TokenSpan` | `class` | Fields: `start: usize, len: usize, next_pos: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodePri` | `(facility: u8, severity: u8): u8` | Serializes/formats data via `encodePri` returning `u8`. |
| `decodeFacility` | `(pri: u8): u8` | Parses input via `decodeFacility` and returns `u8`. |
| `decodeSeverity` | `(pri: u8): u8` | Parses input via `decodeSeverity` and returns `u8`. |
| `readSpToken` | `(raw: &[]u8, pos: usize): TokenSpan` | Invokes `readSpToken` and returns `TokenSpan`. |
| `parseRfc5424` | `(raw: &[]u8): SyslogMessage` | Parses input via `parseRfc5424` and returns `SyslogMessage`. |
| `appendSlice` | `(dst: []u8, pos: usize, s: &[]u8): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendFieldOrNil` | `(dst: []u8, pos: usize, field: &[]u8): i64` | Invokes `appendFieldOrNil` and returns `i64`. |
| `appendU8Dec` | `(dst: []u8, pos: usize, val: u8): i64` | Invokes `appendU8Dec` and returns `i64`. |
| `formatRfc5424` | `(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64` | Serializes/formats data via `formatRfc5424` returning `i64`. |

## Minimal Example

```cp
import { encodePri } from "syslog";

pub fn main() {
    let result = encodePri(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
