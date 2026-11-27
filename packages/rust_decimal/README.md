[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `rust_decimal`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rust_decimal` ([https://github.com/paupino/rust-decimal](https://github.com/paupino/rust-decimal))（原库名 `rust_decimal` → CP 包名 `rust_decimal`）

Zero-heap fixed-point financial decimal arithmetic (`mantissa * 10^(-scale)`, `scale` in `0..12`) in pure CP. Eliminates IEEE-754 floating-point drift (`0.1 + 0.2 == 0.3`).

## Features

- **100% Zero Heap Allocation**: Value-type `Decimal { mantissa: i64, scale: u8 }` with stack-buffer `format(out: []u8)`.
- **Exact Financial Arithmetic**: `add`, `sub`, `mul`, `div` (overflow-safe digit-by-digit long division), `neg`, `abs`, `cmp`, `eq`, `toF64`.
- **Rounding Modes**: `normalize`, `rescale` (half-away-from-zero), and `roundHalfEven` (banker's rounding).
- **Parsing & Formatting**: `Decimal.parse(s)` and `d.format(out)` with full sign and leading/trailing zero support.

## Quick Start

```cp
import { Decimal } from "rust_decimal";

const a = Decimal.parse("0.1").value;
const b = Decimal.parse("0.2").value;
const c = Decimal.parse("0.3").value;
const exact = a.add(b).eq(c); // true!

let buf: [32]u8 = undefined;
const n = Decimal.parse("-123.45").value.format(&buf); // "-123.45"
```

## License

MIT
