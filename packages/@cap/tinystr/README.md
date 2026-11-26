# `@cap/tinystr`

Zero-allocation stack-inline fixed-capacity ASCII strings in pure CP (ICU4X `tinystr` v0.7/v0.8 parity). Strictly enforces non-NUL 7-bit ASCII (`0x01..0x7F`) for O(1) BCP-47 locale tags, currency codes, script subtags, and protocol identifiers.

## Types

- `TinyAsciiStr4` — Up to 4 ASCII bytes, packable to/from `u32` (`toPackedU32` / `fromPackedU32`)
- `TinyAsciiStr8` — Up to 8 ASCII bytes, packable to/from `u64` (`toPackedU64` / `fromPackedU64`)
- `TinyAsciiStr16` — Up to 16 ASCII bytes

## Core Methods (O(1), zero heap allocation)

- Construction: `fromStr(s: string)`, `fromBytes(src: []const u8)`, `empty()`, `fromPackedU32(v: u32)`, `fromPackedU64(v: u64)`
- Accessors: `len()`, `isEmpty()`, `asBytes(out: []u8)`, `byteAt(idx: usize)`, `toPackedU32()`, `toPackedU64()`
- Comparison: `eq(other)`, `cmp(other): i32`, `eqIgnoreAsciiCase(other): bool`
- Case Transforms: `toAsciiLowercase()`, `toAsciiUppercase()`, `toAsciiTitlecase()`
- Predicates: `isAsciiAlphabetic()`, `isAsciiAlphanumeric()`, `isAsciiNumeric()`, `isAsciiLowercase()`, `isAsciiUppercase()`

## Error Codes

- `TINYSTR_OK` (`0`)
- `TINYSTR_ERR_TOO_LONG` (`-1`)
- `TINYSTR_ERR_NON_ASCII` (`-2`)
- `TINYSTR_ERR_CONTAINS_NUL` (`-3`)
