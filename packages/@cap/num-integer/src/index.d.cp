// Type definitions for CP

export class ExtendedGcd {
    gcd: i64,
    x: i64,
    y: i64,
}

export fn absU64(v: i64): u64;
export fn divTruncI64(a: i64, b: i64): i64;
export fn remTruncI64(a: i64, b: i64): i64;
export fn divFloor(a: i64, b: i64): i64;
export fn modFloor(a: i64, b: i64): i64;
export fn divCeil(a: i64, b: i64): i64;
export fn divEuclid(a: i64, b: i64): i64;
export fn remEuclid(a: i64, b: i64): i64;
export fn gcdU64(a: u64, b: u64): u64;
export fn lcmU64(a: u64, b: u64): u64;
export fn gcdI64(a: i64, b: i64): i64;
export fn lcmI64(a: i64, b: i64): i64;
export fn extendedGcd(a: i64, b: i64): ExtendedGcd;
export fn modInverse(a: i64, m: i64): i64?;
export fn isqrtU64(n: u64): u64;
export fn icbrtU64(n: u64): u64;
export fn binomialU64(n: u64, k: u64): u64;
export fn isPrimeU64(n: u64): bool;
export fn averageFloor(a: i64, b: i64): i64;
