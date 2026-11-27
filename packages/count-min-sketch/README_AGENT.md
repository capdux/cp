# count-min-sketch — AI Agent Quick Ref

Zero-allocation Count-Min Sketch streaming frequency estimator and heavy-hitter detector (Rust count-min-sketch / Redis CMS parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CountMinSketch,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    rowSeed,
    hashBytesRow,
    hashU64Row,
} from "count-min-sketch";
```

## Core Types & Constants

```cp
class CountMinSketch { counters: [CMS_CELLS]u32, total: u64 } // methods: init(...) +11
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn addWrap64(a: u64, b: u64): u64;
fn mulWrap64(a: u64, b: u64): u64;
fn splitMix64(x_in: u64): u64;
fn rowSeed(row: usize): u64;
fn hashBytesRow(s: string, row: usize): usize;
fn hashU64Row(val: u64, row: usize): usize;
fn satAddU32(a: u32, b: u32): u32;
```

## Minimal Usage

```cp
import { hashBytesRow } from "count-min-sketch";

pub fn main() {
    let result = hashBytesRow("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"count-min-sketch"`; install via `cap add count-min-sketch`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
