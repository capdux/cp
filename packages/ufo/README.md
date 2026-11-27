[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ufo

Zero-heap URL protocol detection, slash normalization, double-slash cleanup, and safe URL joining utilities in pure CP.

## API

- `hasProtocol(url: string): bool`
- `isRelative(url: string): bool`
- `withoutLeadingSlash(s: string): string` / `withoutTrailingSlash(s: string): string`
- `withLeadingSlash(s: string, out: []u8): usize` / `withTrailingSlash(s: string, out: []u8): usize`
- `cleanDoubleSlashes(url: string, out: []u8): usize`
- `joinUrl(base: string, segment: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`ufo@1.5.4`)
- **Upstream Repository**: https://github.com/unjs/ufo
- **License**: MIT (clean-room CP implementation)
