[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bigint-u256

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bn.js` ([https://github.com/indutny/bn.js](https://github.com/indutny/bn.js)) / `crates.io:primitive-types` ([https://github.com/paritytech/parity-common](https://github.com/paritytech/parity-common)) / `crates.io:ruint` ([https://github.com/recmo/uint](https://github.com/recmo/uint))（原库名 `bn.js` → CP 包名 `@cap/bigint-u256`）

Zero-heap-allocation 256-bit unsigned integer (`U256`, 4×`u64` little-endian limbs) arithmetic library in pure CP with overflow-safe Debug-mode arithmetic.

## Features

- **Constructors**: `U256.zero()`, `U256.one()`, `U256.fromU64(v)`, `U256.fromU128(lo, hi)`, `U256.maxVal()`, `U256.fromHex(s)` (supports optional `0x`/`0X` prefix)
- **Arithmetic**: `wrappingAdd`, `wrappingSub`, `wrappingMul`, `divRem` (returns `{ quot, rem }`, zero-division safe)
- **Bitwise & Shifts**: `shl`, `shr`, `bitAnd`, `bitOr`, `bitXor`, `leadingZeros`, `bitLen`
- **Comparison & Formatting**: `isZero`, `cmp` (`-1`/`0`/`1`), `eq`, `toHex(out: []u8): usize`

## License

MIT
