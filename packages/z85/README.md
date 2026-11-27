[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# z85

Zero-allocation Base85 multi-standard encoder/decoder supporting ZeroMQ Z85 (RFC 32) and Adobe Ascii85 with 'z' zero-compression and <~...~> framing (Rust z85 / ascii85 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:z85` ([https://github.com/murar8/z85](https://github.com/murar8/z85))（原库名 `z85` → CP 包名 `z85`）

| Metadata | Value |
| --- | --- |
| Package | `z85` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add z85
```

```cp
import { encodeZ85, decodeZ85, encodeAscii85, decodeAscii85 } from "z85";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodeZ85` | `(input: string, out: []u8): string?` | Serializes/formats data via `encodeZ85` returning `string?`. |
| `decodeZ85` | `(encoded: string, out: []u8): string?` | Parses input via `decodeZ85` and returns `string?`. |
| `encodeAscii85` | `(input: string, out: []u8): string?` | Serializes/formats data via `encodeAscii85` returning `string?`. |
| `decodeAscii85` | `(encoded: string, out: []u8): string?` | Parses input via `decodeAscii85` and returns `string?`. |

## Minimal Example

```cp
import { encodeZ85 } from "z85";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeZ85("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
