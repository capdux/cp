[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# futoin-hkdf

Pure CP zero-allocation RFC 5869 HKDF-SHA256 key derivation function (RustCrypto hkdf parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:futoin-hkdf` ([https://github.com/futoin/util-js-hkdf](https://github.com/futoin/util-js-hkdf))（原库名 `futoin-hkdf` → CP 包名 `futoin-hkdf`）

| Metadata | Value |
| --- | --- |
| Package | `futoin-hkdf` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add futoin-hkdf
```

```cp
import { hkdfStreamingHmac, hkdfExtract, hkdfExpand, hkdfDerive } from "futoin-hkdf";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hkdfStreamingHmac` | `(prk32: []u8, prev_t: []u8, info: []u8, counter: u8, out32: []u8)` | Executes `hkdfStreamingHmac` in-place. |
| `hkdfExtract` | `(salt: []u8, ikm: []u8, out_prk32: []u8): bool` | Invokes `hkdfExtract` and returns `bool`. |
| `hkdfExpand` | `(prk32: []u8, info: []u8, out_okm: []u8): bool` | Invokes `hkdfExpand` and returns `bool`. |
| `hkdfDerive` | `(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool` | Invokes `hkdfDerive` and returns `bool`. |

## Minimal Example

```cp
import { hkdfExtract } from "futoin-hkdf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let mut buf_2: [256]u8 = undefined;
    let result = hkdfExtract(out_buf[0..256], buf_1[0..256], buf_2[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
