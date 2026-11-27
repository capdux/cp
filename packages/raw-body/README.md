[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# raw-body

Zero-allocation HTTP request body byte-limit enforcer, Content-Length verifier, streaming chunk accumulator, and RFC 3629 UTF-8 validator for CP (clean-room parity with npm raw-body)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:raw-body` ([https://github.com/stream-utils/raw-body](https://github.com/stream-utils/raw-body))

| Metadata | Value |
| --- | --- |
| Package | `raw-body` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add raw-body
```

```cp
import {
    RawBodyOptions,
    RawBodyResult,
    RawBodyReader,
    rwbIsCont,
    rwbUnitMultiplier,
    isValidUtf8,
    parseByteLimit,
} from "raw-body";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RawBodyOptions` | `class` | Fields: `limit: usize, expected_len: usize, has_expected_len: bool, validate_utf8: bool` · Methods: `defaultOpts, withLimit` |
| `RawBodyResult` | `class` | Fields: `status: u16, bytes_written: usize` · Methods: `isOk` |
| `RawBodyReader` | `class` | Fields: `opts: RawBodyOptions, received: usize, failed_status: u16, finished: bool` · Methods: `new, checkDeclaredLength, pushChunk, abort, finish, receivedBytes` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rwbIsCont` | `(b: u8): bool` | Invokes `rwbIsCont` and returns `bool`. |
| `rwbUnitMultiplier` | `(unit: string): u64` | Invokes `rwbUnitMultiplier` and returns `u64`. |
| `isValidUtf8` | `(data: &[]u8): bool` | Checks predicate `isValidUtf8` and returns `bool`. |
| `parseByteLimit` | `(spec: string): usize?` | Parses input via `parseByteLimit` and returns `usize?`. |

## Minimal Example

```cp
import { parseByteLimit } from "raw-body";

pub fn main() {
    let result = parseByteLimit("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
