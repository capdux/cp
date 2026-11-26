# @cap/ryu-float

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ryu` ([https://github.com/dtolnay/ryu](https://github.com/dtolnay/ryu))（原库名 `ryu` → CP 包名 `@cap/ryu-float`）

Pure CP, zero-allocation fast floating-point-to-string formatter (clean-room MIT parity with Rust `ryu` v1).

## Dependencies

- `@cap/itoa` — Reuses `formatU64` for all decimal mantissa and exponent rendering with zero duplication.

## Features

- **Shortest Exact Round-Trip Formatting (`formatF64`, `formatF32`)**: Emits shortest decimal or scientific representation (`0.0`, `-0.0`, `1.0`, `3.141592653589793`, `1e20`, `1e-7`), strips redundant trailing zeros while preserving `.0` for integers (Rust `ryu` convention), and handles `NaN`, `inf`, `-inf`.
- **Finite-Only Mode (`formatF64Finite`)**: Returns `0` for `NaN` / `inf` / `-inf` (ideal for strict JSON serializers).
- **Exact Decimal Rounding (`formatFixedF64`, `formatExpF64`)**: Supports `0..12` decimal places with exact carry handling (e.g. `9.995` -> `"10.00"`) and scientific notation (`"1.23e4"`, `"1.5e-3"`).
- **Zero Heap Allocation & Overflow-Safe**: Writes directly into caller-provided `[]u8` slices and returns `0` if `out` is too small.

## API

- `formatF64(val: f64, out: []u8): usize`
- `formatF64Finite(val: f64, out: []u8): usize`
- `formatF32(val: f32, out: []u8): usize`
- `formatFixedF64(val: f64, decimals: u8, out: []u8): usize`
- `formatExpF64(val: f64, precision: u8, out: []u8): usize`
- `isFiniteF64(val: f64): bool`
- `isNanF64(val: f64): bool`
- `isInfF64(val: f64): bool`

## License

MIT
