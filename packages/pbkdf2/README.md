[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pbkdf2

Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`pbkdf2`](https://github.com/RustCrypto/password-hashes)（`MIT OR Apache-2.0`）（原库名 `pbkdf2`，CP 包名 `pbkdf2`）。

| Metadata | Value |
| --- | --- |
| Package | `pbkdf2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pbkdf2
```

```cp
import {
    Pbkdf2Pads,
    pbkdf2InitPads,
    pbkdf2HmacSaltBlock,
    pbkdf2Hmac32,
    pbkdf2HmacSha256,
    pbkdf2Verify,
    pbkdf2HmacSha256Str,
} from "pbkdf2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pbkdf2Pads` | `class` | Fields: `inner_base: Sha256State, outer_base: Sha256State` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pbkdf2InitPads` | `(password: &[]u8): Pbkdf2Pads` | Invokes `pbkdf2InitPads` and returns `Pbkdf2Pads`. |
| `pbkdf2HmacSaltBlock` | `(pads: Pbkdf2Pads, salt: &[]u8, block_idx: u32, out32: []u8)` | Executes `pbkdf2HmacSaltBlock` in-place. |
| `pbkdf2Hmac32` | `(pads: Pbkdf2Pads, msg32: []u8, out32: []u8)` | Executes `pbkdf2Hmac32` in-place. |
| `pbkdf2HmacSha256` | `(password: &[]u8, salt: &[]u8, iterations: u32, out_dk: []u8): bool` | Invokes `pbkdf2HmacSha256` and returns `bool`. |
| `pbkdf2Verify` | `(password: &[]u8, salt: &[]u8, iterations: u32, expected_dk: &[]u8): bool` | Invokes `pbkdf2Verify` and returns `bool`. |
| `pbkdf2HmacSha256Str` | `(password: string, salt: string, iterations: u32, out_dk: []u8): bool` | Invokes `pbkdf2HmacSha256Str` and returns `bool`. |

## Minimal Example

```cp
import { pbkdf2HmacSha256Str } from "pbkdf2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbkdf2HmacSha256Str("example", "example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
