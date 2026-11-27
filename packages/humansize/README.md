[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/humansize`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:humansize` ([https://github.com/LeopoldArkham/humansize](https://github.com/LeopoldArkham/humansize))（原库名 `humansize` → CP 包名 `@cap/humansize`）

Zero-heap human-readable byte size formatter in pure CP, modeled after Rust's `humansize` v2 crate. Supports SI `Decimal` (`1000`, `kB`/`MB`), IEC `Binary` (`1024`, `KiB`/`MiB`), and `Windows` (`1024`, `KB`/`MB`) bases, configurable decimal precision, optional space separator, and short/long unit names.

零堆分配纯 CP 人类可读文件大小格式化库，完整对标 `crates.io:humansize` v2：支持 `Decimal`（1000 基数）、`Binary`（1024 IEC 基数）、`Windows`（1024 KB/MB 基数），支持小数位数、空格分隔与长短单位配置。

## Installation

```bash
cap add @cap/humansize
```

## API

| Symbol | Description |
| --- | --- |
| `BaseUnit` | Enum: `Decimal`, `Binary`, `Windows` |
| `FormatSizeOptions` | `{ base, decimal_places, space_after_value, long_units }` |
| `DECIMAL` / `BINARY` / `WINDOWS` | Standard preset `FormatSizeOptions` |
| `formatSize(bytes, opts, out)` | Format `bytes` into `out`, returning bytes written |
| `formatDecimal(bytes, out)` | Format with `DECIMAL` preset |
| `formatBinary(bytes, out)` | Format with `BINARY` preset |

## License

MIT OR Apache-2.0
