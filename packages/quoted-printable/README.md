[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# quoted-printable

Zero-allocation RFC 2045 Section 6.7 MIME Quoted-Printable encoder and decoder with 76-col soft line breaks in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quoted-printable` ([https://github.com/mathiasbynens/quoted-printable](https://github.com/mathiasbynens/quoted-printable)) / `crates.io:quoted_printable` ([https://github.com/staktrace/quoted-printable](https://github.com/staktrace/quoted-printable))

| Metadata | Value |
| --- | --- |
| Package | `quoted-printable` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add quoted-printable
```

```cp
import { isDirectPrintable, encode, decode } from "quoted-printable";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isDirectPrintable` | `(b: u8): bool` | Checks predicate `isDirectPrintable` and returns `bool`. |
| `encode` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |

## Minimal Example

```cp
import { decode } from "quoted-printable";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
