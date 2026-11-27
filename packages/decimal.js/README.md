[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# decimal.js

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:decimal.js` ([https://github.com/MikeMcl/decimal.js](https://github.com/MikeMcl/decimal.js))（原库名 `decimal.js` → CP 包名 `decimal.js`）

Zero-allocation 128-bit exact decimal & financial arithmetic engine for CP — clean-room MIT parity with npm `decimal.js` / `bignumber.js` and Rust `rust_decimal`.

## Install

```bash
cap add decimal.js
```

## Features

- **Zero Floating-Point Drift**: `0.1 + 0.2 == 0.3` exact arithmetic backed by signed 128-bit mantissa (`i128`) + scale (`0..28`).
- **6 Rounding Modes**: `HalfEven` (Banker's rounding), `HalfUp`, `HalfDown`, `Floor`, `Ceil`, `Trunc`.
- **Exact String & Scientific Notation Parsing**: handles `"-1234.5678"`, `"1_000_000.25"`, `"1.25e-4"`, `"3e6"`.
- **Zero-Allocation Formatting**: `format(buf)` and `toFixed(2, RoundingMode.HalfEven, buf)` write directly into caller stack buffers.

## Usage

```cp
import { Decimal, RoundingMode } from "decimal.js";

const a = Decimal.parse("0.1")!;
const b = Decimal.parse("0.2")!;
const sum = a.add(b); // exact 0.3

let buf: [64]u8 = undefined;
const text = sum.format(buf[0..64]); // "0.3"
const money = Decimal.parse("19.995")!.toFixed(2, RoundingMode.HalfUp, buf[0..64]); // "20.00"
```
