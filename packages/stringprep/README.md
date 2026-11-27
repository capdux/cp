[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# stringprep

Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation for SCRAM-SHA-256, PostgreSQL, and LDAP in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:stringprep` ([https://github.com/sfackler/rust-stringprep](https://github.com/sfackler/rust-stringprep))

| Metadata | Value |
| --- | --- |
| Package | `stringprep` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add stringprep
```

```cp
import {
    PrepResult,
    DecCp,
    decodeUtf8,
    encodeUtf8,
    isMappedToNothing,
    isNonAsciiSpace,
    isSaslProhibited,
    saslprep,
} from "stringprep";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PrepResult` | `class` | Fields: `len: usize, ok: bool` |
| `DecCp` | `class` | Fields: `cp: u32, width: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeUtf8` | `(s: string, i: usize): DecCp` | Parses input via `decodeUtf8` and returns `DecCp`. |
| `encodeUtf8` | `(cp: u32, out: []u8, pos: usize): i64` | Serializes/formats data via `encodeUtf8` returning `i64`. |
| `isMappedToNothing` | `(cp: u32): bool` | Checks predicate `isMappedToNothing` and returns `bool`. |
| `isNonAsciiSpace` | `(cp: u32): bool` | Checks predicate `isNonAsciiSpace` and returns `bool`. |
| `isSaslProhibited` | `(cp: u32): bool` | Checks predicate `isSaslProhibited` and returns `bool`. |
| `saslprep` | `(input: string, out: []u8): PrepResult` | Invokes `saslprep` and returns `PrepResult`. |
| `nameprepAscii` | `(input: string, out: []u8): PrepResult` | Invokes `nameprepAscii` and returns `PrepResult`. |

## Minimal Example

```cp
import { decodeUtf8 } from "stringprep";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
