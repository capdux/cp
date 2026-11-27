[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# chardet

Zero-allocation character encoding detector in pure CP supporting BOM detection (UTF-8, UTF-16LE/BE, UTF-32LE/BE), BOM-less UTF-16 cadence detection, strict UTF-8/ASCII validation, Shift-JIS, GBK/GB18030, Windows-1252, and ISO-8859-1.

## API

- `detect(data: []const u8): CharsetMatch` — returns `{ charset, name, confidence }`.
- `detectName(data: []const u8): string` — returns the canonical encoding name (`"UTF-8"`, `"UTF-16LE"`, `"ASCII"`, `"Shift_JIS"`, `"GB18030"`, `"windows-1252"`, `"ISO-8859-1"`, etc.).
- `charsetName(cs: DetectedCharset): string` — formats a `DetectedCharset` enum value.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`chardet@2.1.0`)
- **Upstream Repository**: https://github.com/runk/node-chardet
- **License**: MIT (clean-room CP implementation)
