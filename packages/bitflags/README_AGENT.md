# bitflags@1.0.0 (Agent Quickref)

> Zero-heap 64-bit bitflags set algebra (contains, intersects, insert, remove, toggle, difference, popcount, trailing/leading zeros) for CP (compatible with Rust bitflags & npm bitfield) | License: MIT
> Import: `import { ... } from "bitflags";` | Full types: `src/index.d.cp`

## Functions
- `export fn empty(): Flags64;`
- `export fn fromBits(raw: u64): Flags64;`
- `export fn fromBitIndex(idx: u8): Flags64;`
- `export fn isEmpty(f: Flags64): bool;`
- `export fn contains(f: Flags64, other: Flags64): bool;`
- `export fn intersects(f: Flags64, other: Flags64): bool;`
- `export fn unionFlags(a: Flags64, b: Flags64): Flags64;`
- `export fn intersection(a: Flags64, b: Flags64): Flags64;`
- `export fn difference(a: Flags64, b: Flags64): Flags64;`
- `export fn symmetricDifference(a: Flags64, b: Flags64): Flags64;`
- `export fn insert(f: &mut Flags64, other: Flags64);`
- `export fn remove(f: &mut Flags64, other: Flags64);`
- `export fn toggle(f: &mut Flags64, other: Flags64);`
- `export fn setCond(f: &mut Flags64, other: Flags64, enabled: bool);`
- `export fn truncate(raw: u64, valid_mask: u64): Flags64;`
- `export fn countBits(f: Flags64): u32;`
- `export fn lowestBitIndex(f: Flags64): i32;`
- `export fn formatHex(f: Flags64, out: []u8): usize;`
