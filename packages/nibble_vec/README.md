[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# nibble_vec

Zero-heap 4-bit nibble vector (`NibbleVec`, up to 128 nibbles / 64 bytes inline) with `push`/`pop`, `split`/`join`/`extend`/`slice`, and prefix matching (`commonPrefixLen`, `hasPrefix`) for radix trees and tries. Ported from Rust `nibble_vec`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `nibble_vec` (`michaelsproul/rust_nibble_vec`) |
| **CP Package** | `nibble_vec` (`v0.1.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: nibble_vec)
use nibble_vec::Nibblet;
let mut nv = Nibblet::from_byte_vec(vec![0xAB, 0xCD]);
assert_eq!(nv.get(0), 0xA);
```

```typescript
// CP (same package name: "nibble_vec")
import { NibbleVec } from "nibble_vec";
let raw: [2]u8 = [0xAB, 0xCD];
let nv = NibbleVec.fromBytes(raw[0..2]); // nibbles: [0xA, 0xB, 0xC, 0xD]
```

## Installation

```bash
cap add nibble_vec
```

## Quick Start & Practical Examples

### 1. Construction, Indexing & Mutation

```typescript
import { NibbleVec, nvecMaxNibbles } from "nibble_vec";

fn basicOps(): void {
    let cap = nvecMaxNibbles(); // 128 nibbles (64 bytes)

    // Create empty or from byte slices
    let mut nv = NibbleVec.new();
    let ok1 = nv.push(0xA);
    let ok2 = nv.push(0xB);
    let ok3 = nv.push(0xC);

    let len = nv.len();       // 3
    let bytes = nv.byteLen(); // 2
    let first = nv.get(0);    // 0xA?
    let updated = nv.set(1, 0xF);
    let popped = nv.pop();    // 0xC?

    // Construct directly from bytes (big-endian nibble order: hi nibble first)
    let src: [2]u8 = [0x12, 0x34];
    let mut from_b = NibbleVec.fromBytes(src[0..2]); // alias: fromByteSlice
    let view = from_b.asBytes();

    let mut dst: [8]u8 = undefined;
    let copied = from_b.copyBytes(dst[0..8]); // 2

    nv.truncate(1);
    nv.clear();
}
```

### 2. Radix Tree Operations: `split`, `join`, `extend`, `slice` & Prefix Matching

```typescript
import { NibbleVec } from "nibble_vec";

fn radixOps(): void {
    let b1: [2]u8 = [0xAB, 0xCD];
    let b2: [2]u8 = [0xAB, 0xEF];

    let mut a = NibbleVec.fromBytes(b1[0..2]); // [A, B, C, D]
    let b = NibbleVec.fromBytes(b2[0..2]);     // [A, B, E, F]

    let lcp = a.commonPrefixLen(b);            // 2 ([A, B])
    let prefix = a.slice(0, lcp);              // [A, B]
    let has_pre = b.hasPrefix(prefix);         // true

    // Split `a` in-place at index 2 -> `a` keeps [A, B], returns tail [C, D]
    let tail = a.split(2);

    // Non-mutating join & in-place extend
    let joined = a.join(tail);                 // [A, B, C, D]
    let eq = joined.equals(NibbleVec.fromBytes(b1[0..2])); // true

    let mut extended = a;
    let ok = extended.extend(tail);
}
```

## API Reference

### Constants & Helpers
- `NIBBLE_MAX_BYTES: usize` (`64`), `nvecMaxNibbles(): usize` (`128`), `nvecGetRaw(data: [64]u8, idx: usize): u8`

### `NibbleVec`
- Constructors: `NibbleVec.new()`, `NibbleVec.fromBytes(bytes: &[]u8)`, `NibbleVec.fromByteSlice(bytes: &[]u8)`
- Inspection: `len(): usize`, `isEmpty(): bool`, `byteLen(): usize`, `asBytes(): &[]u8`, `copyBytes(dst: []u8): usize`, `get(idx: usize): u8?`
- Mutation: `push(nib: u8): bool`, `pop(): u8?`, `set(idx: usize, nib: u8): bool`, `clear()`, `truncate(new_len: usize)`, `split(idx: usize): NibbleVec`, `extend(other: NibbleVec): bool`
- Composition & Prefix: `join(other: NibbleVec): NibbleVec`, `slice(start: usize, end: usize): NibbleVec`, `commonPrefixLen(other: NibbleVec): usize`, `hasPrefix(prefix: NibbleVec): bool`, `equals(other: NibbleVec): bool`
