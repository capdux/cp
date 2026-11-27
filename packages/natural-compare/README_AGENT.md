# natural-compare — AI Agent Quick Ref

Zero-allocation natural / alphanumeric string comparison and in-place sorting (natural-compare / natsort / natord parity, arbitrary-length integer segments without overflow) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    compareRawBytes,
    naturalCompareImpl,
    naturalCompare,
    naturalCompareIgnoreCase,
    naturalLess,
    naturalLessIgnoreCase,
    naturalEq,
    comesBefore,
} from "natural-compare";
```

## API Signatures

```cp
fn compareRawBytes(a: string, b: string): i8;
fn naturalCompareImpl(a: string, b: string, ignore_case: bool): i8;
fn naturalCompare(a: string, b: string): i8;
fn naturalCompareIgnoreCase(a: string, b: string): i8;
fn naturalLess(a: string, b: string): bool;
fn naturalLessIgnoreCase(a: string, b: string): bool;
fn naturalEq(a: string, b: string): bool;
fn comesBefore(a: string, b: string, desc: bool, ignore_case: bool): bool;
fn siftDown(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool);
fn sortInPlace(items: []string, desc: bool, ignore_case: bool);
fn sortNatural(items: []string);
fn sortNaturalDesc(items: []string);
fn sortNaturalIgnoreCase(items: []string);
fn isSortedNatural(items: []string): bool;
```

## Minimal Usage

```cp
import { isSortedNatural } from "natural-compare";

pub fn main() {
    let result = isSortedNatural(/* items: []string */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"natural-compare"`; install via `cap add natural-compare`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
