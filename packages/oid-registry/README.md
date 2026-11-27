[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# oid-registry

Zero-allocation static X.509/PKCS/TLS/NIST/RFC OID registry with DER OID encoder and decoder (Rust oid-registry parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oid-registry` ([https://github.com/rusticata/oid-registry](https://github.com/rusticata/oid-registry))

| Metadata | Value |
| --- | --- |
| Package | `oid-registry` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add oid-registry
```

```cp
import {
    OidEntry,
    OidrDecodeStep,
    oidrEntryAt,
    oidrStrEq,
    oidEntryCount,
    lookupOid,
    lookupShortName,
    oidrEncodeBase128,
} from "oid-registry";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OidEntry` | `class` | Fields: `oid: string, short_name: string, description: string, category: string` |
| `OidrDecodeStep` | `class` | Fields: `val: u64, next_pos: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `oidrEntryAt` | `(idx: usize): OidEntry` | Invokes `oidrEntryAt` and returns `OidEntry`. |
| `oidrStrEq` | `(a: string, b: string): bool` | Invokes `oidrStrEq` and returns `bool`. |
| `oidEntryCount` | `(): usize` | Invokes `oidEntryCount` and returns `usize`. |
| `lookupOid` | `(dotted: string): OidEntry?` | Queries/retrieves data via `lookupOid` returning `OidEntry?`. |
| `lookupShortName` | `(sn: string): OidEntry?` | Queries/retrieves data via `lookupShortName` returning `OidEntry?`. |
| `oidrEncodeBase128` | `(val: u64, out: []u8, pos: usize): usize?` | Invokes `oidrEncodeBase128` and returns `usize?`. |
| `oidrWriteU64Dec` | `(val: u64, out: []u8, pos: usize): usize?` | Invokes `oidrWriteU64Dec` and returns `usize?`. |
| `encodeOidDer` | `(dotted: string, out: []u8): usize` | Serializes/formats data via `encodeOidDer` returning `usize`. |
| `oidrDecodeBase128` | `(der: &[]u8, start: usize): OidrDecodeStep` | Invokes `oidrDecodeBase128` and returns `OidrDecodeStep`. |
| `decodeOidDer` | `(der: &[]u8, out: []u8): usize` | Parses input via `decodeOidDer` and returns `usize`. |

## Minimal Example

```cp
import { encodeOidDer } from "oid-registry";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeOidDer("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
