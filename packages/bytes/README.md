[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bytes

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bytes` ([https://github.com/visionmedia/bytes.js](https://github.com/visionmedia/bytes.js)) / `npm:bytes` ([https://github.com/visionmedia/bytes.js](https://github.com/visionmedia/bytes.js))

Zero-heap human-readable byte size parser and formatter for CP.
Compatible with npm `bytes` / `pretty-bytes` and Rust `bytesize` (supports both IEC binary `KiB/MiB/GiB` and SI decimal `KB/MB/GB`).

## API

- Constants: `KIB`, `MIB`, `GIB`, `TIB`, `PIB`, `KB`, `MB`, `GB`, `TB`, `PB`
- `parse(input: string): i64`
- `parseBytes(input: string, binary_default: bool): i64`
- `formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize`
- `formatIec(bytes: u64, out: []u8): usize`
- `formatSi(bytes: u64, out: []u8): usize`

## License

MIT
