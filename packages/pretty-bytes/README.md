[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pretty-bytes

Pure CP zero-allocation human-readable byte and bit size formatter supporting SI/IEC, bits, signed, and compact modes (npm pretty-bytes parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-bytes` ([https://github.com/sindresorhus/pretty-bytes](https://github.com/sindresorhus/pretty-bytes))

| Metadata | Value |
| --- | --- |
| Package | `pretty-bytes` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pretty-bytes
```

```cp
import {
    PrettyBytesOpts,
    pbUnitStr,
    pbAppendStr,
    pbAppendByte,
    pbAppendU64,
    prettyBytesWith,
    prettyBytes,
} from "pretty-bytes";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PrettyBytesOpts` | `class` | Fields: `binary: bool, bits: bool, signed_val: bool, space: bool, precision: u8` · Methods: `defaults` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pbUnitStr` | `(idx: usize, binary: bool, bits: bool): string` | Invokes `pbUnitStr` and returns `string`. |
| `pbAppendStr` | `(out: []u8, pos: &mut usize, s: string): bool` | Invokes `pbAppendStr` and returns `bool`. |
| `pbAppendByte` | `(out: []u8, pos: &mut usize, ch: u8): bool` | Invokes `pbAppendByte` and returns `bool`. |
| `pbAppendU64` | `(out: []u8, pos: &mut usize, v: u64): bool` | Invokes `pbAppendU64` and returns `bool`. |
| `prettyBytesWith` | `(num: f64, opts: PrettyBytesOpts, out: []u8): usize` | Invokes `prettyBytesWith` and returns `usize`. |
| `prettyBytes` | `(num: f64, out: []u8): usize` | Invokes `prettyBytes` and returns `usize`. |

## Minimal Example

```cp
import { pbAppendStr } from "pretty-bytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbAppendStr(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
