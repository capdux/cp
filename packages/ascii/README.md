[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ascii

Zero-heap ASCII validation, byte classification, case conversion, trimming, and case-insensitive comparison in pure CP.

## API

- `isAscii(s)` / `firstNonAscii(s)`
- `isAsciiAlpha` / `isAsciiDigit` / `isAsciiAlphanumeric` / `isAsciiHexDigit` / `isAsciiWhitespace` / `isAsciiPrintable` / `isAsciiControl`
- `toAsciiLowerByte` / `toAsciiUpperByte` / `toAsciiLower` / `toAsciiUpper`
- `eqIgnoreAsciiCase(a, b)` / `trimAscii(s)`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`ascii@1.1.0`)
- **Upstream Repository**: https://github.com/tomprogrammer/rust-ascii
- **License**: MIT (clean-room CP implementation)
