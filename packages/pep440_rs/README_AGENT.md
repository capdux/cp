# pep440_rs — AI Agent Quick Ref

Zero-heap PEP 440 Python version parser, total-order comparator, and specifier matcher in pure CP (modeled after Astral pep440_rs) (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    PreKind,
    Pep440Version,
    PepNumRes,
    pepInvalid,
    pepIsDigit,
    pepIsSpace,
    pepToLower,
    pepIsSep,
} from "pep440_rs";
```

## Core Types & Constants

```cp
enum PreKind { None, Alpha, Beta, Rc }
class Pep440Version { valid: bool, epoch: u32, release: [4]u32, release_len: u8, /* +6 fields */ }
class PepNumRes { ok: bool, val: u32, next: usize }
```

## API Signatures

```cp
fn pepInvalid(): Pep440Version;
fn pepIsDigit(c: u8): bool;
fn pepIsSpace(c: u8): bool;
fn pepToLower(c: u8): u8;
fn pepIsSep(c: u8): bool;
fn pepMatchWord(s: string, pos: usize, end: usize, word: string): bool;
fn pepReadU32(s: string, pos: usize, end: usize): PepNumRes;
fn parsePep440(s: string): Pep440Version;
fn pepCmpU32(a: u32, b: u32): i32;
fn pepPreRank(k: PreKind): i32;
fn pepStageRank(v: Pep440Version): i32;
fn comparePep440(a: Pep440Version, b: Pep440Version): i32;
fn pepPrefixMatch(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool;
fn pepMatchSingle(v: Pep440Version, raw: string): bool;
fn matchesSpecifier(ver: string, spec: string): bool;
```

## Minimal Usage

```cp
import { parsePep440 } from "pep440_rs";

pub fn main() {
    let result = parsePep440("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pep440_rs"`; install via `cap add pep440_rs`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
