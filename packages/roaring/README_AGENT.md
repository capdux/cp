# roaring — AI Agent Quick Ref

Zero-allocation Roaring Bitmap with 16-bit high-key containers, binary search, rank, and set algebra (Rust roaring parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BinSearch,
    RoaringContainer,
    RoaringBitmap,
    highKey,
    lowVal,
    combineKeyVal,
    emptyContainer,
    searchU16,
} from "roaring";
```

## Core Types & Constants

```cp
class BinSearch { found: bool, pos: usize }
class RoaringContainer { key: u16, values: [MAX_CONTAINER_ITEMS]u16, count: usize }
class RoaringBitmap { containers: [MAX_CONTAINERS]RoaringContainer, container_count: usize, total_len: u64 } // methods: init(...) +12
```

## API Signatures

```cp
fn highKey(v: u32): u16;
fn lowVal(v: u32): u16;
fn combineKeyVal(hi: u16, lo: u16): u32;
fn emptyContainer(): RoaringContainer;
fn searchU16(arr: [MAX_CONTAINER_ITEMS]u16, count: usize, target: u16): BinSearch;
fn searchContainer(containers: [MAX_CONTAINERS]RoaringContainer, count: usize, key: u16): BinSearch;
fn unionContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
fn intersectContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
fn diffContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
```

## Minimal Usage

```cp
import { highKey } from "roaring";

pub fn main() {
    let result = highKey(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"roaring"`; install via `cap add roaring`.
