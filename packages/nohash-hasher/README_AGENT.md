# nohash-hasher — AI Agent Quick Ref

Zero-overhead identity integer hasher, Fibonacci/SplitMix64 key mixers, and fixed 64-slot stack containers (`IntMap64`, `IntSet64`) (Rust `nohash-hasher` v0.2.0 parity).

## Import

```cp
import {
    INT_TABLE_CAP, IntHasher, NoHashHasher, BuildNoHashHasher,
    newIntHasher, newNoHashHasher,
    nohashU8, nohashU16, nohashU32, nohashU64, nohashUsize,
    nohashI8, nohashI16, nohashI32, nohashI64, nohashIsize,
    fibBucketIndex, mixU64, unmixU64, IntMap64, IntSet64,
} from "nohash-hasher";
```

## API Signatures

```cp
export const INT_TABLE_CAP: usize; // 64
export type NoHashHasher = IntHasher;

export class IntHasher {
    state: u64,
    static fn new(): IntHasher;
    fn writeU8(*self, v: u8);   // also writeU16/U32/U64/Usize, writeI8/I16/I32/I64/Isize
    fn finish(): u64;
    fn reset(*self);
    fn clone(): IntHasher;
}
export class BuildNoHashHasher {
    seed: u64,
    static fn new(): BuildNoHashHasher;
    fn buildHasher(): IntHasher;
}
export fn fibBucketIndex(key: u64, bits: u6): usize;
export fn mixU64(x: u64): u64;
export fn unmixU64(x: u64): u64;

export class IntMap64 {
    static fn init(): IntMap64;
    fn len(): usize; fn isEmpty(): bool; fn clear(*self);
    fn insert(*self, key: u64, val: u64): bool;
    fn get(key: u64): u64?; fn containsKey(key: u64): bool; fn remove(*self, key: u64): u64?;
}
export class IntSet64 {
    static fn init(): IntSet64;
    fn len(): usize; fn isEmpty(): bool; fn clear(*self);
    fn insert(*self, key: u64): bool; fn contains(key: u64): bool; fn remove(*self, key: u64): bool;
}
```

## Minimal Example

```cp
import { IntMap64, mixU64, unmixU64 } from "nohash-hasher";

fn demo(): void {
    let mut m = IntMap64.init();
    m.insert(42, 100);
    const v = m.get(42);          // 100
    const back = unmixU64(mixU64(42)); // 42
}
```

## Caveats & Borrowing Rules

- **Mutable receiver**: `IntHasher.write*`/`reset` and `IntMap64`/`IntSet64` mutating methods (`clear`, `insert`, `remove`) require `let mut` instances.
- **Fixed capacity**: `IntMap64` and `IntSet64` hold at most `INT_TABLE_CAP` (`64`) entries; `insert` returns `false` when full.
- **Not cryptographic**: Identity hashing offers no HashDoS protection; use `mixU64` for better bucket distribution or a keyed hash for untrusted inputs.
