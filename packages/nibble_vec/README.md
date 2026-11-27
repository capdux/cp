[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nibble_vec

Zero-heap 4-bit nibble vector (`NibbleVec`, up to 128 nibbles / 64 bytes inline) for radix trees, Patricia tries, and prefix compression in pure CP.

- **Package**: `nibble_vec` (`v0.1.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/nibble_vec` (`v0.1.0`, `MIT`, `rust_nibble_vec`), redesigned with fixed inline stack storage for zero heap allocation.

## Installation & Import

```bash
cap add nibble_vec
```

```typescript
import { NibbleVec, nvecMaxNibbles, nvecGetRaw } from "nibble_vec";
```

## Core API

### Module Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nvecMaxNibbles` | `(): usize` | Returns the inline nibble capacity (`128`, backed by `[64]u8`). |
| `nvecGetRaw` | `(data: [64]u8, idx: usize): u8` | Reads nibble `idx` (`0..=15`) directly from a raw `[64]u8` array. |

### `NibbleVec` Class

Fields: `data: [64]u8`, `nibble_len: usize`.

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `(): NibbleVec` | Creates an empty `NibbleVec`. |
| `fromBytes` / `fromByteSlice` | `(bytes: &[]u8): NibbleVec` | Packs bytes into nibbles (high nibble first, clamped to 64 bytes / 128 nibbles). |
| `len` / `isEmpty` / `byteLen` | `(): usize` / `bool` / `usize` | Nibble count, empty check, and ceil byte length (`(len + 1) / 2`). |
| `asBytes` | `(): &[]u8` | Borrows the packed underlying byte slice (`data[0..byteLen()]`). |
| `copyBytes` | `(dst: []u8): usize` | Copies packed bytes into `dst`, returning bytes copied. |
| `push` | `(nib: u8): bool` | Appends a 4-bit nibble (`nib & 0x0F`); returns `false` if full (`128`). |
| `pop` | `(): u8?` | Removes and returns the last nibble, or `null` if empty. |
| `get` | `(idx: usize): u8?` | Returns nibble at `idx`, or `null` if out of bounds. |
| `set` | `(idx: usize, nib: u8): bool` | Updates nibble at `idx`; returns `false` if out of bounds. |
| `clear` / `truncate` | `()` / `(new_len: usize)` | Clears all nibbles or truncates to `new_len`. |
| `split` | `(idx: usize): NibbleVec` | Splits at `idx`: keeps `[0..idx)` in `self` and returns `[idx..len)`. |
| `join` | `(other: NibbleVec): NibbleVec` | Returns a new `NibbleVec` concatenating `self` and `other`. |
| `extend` | `(other: NibbleVec): bool` | Appends `other` in place; returns `false` if combined length exceeds `128`. |
| `slice` | `(start: usize, end: usize): NibbleVec` | Returns a sub-vector covering `[start..end)`. |
| `commonPrefixLen` | `(other: NibbleVec): usize` | Computes length of the longest shared nibble prefix. |
| `hasPrefix` / `equals` | `(other: NibbleVec): bool` | Prefix check and exact nibble-by-nibble equality. |

## Examples

### 1. Build from Bytes & Inspect Nibbles

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let raw: [2]u8 = [0xAB, 0xCD];
    let nv = NibbleVec.fromBytes(raw[0..]); // nibbles: [0xA, 0xB, 0xC, 0xD]
    let n = nv.len();                       // 4
    if (let first = nv.get(0)) {
        // first == 0xA
    }
}
```

### 2. Prefix Matching & Radix Tree Split

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let a_bytes: [2]u8 = [0xAB, 0x12];
    let b_bytes: [2]u8 = [0xAB, 0x99];
    let a = NibbleVec.fromBytes(a_bytes[0..]);
    let b = NibbleVec.fromBytes(b_bytes[0..]);

    let lcp = a.commonPrefixLen(b); // 2 (0xA, 0xB)
    let tail = a.split(lcp);        // a = [0xA, 0xB], tail = [0x1, 0x2]
    let merged = a.join(tail);      // [0xA, 0xB, 0x1, 0x2]
}
```

### 3. Push, Pop & Slice

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let nv = NibbleVec.new();
    nv.push(0xF);
    nv.push(0x0);
    nv.push(0x7);
    let mid = nv.slice(1, 3); // [0x0, 0x7]
    let last = nv.pop();      // 0x7
}
```

## Differences from Upstream & Highlights

- **Fixed Inline Capacity (Zero Heap)**: Uses an inline `[64]u8` buffer (`128` nibbles max) instead of Rust `SmallVec`, guaranteeing deterministic stack memory and zero allocator overhead.
- **Safe Bounds with Optionals**: `get(idx)` and `pop()` return `u8?` instead of panicking on out-of-bounds or empty vectors; `push`/`set`/`extend` return `bool`.
- **High-Nibble-First Packing**: Matches `rust_nibble_vec` bit layout (`even` index = high 4 bits, `odd` index = low 4 bits) for byte-order-preserving radix tree keys.
