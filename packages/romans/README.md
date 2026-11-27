[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/roman-num`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:romans` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:roman` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `romans` → CP 包名 `@cap/roman-num`）

Zero-heap-allocation strict Roman numeral (`1..3999`) bidirectional parser, validator, and formatter in pure CP.

## Features

- **`toRoman`**: Formats `1..3999` to canonical uppercase Roman numerals (`I..MMMCMXCIX`).
- **`parseRoman`**: Case-insensitive parser with canonical round-trip validation rejecting non-standard forms (`IIII`, `VV`, `IL`, `IC`, `XM`).
- **`isValidRoman`**: Strict validation predicate.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { toRoman, parseRoman, isValidRoman } from "@cap/roman-num";
```

## License

MIT
