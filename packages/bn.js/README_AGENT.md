# bn.js — AI Agent Quick Ref

Zero-allocation 256-bit unsigned big integer arithmetic (4x u64 limbs) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Add64Res,
    Sub64Res,
    Mul64Res,
    DivRem256,
    U256,
    addWithCarry64,
    subWithBorrow64,
    mulWide64,
} from "bn.js";
```

## Core Types & Constants

```cp
class Add64Res { sum: u64, carry: u64 }
class Sub64Res { diff: u64, borrow: u64 }
class Mul64Res { lo: u64, hi: u64 }
class DivRem256 { quot: U256, rem: U256 }
class U256 { limbs: [4]u64 } // methods: zero(...) +20
```

## API Signatures

```cp
fn addWithCarry64(a: u64, b: u64, carry_in: u64): Add64Res;
fn subWithBorrow64(a: u64, b: u64, borrow_in: u64): Sub64Res;
fn mulWide64(a: u64, b: u64): Mul64Res;
fn clz64(x: u64): u32;
fn zero(): U256;
fn one(): U256;
fn fromU64(v: u64): U256;
fn fromU128(lo: u64, hi: u64): U256;
fn maxVal(): U256;
fn fromHex(s: string): U256;
```

## Minimal Usage

```cp
import { fromHex } from "bn.js";

pub fn main() {
    let result = fromHex("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bn.js"`; install via `cap add bn.js`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
