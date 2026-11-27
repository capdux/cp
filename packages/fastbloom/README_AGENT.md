# fastbloom — AI Agent Quick Ref

Zero-allocation Kirsch-Mitzenmacher double-hashing Bloom filters BloomFilter1024/BloomFilter4096 (Rust fastbloom / bloomfilter parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HashPair,
    BloomFilter1024,
    BloomFilter4096,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    hashBytesPair,
} from "fastbloom";
```

## Core Types & Constants

```cp
class HashPair { h1: u64, h2: u64 }
class BloomFilter1024 { words: [16]u64, num_hashes: u8, items: usize } // methods: init(...) +10
class BloomFilter4096 { words: [64]u64, num_hashes: u8, items: usize } // methods: init(...) +10
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn addWrap64(a: u64, b: u64): u64;
fn mulWrap64(a: u64, b: u64): u64;
fn splitMix64(x_in: u64): u64;
fn hashBytesPair(s: string): HashPair;
fn hashU64Pair(val: u64): HashPair;
fn bitMask64(bit: usize): u64;
fn popcount64(x_in: u64): usize;
fn clampHashes(k: u8): u8;
fn expNeg(x_in: f64): f64;
fn computeFpr(k: u8, n: usize, m: usize): f64;
```

## Minimal Usage

```cp
import { hashBytesPair } from "fastbloom";

pub fn main() {
    let result = hashBytesPair("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fastbloom"`; install via `cap add fastbloom`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
