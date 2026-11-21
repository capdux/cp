# `cap:number_format` — Human Number Grouping, Fixed-Point, Ordinal & Radix Formatter

Zero-allocation integer & fixed-point number formatter supporting thousands grouping, English ordinals (`1st`/`2nd`/`3rd`/`11th`), arbitrary radix (`2..=36`), and fixed-point decimal formatting.

## Exports

- `formatGroupedU64(val, sep, out): ([]const u8)?` / `formatGroupedI64(val, sep, out): ([]const u8)?`
- `ordinalSuffix(n): []const u8` / `formatOrdinal(n, out): ([]const u8)?`
- `formatRadixU64(val, radix, uppercase, out): ([]const u8)?`
- `formatFixedPoint(scaled_val, decimals, sep, out): ([]const u8)?`
