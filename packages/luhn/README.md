[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# luhn

Zero-heap ISO/IEC 7812 Luhn Mod-10, Damm, and Verhoeff check digit algorithms in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:luhn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `luhn` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add luhn
```

```cp
import {
    luhnValidate,
    luhnCheckDigit,
    dammCheckDigit,
    dammValidate,
    verhoeffCheckDigit,
    verhoeffValidate,
} from "luhn";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `luhnValidate` | `(input: &[]u8): bool` | Invokes `luhnValidate` and returns `bool`. |
| `luhnCheckDigit` | `(payload: &[]u8): i32` | Invokes `luhnCheckDigit` and returns `i32`. |
| `dammCheckDigit` | `(payload: &[]u8): i32` | Invokes `dammCheckDigit` and returns `i32`. |
| `dammValidate` | `(input: &[]u8): bool` | Invokes `dammValidate` and returns `bool`. |
| `verhoeffCheckDigit` | `(payload: &[]u8): i32` | Invokes `verhoeffCheckDigit` and returns `i32`. |
| `verhoeffValidate` | `(input: &[]u8): bool` | Invokes `verhoeffValidate` and returns `bool`. |

## Minimal Example

```cp
import { luhnValidate } from "luhn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = luhnValidate(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
