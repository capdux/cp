# humansize — AI Agent Quick Ref

Zero-heap human-readable byte size formatter (Decimal, Binary, Windows) in pure CP (modeled after Rust humansize v2) (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    BaseUnit,
    FormatSizeOptions,
    hszWriteSlice,
    hszWriteU64,
    hszWriteFixedFrac,
    hszPow10,
    hszUnitStr,
    formatSize,
} from "humansize";
```

## Core Types & Constants

```cp
enum BaseUnit { Decimal, Binary, Windows }
class FormatSizeOptions { base: BaseUnit, decimal_places: u8, space_after_value: bool, long_units: bool }
```

## API Signatures

```cp
fn hszWriteSlice(out: []u8, pos: usize, s: string): usize;
fn hszWriteU64(out: []u8, pos: usize, val: u64): usize;
fn hszWriteFixedFrac(out: []u8, pos: usize, frac: u64, places: u8): usize;
fn hszPow10(places: u8): u64;
fn hszUnitStr(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string;
fn formatSize(bytes: u64, opts: FormatSizeOptions, out: []u8): usize;
fn formatDecimal(bytes: u64, out: []u8): usize;
fn formatBinary(bytes: u64, out: []u8): usize;
```

## Minimal Usage

```cp
import { hszWriteSlice } from "humansize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hszWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"humansize"`; install via `cap add humansize`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
