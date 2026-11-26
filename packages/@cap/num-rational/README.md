# `@cap/num-rational`

Zero-allocation exact 64-bit rational number (`Ratio64`) arithmetic library for CP (clean-room MIT, inspired by Rust `num-rational` v0.4).

## Dependencies (Zero Duplication)

- `@cap/num-integer`: reuses `gcdU64`, `divFloor`, and `divCeil` for canonical reduction and rounding.

## Invariants

Every `Ratio64` produced by the public API is guaranteed to be in canonical reduced form:
- `denom > 0`
- `gcd(|numer|, denom) == 1`
- Zero is uniquely represented as `0 / 1`

## API

```cp
pub struct Ratio64 {
    pub numer: i64,
    pub denom: i64,

    pub static fn new(numer: i64, denom: i64): Ratio64?
    pub static fn fromInt(n: i64): Ratio64
    pub static fn zero(): Ratio64
    pub static fn one(): Ratio64

    pub fn isInteger(): bool
    pub fn isZero(): bool
    pub fn isPositive(): bool
    pub fn isNegative(): bool

    pub fn add(other: Ratio64): Ratio64
    pub fn sub(other: Ratio64): Ratio64
    pub fn mul(other: Ratio64): Ratio64
    pub fn div(other: Ratio64): Ratio64?
    pub fn recip(): Ratio64?
    pub fn neg(): Ratio64
    pub fn abs(): Ratio64

    pub fn cmp(other: Ratio64): i32
    pub fn eq(other: Ratio64): bool
    pub fn floor(): i64
    pub fn ceil(): i64
    pub fn toF64(): f64
}
```

## License

MIT
