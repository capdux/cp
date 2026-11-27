# bitflags — AI Agent Quick Ref

Zero-heap 64-bit bitflags set algebra (contains, intersects, insert, remove, toggle, difference, popcount, trailing/leading zeros) for CP (compatible with Rust bitflags & npm bitfield) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Flags64,
    empty,
    fromBits,
    fromBitIndex,
    isEmpty,
    contains,
    intersects,
    unionFlags,
} from "bitflags";
```

## Core Types & Constants

```cp
class Flags64 { bits: u64 }
```

## API Signatures

```cp
fn empty(): Flags64;
fn fromBits(raw: u64): Flags64;
fn fromBitIndex(idx: u8): Flags64;
fn isEmpty(f: Flags64): bool;
fn contains(f: Flags64, other: Flags64): bool;
fn intersects(f: Flags64, other: Flags64): bool;
fn unionFlags(a: Flags64, b: Flags64): Flags64;
fn intersection(a: Flags64, b: Flags64): Flags64;
fn difference(a: Flags64, b: Flags64): Flags64;
fn symmetricDifference(a: Flags64, b: Flags64): Flags64;
fn insert(f: &mut Flags64, other: Flags64);
fn remove(f: &mut Flags64, other: Flags64);
fn toggle(f: &mut Flags64, other: Flags64);
fn setCond(f: &mut Flags64, other: Flags64, enabled: bool);
fn truncate(raw: u64, valid_mask: u64): Flags64;
fn countBits(f: Flags64): u32;
fn lowestBitIndex(f: Flags64): i32;
fn formatHex(f: Flags64, out: []u8): usize;
```

## Minimal Usage

```cp
import { isEmpty } from "bitflags";

pub fn main() {
    let result = isEmpty(/* f: Flags64 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bitflags"`; install via `cap add bitflags`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
