[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# der

Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder reusing const-oid for OID TLV handling (RustCrypto der parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`der`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（原库名 `der`，CP 包名 `der`）。

| Metadata | Value |
| --- | --- |
| Package | `der` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add der
```

```cp
import {
    DerHeader,
    DerTlvResult,
    DerSliceResult,
    DerBoolResult,
    DerU64Result,
    DerBitStringResult,
    DerSequenceResult,
    DerReader,
} from "der";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DerHeader` | `class` | Fields: `tag: u8, len: usize, header_len: usize, ok: bool` |
| `DerTlvResult` | `class` | Fields: `tag: u8, value: &[]u8, ok: bool` |
| `DerSliceResult` | `class` | Fields: `data: &[]u8, ok: bool` |
| `DerBoolResult` | `class` | Fields: `value: bool, ok: bool` |
| `DerU64Result` | `class` | Fields: `value: u64, ok: bool` |
| `DerBitStringResult` | `class` | Fields: `unused_bits: u8, bytes: &[]u8, ok: bool` |
| `DerSequenceResult` | `class` | Fields: `reader: DerReader, ok: bool` |
| `DerReader` | `class` | Fields: `buf: &[]u8, pos: usize` · Methods: `new, remaining, isEmpty, peekTag, readHeader, readTlv, +7 more` |
| `DerWriter` | `class` | Fields: `buf: []u8, pos: usize` · Methods: `new, len, remaining, written, encodedLengthSize, encodeLength, +8 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `derEmptySlice` | `(): &[]u8` | Invokes `derEmptySlice` and returns `&[]u8`. |
| `derMinBytesUsize` | `(v: usize): usize` | Invokes `derMinBytesUsize` and returns `usize`. |

## Minimal Example

```cp
import { derEmptySlice } from "der";

pub fn main() {
    let result = derEmptySlice();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
