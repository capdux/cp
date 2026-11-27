# num-integer@1.0.0 (Agent Quickref)

> Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, floor/ceil division, isqrt/icbrt, and binomial coefficients for CP. | License: MIT
> Import: `import { ... } from "num-integer";` | Full types: `src/index.d.cp`

## Functions
- `export fn nintAbsU64(v: i64): u64;`
- `export fn nintDivTruncRem(a: i64, b: i64, q_out: &mut i64, r_out: &mut i64);`
- `export fn gcdU64(a: u64, b: u64): u64;`
- `export fn lcmU64(a: u64, b: u64): u64;`
- `export fn extendedGcd(a: i64, b: i64): ExtGcdResult;`
- `export fn divFloor(a: i64, b: i64): i64;`
- `export fn modFloor(a: i64, b: i64): i64;`
- `export fn divCeil(a: i64, b: i64): i64;`
- `export fn isqrtU64(n: u64): u64;`
- `export fn icbrtU64(n: u64): u64;`
- `export fn binomialU64(n: u64, k: u64): u64;`
- `export fn isEven(n: i64): bool;`
- `export fn isOdd(n: i64): bool;`
- `export fn nextMultipleOfU64(n: u64, step: u64): u64;`
- `export fn prevMultipleOfU64(n: u64, step: u64): u64;`
