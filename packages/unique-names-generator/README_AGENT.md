# unique-names-generator — AI Agent Quick Ref

Zero-allocation deterministic human-readable name generator for containers, sandboxes & subdomains (unique-names-generator / human-id / Rust petname parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Petname,
    splitmix64Next,
    adjectiveAt,
    adverbAt,
    nounAt,
    appendStr,
    appendHex4,
    petnameFromSeed,
} from "unique-names-generator";
```

## Core Types & Constants

```cp
class Petname { state: u64 } // methods: init(...) +4
```

## API Signatures

```cp
fn splitmix64Next(state: u64): u64;
fn adjectiveAt(idx: usize): string;
fn adverbAt(idx: usize): string;
fn nounAt(idx: usize): string;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn appendHex4(out: []u8, pos: usize, val: u64): usize;
fn petnameFromSeed(seed: u64, words: u8, sep: string, out: []u8): string;
fn subdomainFromSeed(seed: u64, out: []u8): string;
```

## Minimal Usage

```cp
import { appendStr } from "unique-names-generator";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unique-names-generator"`; install via `cap add unique-names-generator`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
