// Type definitions for CP

export fn ntrtAbsI64ToU64(x: i64): u64;
export fn checkedAddI64(a: i64, b: i64): i64?;
export fn checkedSubI64(a: i64, b: i64): i64?;
export fn checkedMulI64(a: i64, b: i64): i64?;
export fn checkedAddU64(a: u64, b: u64): u64?;
export fn checkedSubU64(a: u64, b: u64): u64?;
export fn checkedMulU64(a: u64, b: u64): u64?;
export fn saturatingAddU64(a: u64, b: u64): u64;
export fn saturatingSubU64(a: u64, b: u64): u64;
export fn saturatingMulU64(a: u64, b: u64): u64;
export fn saturatingAddI64(a: i64, b: i64): i64;
export fn saturatingSubI64(a: i64, b: i64): i64;
export fn clampI64(v: i64, lo: i64, hi: i64): i64;
export fn clampF64(v: f64, lo: f64, hi: f64): f64;
export fn signumI64(v: i64): i64;
export fn absDiffU64(a: u64, b: u64): u64;
export fn checkedPowU64(base: u64, exp: u32): u64?;
export fn isPowerOfTwoU64(n: u64): bool;
export fn nextPowerOfTwoU64(n: u64): u64?;
export fn countOnesU64(n: u64): u32;
export fn leadingZerosU64(n: u64): u32;
export fn trailingZerosU64(n: u64): u32;
export fn rotateLeftU64(n: u64, k: u32): u64;
export fn rotateRightU64(n: u64, k: u32): u64;
