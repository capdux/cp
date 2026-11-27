[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# const-oid

Pure CP zero-allocation ITU-T X.690 ASN.1 Object Identifier (OID) parser, encoder, and comparator (parity with RustCrypto const-oid)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`const-oid`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（包名 `const-oid`）。

| Metadata | Value |
| --- | --- |
| Package | `const-oid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add const-oid
```

```cp
import {
    OidParseResult,
    ObjectIdentifier,
    coidZeroArcs,
    coidZeroDer,
    coidEncodeBase128U64,
    coidEncodeArcsToDer,
    coidWriteU32Dec,
} from "const-oid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OidParseResult` | `class` | Fields: `ok: bool, oid: ObjectIdentifier` |
| `ObjectIdentifier` | `class` | Fields: `arcs: [16]u32, len: usize, der_buf: [80]u8, der_len: usize` · Methods: `empty, fromStr, fromDer, fromDerTlv, toDer, toDerTlv, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `coidZeroArcs` | `(): [16]u32` | Invokes `coidZeroArcs` and returns `[16]u32`. |
| `coidZeroDer` | `(): [80]u8` | Invokes `coidZeroDer` and returns `[80]u8`. |
| `coidEncodeBase128U64` | `(val: u64, out: []u8, start_pos: usize): usize` | Invokes `coidEncodeBase128U64` and returns `usize`. |
| `coidEncodeArcsToDer` | `(arcs: [16]u32, arc_len: usize, der_out: []u8): usize` | Invokes `coidEncodeArcsToDer` and returns `usize`. |
| `coidWriteU32Dec` | `(val: u32, out: []u8, start_pos: usize): usize` | Invokes `coidWriteU32Dec` and returns `usize`. |

## Minimal Example

```cp
import { coidZeroArcs } from "const-oid";

pub fn main() {
    let result = coidZeroArcs();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
