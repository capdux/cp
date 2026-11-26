# @cap/bigdecimal

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bigdecimal` ([https://github.com/akubera/bigdecimal-rs](https://github.com/akubera/bigdecimal-rs))

Clean-room MIT, zero-heap exact base-10 financial arithmetic library for CP — modeled on crates.io `bigdecimal` v0.4.

## Features

- **Zero heap allocation**: compact stack struct `BigDecimal { digits: i64, scale: i32 }` representing `digits * 10^(-scale)` with up to 18 significant decimal digits and zero binary floating-point error (`0.1 + 0.2 == 0.3`).
- **Parsing & Formatting**: `parseDecimal` handles optional signs, decimals, scientific notation (`e`/`E`), and surrounding whitespace while rejecting invalid input; `formatDecimal` writes exact base-10 text into a caller-provided `[]u8` buffer.
- **Exact Arithmetic & Financial Rounding**: `add`, `sub`, `mul`, `divWithScale` (safe long-division returning `null` on division by zero), `cmp`, `eq`, `neg`, `abs`, `normalize`, and half-away-from-zero `round`.

## API

### Types
- `BigDecimal { digits: i64, scale: i32 }`
  - `BigDecimal.zero(): BigDecimal`
  - `BigDecimal.fromInt(v: i64): BigDecimal`
  - `BigDecimal.new(digits: i64, scale: i32): BigDecimal`
  - `normalize(): BigDecimal`
  - `neg(): BigDecimal`, `abs(): BigDecimal`
  - `add(other: BigDecimal): BigDecimal`
  - `sub(other: BigDecimal): BigDecimal`
  - `mul(other: BigDecimal): BigDecimal`
  - `divWithScale(other: BigDecimal, target_scale: i32): BigDecimal?`
  - `round(new_scale: i32): BigDecimal`
  - `cmp(other: BigDecimal): i32`, `eq(other: BigDecimal): bool`

### Functions
- `parseDecimal(s: string): BigDecimal?`
- `formatDecimal(val: BigDecimal, out: []u8): usize`

## License

MIT
