[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# chacha20

Zero-allocation RFC 8439 ChaCha20 stream cipher (Rust chacha20 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:chacha20` ([https://github.com/RustCrypto/stream-ciphers](https://github.com/RustCrypto/stream-ciphers))（原库名 `chacha20` → CP 包名 `chacha20`）

| Metadata | Value |
| --- | --- |
| Package | `chacha20` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add chacha20
```

```cp
import {
    ChaCha20Cipher,
    cc20Add32,
    cc20Rotl16,
    cc20Rotl12,
    cc20Rotl8,
    cc20Rotl7,
    cc20ReadLe32,
    cc20WriteLe32,
} from "chacha20";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ChaCha20Cipher` | `class` | Fields: `key: [32]u8, nonce: [12]u8, counter: u32, block_buf: [64]u8, block_pos: usize` · Methods: `new, newWithCounter, seek, applyKeystream` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cc20Add32` | `(a: u32, b: u32): u32` | Invokes `cc20Add32` and returns `u32`. |
| `cc20Rotl16` | `(x: u32): u32` | Invokes `cc20Rotl16` and returns `u32`. |
| `cc20Rotl12` | `(x: u32): u32` | Invokes `cc20Rotl12` and returns `u32`. |
| `cc20Rotl8` | `(x: u32): u32` | Invokes `cc20Rotl8` and returns `u32`. |
| `cc20Rotl7` | `(x: u32): u32` | Invokes `cc20Rotl7` and returns `u32`. |
| `cc20ReadLe32` | `(b: []u8, off: usize): u32` | Invokes `cc20ReadLe32` and returns `u32`. |
| `cc20WriteLe32` | `(out: []u8, off: usize, v: u32)` | Executes `cc20WriteLe32` in-place. |
| `cc20QuarterRound` | `(st: []u32, ai: usize, bi: usize, ci: usize, di: usize)` | Executes `cc20QuarterRound` in-place. |
| `chacha20Block` | `(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool` | Invokes `chacha20Block` and returns `bool`. |

## Minimal Example

```cp
import { cc20Add32 } from "chacha20";

pub fn main() {
    let result = cc20Add32(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
