# @cap/h3o

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h3o` ([https://github.com/HydroniumLabs/h3o](https://github.com/HydroniumLabs/h3o))

Clean-room MIT, zero-heap Uber H3 64-bit hexagonal hierarchical spatial index library for CP — modeled on crates.io `h3o` v0.6.

## Features

- **Zero heap allocation**: 8-byte `H3Cell { bits: u64 }` value type implementing the official 64-bit H3 Cell Index bit layout.
- **Strict Validation**: validates mode (`1`), reserved bits (`0`), resolution (`0..15`), base cell (`0..121`), active/unused 3-bit digits (`0..6` / `7`), and pentagon deleted K-axis constraints.
- **Hierarchy & Pentagon Queries**: `resolution`, `baseCell`, `digitAt`, `isPentagon`, `parent`, and `childAt`.
- **Hex Codec & Area Table**: `toHex`, `parseH3Hex` (with optional `0x` prefix), and `avgHexAreaKm2` for resolutions `0..15`.

## API

### Types
- `H3Cell { bits: u64 }`
  - `H3Cell.fromParts(res: u8, base_cell: u8, digits: [15]u8): H3Cell?`
  - `H3Cell.fromU64(raw: u64): H3Cell?`
  - `resolution(): u8`, `baseCell(): u8`, `digitAt(res: u8): u8`
  - `isPentagon(): bool`
  - `parent(target_res: u8): H3Cell?`
  - `childAt(digit: u8): H3Cell?`
  - `toHex(out: []u8): usize`

### Functions
- `parseH3Hex(hex: string): H3Cell?`
- `avgHexAreaKm2(res: u8): f64`

## License

MIT
