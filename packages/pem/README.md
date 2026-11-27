[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pem

Zero-heap RFC 7468 PEM armor multi-block parser, validator, and 64-column wrapped encoder for certificates and keys

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-pemfile` ([https://github.com/rustls/pemfile](https://github.com/rustls/pemfile)) / `crates.io:pem-rfc7468` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))（原库名 `rustls-pemfile` → CP 包名 `pem`）

| Metadata | Value |
| --- | --- |
| Package | `pem` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pem
```

```cp
import {
    PemBlock,
    PemBundle,
    isValidLabel,
    parsePemFrom,
    parsePem,
    parsePemBundle,
    extractBase64,
    encodePem,
} from "pem";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PemBlock` | `class` | Fields: `valid: bool, label: string, raw_body: string, start_offset: usize, end_offset: usize` |
| `PemBundle` | `class` | Fields: `blocks: [16]PemBlock, count: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValidLabel` | `(label: string): bool` | Checks predicate `isValidLabel` and returns `bool`. |
| `parsePemFrom` | `(input: string, from_offset: usize): PemBlock` | Parses input via `parsePemFrom` and returns `PemBlock`. |
| `parsePem` | `(input: string): PemBlock` | Parses input via `parsePem` and returns `PemBlock`. |
| `parsePemBundle` | `(input: string): PemBundle` | Parses input via `parsePemBundle` and returns `PemBundle`. |
| `extractBase64` | `(blk: PemBlock, out: []u8): string` | Invokes `extractBase64` and returns `string`. |
| `encodePem` | `(label: string, base64_payload: string, out: []u8): string` | Serializes/formats data via `encodePem` returning `string`. |

## Minimal Example

```cp
import { isValidLabel } from "pem";

pub fn main() {
    let result = isValidLabel("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
