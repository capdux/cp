[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ecdsa-sig-formatter

Zero-allocation ASN.1 DER <-> JOSE IEEE P1363 (r || s) ECDSA signature converter for ES256, ES384, and ES512 in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ecdsa-sig-formatter` ([https://github.com/Brightspace/node-ecdsa-sig-formatter](https://github.com/Brightspace/node-ecdsa-sig-formatter), `Apache-2.0`)

| Metadata | Value |
| --- | --- |
| Package | `ecdsa-sig-formatter` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ecdsa-sig-formatter
```

```cp
import {
    EcdsaAlg,
    paramBytes,
    esfParseDerInt,
    esfTrimLeadingZeros,
    derToJose,
    joseToDer,
} from "ecdsa-sig-formatter";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `EcdsaAlg` | `ES256, ES384, ES512` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `paramBytes` | `(alg: EcdsaAlg): usize` | Invokes `paramBytes` and returns `usize`. |
| `esfParseDerInt` | `(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64` | Invokes `esfParseDerInt` and returns `i64`. |
| `esfTrimLeadingZeros` | `(src: &[]u8): usize` | Invokes `esfTrimLeadingZeros` and returns `usize`. |
| `derToJose` | `(der: &[]u8, alg: EcdsaAlg, out: []u8): i64` | Invokes `derToJose` and returns `i64`. |
| `joseToDer` | `(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64` | Invokes `joseToDer` and returns `i64`. |

## Minimal Example

```cp
import { paramBytes } from "ecdsa-sig-formatter";

pub fn main() {
    let result = paramBytes(/* alg: EcdsaAlg */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
