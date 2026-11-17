# cap:simd (SIMD Acceleration)

The `cap:simd` module provides high-performance SIMD primitives for CP, designed for AI/ML workloads and high-throughput data processing. It wraps Zig's native `std.simd` and `@Vector` features while offering a clean, idiomatic CP interface.

## Core API

### `Simd` Struct

- **`suggestVectorLength<T>(): usize`**: Returns the optimal vector length for type `T` on the current CPU, defaulting to 4 or 8 if hardware SIMD is not fully utilized.
- **`iotaI32<N>(): [N]i32`**: Generates a vector/array `[0, 1, ..., N-1]` of `i32`.
- **`reduceAddI32(slice: []const i32): i32`**: SIMD-accelerated reduction sum.
- **`reduceMinI32(slice: []const i32): i32`**: SIMD-accelerated reduction minimum.
- **`reduceMaxI32(slice: []const i32): i32`**: SIMD-accelerated reduction maximum.
- **`dotProductF32(a: []const f32, b: []const f32): f32`**: SIMD-accelerated dot product for `f32` vectors.
- **`prefixSumI32(slice: []i32)`**: SIMD-accelerated prefix sum scan.

## Example

```typescript
import { Simd } from "cap:simd";

let a: [4]f32 = [1.0, 2.0, 3.0, 4.0];
let b: [4]f32 = [2.0, 3.0, 4.0, 5.0];
let dot = Simd.dotProductF32(a[0..], b[0..]); // 40.0
```
