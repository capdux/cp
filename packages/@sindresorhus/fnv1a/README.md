[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/fnv-hash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sindresorhus/fnv1a` ([https://github.com/sindresorhus/fnv1a](https://github.com/sindresorhus/fnv1a)) / `crates.io:const-fnv1a-hash` ([https://github.com/jonasbb/const-fnv1a-hash](https://github.com/jonasbb/const-fnv1a-hash))（原库名 `@sindresorhus/fnv1a` → CP 包名 `@cap/fnv-hash`）

Zero-heap FNV-1 and FNV-1a (32-bit and 64-bit) streaming non-cryptographic hash functions for CP.
Compatible with Rust `fnv` and npm `@sindresorhus/fnv1a`.

## API

- `fnv1a32(data: []const u8): u32` / `fnv1a32Update(state: u32, data: []const u8): u32`
- `fnv132(data: []const u8): u32`
- `fnv1a64(data: []const u8): u64` / `fnv1a64Update(state: u64, data: []const u8): u64`
- `fnv164(data: []const u8): u64`
- `formatHex64(h: u64, out: []u8): usize`

## License

MIT
