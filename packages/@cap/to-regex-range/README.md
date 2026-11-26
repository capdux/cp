# @cap/to-regex-range

Zero-heap-allocation numeric range to compact regex pattern compiler and decimal range matcher in pure CP (parity with npm `to-regex-range`).

## Features

- `toRegexRange(min_val: i64, max_val: i64, out: []u8): usize`: compiles any integer range `[min_val, max_val]` (including single values, multi-digit spans, negative/mixed ranges, and reversed bounds) into a compact regex pattern without heap allocation.
- `matchNumericRange(s: string, min_val: i64, max_val: i64): bool`: zero-allocation exact decimal integer range validator with overflow protection.
- `countDigits(n: u64): usize`: counts base-10 digits of a `u64`.

## License

MIT
