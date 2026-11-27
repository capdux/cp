# @faker-js/faker — AI Agent Quick Ref

Zero-allocation deterministic fake data generator (xoshiro256** PRNG, person/network/text/commerce/UUIDv4/Luhn card/collections) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Faker,
    splitmixStep,
    rotl7,
    rotl45,
    appendByte,
    appendBytes,
    appendLowerAscii,
    appendCapitalized,
} from "@faker-js/faker";
```

## Core Types & Constants

```cp
class Faker { s0: u64, s1: u64, s2: u64, s3: u64 } // methods: init(...) +28
```

## API Signatures

```cp
fn splitmixStep(state: &mut u64): u64;
fn rotl7(x: u64): u64;
fn rotl45(x: u64): u64;
fn appendByte(out: []u8, pos: &mut usize, b: u8);
fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8);
fn appendLowerAscii(out: []u8, pos: &mut usize, data: &[]u8);
fn appendCapitalized(out: []u8, pos: &mut usize, data: &[]u8);
fn appendU64Dec(out: []u8, pos: &mut usize, val: u64);
fn appendDecPadded(out: []u8, pos: &mut usize, val: u64, width: usize);
fn appendHexByte(out: []u8, pos: &mut usize, b: u8);
fn isLeapYear(year: u64): bool;
fn daysInMonth(year: u64, month: u64): u64;
fn isValidLuhn(digits: &[]u8): bool;
```

## Minimal Usage

```cp
import { isLeapYear } from "@faker-js/faker";

pub fn main() {
    let result = isLeapYear(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@faker-js/faker"`; install via `cap add @faker-js/faker`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
