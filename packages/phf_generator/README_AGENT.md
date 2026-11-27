# phf_generator — AI Agent Quick Ref

Zero-allocation CHD Displace-Hash-Compress perfect hash table builder and O(1) lookup for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PhfHashes,
    PhfBuildResult,
    add64,
    mul64,
    mix64,
    hashKey,
    displace,
    buildPhfWithSeed,
} from "phf_generator";
```

## Core Types & Constants

```cp
class PhfHashes { g: u32, f1: u32, f2: u32 }
class PhfBuildResult { ok: bool, seed: u64, table_len: usize, disp_len: usize }
```

## API Signatures

```cp
fn add64(a: u64, b: u64): u64;
fn mul64(a: u64, b: u64): u64;
fn mix64(z_in: u64): u64;
fn hashKey(key: string, seed: u64): PhfHashes;
fn displace(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize;
fn buildPhfWithSeed(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;
fn buildPhf(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;
fn lookupIndex(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64;
```

## Minimal Usage

```cp
import { hashKey } from "phf_generator";

pub fn main() {
    let result = hashKey("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"phf_generator"`; install via `cap add phf_generator`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
