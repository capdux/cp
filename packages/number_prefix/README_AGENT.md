# number_prefix — AI Agent Quick Ref

Zero-heap SI decimal (k/M/G/T/P/E) and IEC binary (Ki/Mi/Gi/Ti/Pi/Ei) numeric prefix formatter (`v0.4.0`, `MIT`)

## Import

```cp
import {
    PrefixKind,
    PrefixedNumber,
    decimalPrefix,
    binaryPrefix,
    npfWriteStr,
    npfWriteU64,
    formatPrefixed,
} from "number_prefix";
```

## Core Types & Constants

```cp
enum PrefixKind { Standalone, Kilo, Mega, Giga, Tera, Peta, ... }
class PrefixedNumber { value: f64, prefix: PrefixKind, symbol: string, is_standalone: bool }
```

## API Signatures

```cp
fn decimalPrefix(amount: f64): PrefixedNumber;
fn binaryPrefix(amount: f64): PrefixedNumber;
fn npfWriteStr(s: string, out: []u8, pos: usize): usize;
fn npfWriteU64(val: u64, out: []u8, pos: usize): usize;
fn formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatPrefixed } from "number_prefix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrefixed(/* pn: PrefixedNumber */, 0, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"number_prefix"`; install via `cap add number_prefix`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
