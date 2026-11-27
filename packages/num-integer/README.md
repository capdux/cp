[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `num-integer`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-integer` ([https://github.com/rust-num/num-integer](https://github.com/rust-num/num-integer))

Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, mathematical floor/ceil division, exact integer roots, and binomial coefficients for CP (clean-room MIT, inspired by Rust `num-integer` v0.1).

## API

```cp
pub struct ExtGcdResult {
    pub gcd: i64,
    pub x: i64,
    pub y: i64,
}

pub fn gcdU64(a: u64, b: u64): u64
pub fn lcmU64(a: u64, b: u64): u64
pub fn extendedGcd(a: i64, b: i64): ExtGcdResult
pub fn divFloor(a: i64, b: i64): i64
pub fn modFloor(a: i64, b: i64): i64
pub fn divCeil(a: i64, b: i64): i64
pub fn isqrtU64(n: u64): u64
pub fn icbrtU64(n: u64): u64
pub fn binomialU64(n: u64, k: u64): u64
pub fn isEven(n: i64): bool
pub fn isOdd(n: i64): bool
pub fn nextMultipleOfU64(n: u64, step: u64): u64
pub fn prevMultipleOfU64(n: u64, step: u64): u64
```

## Highlights

- **Zero Heap Allocations**: All functions operate strictly on primitive integer registers.
- **Overflow-Safe**:
  - `lcmU64` divides by `gcd` before multiplying.
  - `binomialU64` cancels common factors via `gcdU64` at every step (computes `C(60, 30)` accurately in `u64`).
  - `isqrtU64` (bit-by-bit restoring) and `icbrtU64` (bounded binary search up to `2642245`) are 100% exact across the entire `0..=u64::MAX` range.
  - Signed division (`divFloor`, `modFloor`, `divCeil`, `extendedGcd`) uses `u64` magnitude arithmetic with exact sign restoration.

## License

MIT
