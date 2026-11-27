# @cap/bit-field

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bit_field` ([https://github.com/phil-opp/rust-bit_field](https://github.com/phil-opp/rust-bit_field))

Zero-heap-allocation bit and bit-range extraction/mutation utilities for `u64`, `u32`, and arbitrary byte slices in pure CP.

## Features

- `getBitU64` / `setBitU64`: read and write individual bits (`0..63`) in `u64`
- `getBitsU64` / `setBitsU64`: extract and write half-open bit ranges `[start..end)` in `u64`
- `getBitU32` / `setBitU32`: read and write individual bits (`0..31`) in `u32`
- `getBitsU32` / `setBitsU32`: extract and write half-open bit ranges `[start..end)` in `u32`
- `getBitSlice` / `setBitSlice`: bounds-checked single-bit read/write over `[]const u8` / `[]u8`

## License

MIT
