# bloom-filters — AI Agent Quick Ref

Zero-heap classic Bloom filter in pure CP with Kirsch-Mitzenmacher double hashing (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BloomFilter,
    fnv1a64,
    djb2Mix64,
    popcount,
    fillRatio,
    optimalNumHashes,
} from "bloom-filters";
```

## Core Types & Constants

```cp
class BloomFilter { num_hashes: u32, bit_count: usize, inserted: u64 } // methods: new(...) +6
```

## API Signatures

```cp
fn fnv1a64(data: &[]u8): u64;
fn djb2Mix64(data: &[]u8): u64;
fn popcount(bits: &[]u8): u64;
fn fillRatio(bits: &[]u8): f64;
fn optimalNumHashes(bit_count: usize, expected_items: usize): u32;
```

## Minimal Usage

```cp
import { fnv1a64 } from "bloom-filters";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = fnv1a64(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bloom-filters"`; install via `cap add bloom-filters`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
