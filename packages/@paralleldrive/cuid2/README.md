[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cuid2

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@paralleldrive/cuid2` ([https://github.com/paralleldrive/cuid2](https://github.com/paralleldrive/cuid2))（原库名 `@paralleldrive/cuid2` → CP 包名 `@cap/cuid2`）

Clean-room MIT, zero-heap collision-resistant CUID2 ID generator & validator for CP — modeled on `@paralleldrive/cuid2`.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — fingerprint & counter entropy mixing.

## API
- Constants: `CUID2_DEFAULT_LEN (24)`, `CUID2_MIN_LEN (2)`, `CUID2_MAX_LEN (32)`
- `isCuid2(id): bool` & `isCuid2WithBounds(id, min_len, max_len): bool`
- `Cuid2Generator` (`new`, `length`, `next`)
