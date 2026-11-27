[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# governor

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `npm:express-rate-limit` ([https://github.com/express-rate-limit/express-rate-limit](https://github.com/express-rate-limit/express-rate-limit))（原库名 `governor` → CP 包名 `governor`）

Zero-allocation IETF HTTP RateLimit headers (`RateLimit-Policy`, `RateLimit`, `X-RateLimit-*`) parser, formatter, and reset delay calculator for CP.

## Features

- **Zero heap allocation**: parses and formats directly over `string` / `[]u8` buffers.
- **Policy parsing & formatting**: `parsePolicy` supports `100;w=60`, `100;w=60;burst=20`, and quoted parameters (`comment="sliding"`); `formatPolicy` serializes `"<quota>;w=<window>"`.
- **Structured state**: `parseStructuredRateLimit`, `formatStructuredRateLimit`, and `isExhausted`.
- **Dual-mode reset delay**: `computeResetDelaySecs` seamlessly handles both IETF relative delta-seconds (`<= 1_000_000_000`) and legacy Unix Epoch timestamps.

## Usage

```cp
import { parsePolicy, parseStructuredRateLimit, isExhausted, computeResetDelaySecs } from "governor";

const pol = parsePolicy("100;w=60;burst=20");
const st = parseStructuredRateLimit("limit=100, remaining=0, reset=15");
const wait = computeResetDelaySecs(st.reset_secs, 1700000000); // 15
```
