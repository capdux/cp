[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/scroll

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:scroll` ([https://github.com/m4b/scroll](https://github.com/m4b/scroll)) / `crates.io:endian-type` ([https://github.com/dpc/endian-type](https://github.com/dpc/endian-type))

Zero-heap-allocation endian-aware (`Little` / `Big`) offset-advancing binary reader and writer cursors in pure CP.

## Features

- `Endian` enum (`Little`, `Big`)
- `PreadCursor { buf: []const u8, offset: usize }`: `new`, `readU8`, `readU16`, `readU32`, `readU64`, `readI32`, `readCstr`, `remaining`
- `PwriteCursor { buf: []u8, offset: usize }`: `new`, `writeU8`, `writeU16`, `writeU32`, `writeU64`, `written`, `remaining`
- Bounds-checked reads and writes that never advance the cursor on truncation or overflow

## License

MIT
