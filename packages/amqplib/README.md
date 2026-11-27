[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# amqplib

Zero-allocation AMQP 0-9-1 / RabbitMQ URI parser (amqp/amqps, IPv6, %2F vhost, heartbeat) and password-redacting formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amqplib` ([https://github.com/amqp-node/amqplib](https://github.com/amqp-node/amqplib)) / `crates.io:lapin` ([https://github.com/amqp-rs/lapin](https://github.com/amqp-rs/lapin))（原库名 `amqplib` → CP 包名 `amqplib`）

| Metadata | Value |
| --- | --- |
| Package | `amqplib` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add amqplib
```

```cp
import {
    AmqpConfig,
    invalidAmqp,
    amqpIsSpace,
    parseU16Strict,
    parseAmqpUrl,
    pushSlice,
    pushU16,
    formatAmqpUrl,
} from "amqplib";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AmqpConfig` | `class` | Fields: `tls: bool, host: string, port: u16, username: string, password: string, vhost: string, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidAmqp` | `(): AmqpConfig` | Invokes `invalidAmqp` and returns `AmqpConfig`. |
| `amqpIsSpace` | `(b: u8): bool` | Invokes `amqpIsSpace` and returns `bool`. |
| `parseU16Strict` | `(s: string): i32` | Parses input via `parseU16Strict` and returns `i32`. |
| `parseAmqpUrl` | `(url: string): AmqpConfig` | Parses input via `parseAmqpUrl` and returns `AmqpConfig`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushU16` | `(dst: []u8, pos: usize, val: u16): i64` | Mutates or appends state via `pushU16`. |
| `formatAmqpUrl` | `(cfg: AmqpConfig, redact_password: bool, out: []u8): usize` | Serializes/formats data via `formatAmqpUrl` returning `usize`. |

## Minimal Example

```cp
import { parseU16Strict } from "amqplib";

pub fn main() {
    let result = parseU16Strict("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
