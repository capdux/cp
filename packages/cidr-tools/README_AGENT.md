# cidr-tools — AI Agent Quick Ref

Zero-allocation IPv4 CIDR parser, formatter, overlap/containment checker, merge, and exclude engine (npm cidr-tools / ip-cidr parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Cidr4,
    Range64,
    invalidCidr,
    pow2_64,
    prefixMask,
    parseIpv4,
    parseCidr4,
    appendU8Dec,
} from "cidr-tools";
```

## Core Types & Constants

```cp
class Cidr4 { network: u32, prefix: u8, valid: bool }
class Range64 { lo: u64, hi: u64 }
```

## API Signatures

```cp
fn invalidCidr(): Cidr4;
fn pow2_64(n: u8): u64;
fn prefixMask(prefix: u8): u32;
fn parseIpv4(s: &[]u8): i64;
fn parseCidr4(s: &[]u8): Cidr4;
fn appendU8Dec(dst: []u8, pos: usize, v_in: u8): usize;
fn formatCidr4(c: Cidr4, dst: []u8): usize;
fn cidrHostCount(c: Cidr4): u64;
fn cidrStart(c: Cidr4): u32;
fn cidrEnd(c: Cidr4): u32;
fn containsIp(c: Cidr4, ip: u32): bool;
fn containsCidr(outer: Cidr4, inner: Cidr4): bool;
fn overlaps(a: Cidr4, b: Cidr4): bool;
fn emitRangeCidrs(lo_in: u64, hi: u64, out: []Cidr4, pos_in: usize): usize;
fn mergeCidrs(inputs: &[]Cidr4, out: []Cidr4): usize;
fn excludeCidr(base: Cidr4, remove: Cidr4, out: []Cidr4): usize;
```

## Minimal Usage

```cp
import { parseIpv4 } from "cidr-tools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIpv4(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cidr-tools"`; install via `cap add cidr-tools`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
