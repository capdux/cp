# `@cap/enumflags2`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:enumflags2` ([https://github.com/meithecatte/enumflags2](https://github.com/meithecatte/enumflags2))

Zero-allocation 64-bit type-safe enum bitflags set with `valid_mask` protection for CP (clean-room MIT, inspired by `enumflags2` v0.7).

## Features

- **Zero Heap Allocations**: Compact 16-byte value type (`bits: u64`, `valid_mask: u64`).
- **Invalid-Bit Protection**: Every constructor, mutator, and set operation enforces `valid_mask`, preventing out-of-range bit injection.
- **Strict & Truncating Construction**: `fromBitsStrict` rejects unknown bits (`null`), while `fromBitsTruncate` silently masks them off.
- **Full Set Algebra**: `unionFlags`, `intersectFlags`, `differenceFlags`, `symmetricDifferenceFlags`, and `complement`.
- **O(1) Bit Cursor Iteration**: `nextFlag(start_bit)` finds the next set bit index `>= start_bit` in `0..=63`.

## API

```cp
pub struct BitFlags64 {
    pub bits: u64,
    pub valid_mask: u64,

    pub static fn empty(valid_mask: u64): BitFlags64
    pub static fn all(valid_mask: u64): BitFlags64
    pub static fn fromBitsTruncate(bits: u64, valid_mask: u64): BitFlags64
    pub static fn fromBitsStrict(bits: u64, valid_mask: u64): BitFlags64?

    pub fn raw(): u64
    pub fn len(): usize
    pub fn isEmpty(): bool
    pub fn isAll(): bool
    pub fn contains(flag: u64): bool
    pub fn intersects(flag: u64): bool

    pub fn insert(*_, flag: u64)
    pub fn remove(*_, flag: u64)
    pub fn toggle(*_, flag: u64)
    pub fn set(*_, flag: u64, enabled: bool)
    pub fn clear(*_)

    pub fn unionFlags(other: BitFlags64): BitFlags64
    pub fn intersectFlags(other: BitFlags64): BitFlags64
    pub fn differenceFlags(other: BitFlags64): BitFlags64
    pub fn symmetricDifferenceFlags(other: BitFlags64): BitFlags64
    pub fn complement(): BitFlags64

    pub fn nextFlag(start_bit: u8): u8?
}
```

## Usage

```cp
import { BitFlags64 } from "@cap/enumflags2";

const READ: u64 = 1 << 0;
const WRITE: u64 = 1 << 1;
const EXEC: u64 = 1 << 2;
const ALL_PERMS: u64 = READ | WRITE | EXEC;

fn example() {
    let perms = BitFlags64.empty(ALL_PERMS);
    perms.insert(READ | WRITE);
    const has_read = perms.contains(READ); // true
    const inv = perms.complement();        // EXEC only

    let cur: u8 = 0;
    while (bit := perms.nextFlag(cur)) {
        cur = bit + 1;
    }
}
```

## License

MIT
