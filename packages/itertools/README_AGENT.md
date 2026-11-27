# itertools — AI Agent Quick Ref

Pure CP zero-allocation iterator combinators and sequence algorithms (Rust itertools parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MinMaxResult,
    PartitionResult,
    CartesianPair,
    itCopyBytes,
    minmax,
    isSorted,
    isSortedDesc,
    allEqual,
} from "itertools";
```

## Core Types & Constants

```cp
class MinMaxResult { empty: bool, min: i64, max: i64 }
class PartitionResult { true_count: usize, false_count: usize }
class CartesianPair { found: bool, first: i64, second: i64 }
```

## API Signatures

```cp
fn itCopyBytes(dst: []u8, pos: &mut usize, src: &[]u8): bool;
fn minmax(slice: &[]i64): MinMaxResult;
fn isSorted(slice: &[]i64): bool;
fn isSortedDesc(slice: &[]i64): bool;
fn allEqual(slice: &[]i64): bool;
fn uniqueI64(src: &[]i64, out: []i64): usize;
fn dedupConsecutive(src: &[]i64, out: []i64): usize;
fn interleave(a: &[]i64, b: &[]i64, out: []i64): usize;
fn intersperse(src: &[]i64, sep: i64, out: []i64): usize;
fn mergeSorted(a: &[]i64, b: &[]i64, out: []i64): usize;
fn joinStrings(items: &[]string, sep: string, out: []u8): usize;
fn partitionByThreshold(src: &[]i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult;
fn cartesianProductCount(a_len: usize, b_len: usize): usize;
fn nthCartesianPair(a: &[]i64, b: &[]i64, idx: usize): CartesianPair;
fn chunkCount(total_len: usize, chunk_size: usize): usize;
fn windowCount(total_len: usize, win_size: usize): usize;
```

## Minimal Usage

```cp
import { joinStrings } from "itertools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinStrings(/* items: &[]string */, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"itertools"`; install via `cap add itertools`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
