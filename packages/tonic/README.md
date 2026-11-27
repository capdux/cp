[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tonic

Zero-allocation gRPC over HTTP/2 length-prefixed frame codec, 17 status codes, and grpc-timeout parser/formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tonic` ([https://github.com/hyperium/tonic](https://github.com/hyperium/tonic)) / `npm:@grpc/grpc-js` ([https://github.com/grpc/grpc-node](https://github.com/grpc/grpc-node))（原库名 `tonic` → CP 包名 `tonic`）

| Metadata | Value |
| --- | --- |
| Package | `tonic` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tonic
```

```cp
import {
    GrpcFrameHeader,
    statusName,
    isRetryableStatus,
    encodeFrame,
    decodeHeader,
    parseTimeoutMicros,
    formatTimeoutMillis,
} from "tonic";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GrpcFrameHeader` | `class` | Fields: `compressed: bool, payload_len: u32, total_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `statusName` | `(code: u8): string` | Invokes `statusName` and returns `string`. |
| `isRetryableStatus` | `(code: u8): bool` | Checks predicate `isRetryableStatus` and returns `bool`. |
| `encodeFrame` | `(compressed: bool, payload: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeFrame` returning `usize`. |
| `decodeHeader` | `(data: &[]u8, out: &mut GrpcFrameHeader): i32` | Parses input via `decodeHeader` and returns `i32`. |
| `parseTimeoutMicros` | `(header: string): i64` | Parses input via `parseTimeoutMicros` and returns `i64`. |
| `formatTimeoutMillis` | `(ms: u64, out: []u8): usize` | Serializes/formats data via `formatTimeoutMillis` returning `usize`. |

## Minimal Example

```cp
import { parseTimeoutMicros } from "tonic";

pub fn main() {
    let result = parseTimeoutMicros("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
