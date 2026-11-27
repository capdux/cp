[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# strum

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strum` ([https://github.com/Peternator7/strum](https://github.com/Peternator7/strum))（原库名 `strum` → CP 包名 `strum`）

Clean-room MIT, zero-heap enum variant string registry, discriminant lookup, and case-insensitive parser for CP — modeled on Rust's `strum` crate (900M+ downloads).

## Dependencies
- `unicase` (`1.0.0`) — ASCII case-insensitive enum parsing (`fromStrIgnoreCase`).

## API
- `EnumRegistry` (`new`, `register`, `variantCount`, `fromStr`, `fromStrIgnoreCase`, `asStr`, `getMessage`, `variantAt`, `formatVariantList`)
