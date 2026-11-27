[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# idempotency-key

Zero-allocation IETF Idempotency-Key HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:idempotency-key` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `idempotency-key` → CP 包名 `idempotency-key`）

| Metadata | Value |
| --- | --- |
| Package | `idempotency-key` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add idempotency-key
```

```cp
import {
    idempIsOws,
    idempTrimOws,
    idempIsVisibleAscii,
    idempIsHex,
    isValidKey,
    parseKeyHeader,
    formatKeyHeader,
    isUuidV4Like,
} from "idempotency-key";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `idempIsOws` | `(c: u8): bool` | Invokes `idempIsOws` and returns `bool`. |
| `idempTrimOws` | `(s: &[]u8): &[]u8` | Invokes `idempTrimOws` and returns `&[]u8`. |
| `idempIsVisibleAscii` | `(c: u8): bool` | Invokes `idempIsVisibleAscii` and returns `bool`. |
| `idempIsHex` | `(c: u8): bool` | Invokes `idempIsHex` and returns `bool`. |
| `isValidKey` | `(key: string): bool` | Checks predicate `isValidKey` and returns `bool`. |
| `parseKeyHeader` | `(header: string, out: []u8): usize` | Parses input via `parseKeyHeader` and returns `usize`. |
| `formatKeyHeader` | `(key: string, out: []u8): usize` | Serializes/formats data via `formatKeyHeader` returning `usize`. |
| `isUuidV4Like` | `(key: string): bool` | Checks predicate `isUuidV4Like` and returns `bool`. |
| `idempMulWrap64` | `(a: u64, b: u64): u64` | Invokes `idempMulWrap64` and returns `u64`. |
| `idempFnvUpdate` | `(h: u64, data: &[]u8): u64` | Invokes `idempFnvUpdate` and returns `u64`. |
| `idempFnvByte` | `(h: u64, b: u8): u64` | Invokes `idempFnvByte` and returns `u64`. |
| `fingerprintRequest` | `(method: string, path: string, body: &[]u8): u64` | Invokes `fingerprintRequest` and returns `u64`. |
| `evaluateState` | `(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32` | Invokes `evaluateState` and returns `i32`. |

## Minimal Example

```cp
import { isValidKey } from "idempotency-key";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
