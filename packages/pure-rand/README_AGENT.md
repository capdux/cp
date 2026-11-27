# pure-rand — AI Agent Quick Ref

Zero-allocation pure functional immutable PRNG (Xoroshiro128Plus, SplitMix64 seeding, jump, and unbiased uniformIntDistribution) (clean-room MIT implementation of npm pure-rand, #443 / 124M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Xoroshiro128Plus,
    NextU64Result,
    NextI32Result,
    NextIntRangeResult,
    prRotl64,
    prSplitMix64Step,
    uniformIntDistribution,
} from "pure-rand";
```

## Core Types & Constants

```cp
class Xoroshiro128Plus { s0: u64, s1: u64 } // methods: fromSeed(...) +4
class NextU64Result { value: u64, next: Xoroshiro128Plus }
class NextI32Result { value: i32, next: Xoroshiro128Plus }
class NextIntRangeResult { value: i64, next: Xoroshiro128Plus }
```

## API Signatures

```cp
fn prRotl64(x: u64, k: u6): u64;
fn prSplitMix64Step(state: u64): u64;
fn uniformIntDistribution(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult;
```

## Minimal Usage

```cp
import { prRotl64 } from "pure-rand";

pub fn main() {
    let result = prRotl64(0, /* k: u6 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pure-rand"`; install via `cap add pure-rand`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
