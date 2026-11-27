[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# cesu8

Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) ↔ standard UTF-8 transcoder in pure CP.

## API

- `toCesu8(utf8: []const u8, modified_nul: bool, out: []u8): i64`
- `fromCesu8(cesu: []const u8, modified_nul: bool, out: []u8): i64`
- `isValidCesu8AsciiOrBmp(utf8: []const u8, modified_nul: bool): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`cesu8@1.1.0`)
- **Upstream Repository**: https://github.com/emk/cesu8-rs
- **License**: MIT (clean-room CP implementation)
