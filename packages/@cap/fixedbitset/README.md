# @cap/fixedbitset

Clean-room MIT, zero-heap 1024-bit `FixedBitSet` for CP — modeled on Rust's `fixedbitset` crate.

## API
- `FixedBitSet.withCapacity(bits)` (up to `FBS_MAX_BITS = 1024`)
- `insert`, `put`, `remove`, `toggle`, `set`, `contains`, `insertRange`, `clear`
- `countOnes`, `isClear`, `nextOne(from_bit)`
- Set algebra: `unionWith`, `intersectWith`, `differenceWith`, `symmetricDifferenceWith`, `isSubset`, `isDisjoint`
