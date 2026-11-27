[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/jsonpath-rust

Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP.

## API

- `JpSegKind` — segment kind enum (`Root`, `Property`, `Index`, `Wildcard`, `RecursiveProp`, `RecursiveWildcard`, `Slice`)
- `JpSegment` — parsed path segment (`kind`, `name`, `index`, `slice_start`, `slice_end`, `has_start`, `has_end`)
- `parseJsonPath(expr, out): i64` — parses expressions like `$.store.book[0].title`, `$..author`, `$.items[*]`, `$.arr[1:4]`, `$['quoted.key']`; returns segment count or `-1` on syntax error
- `formatJsonPath(segs, out): usize` — serializes segments back to canonical JSONPath string

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`jsonpath-rust@0.7.1`)
- **Upstream Repository**: https://github.com/besok/jsonpath-rust
- **License**: MIT (clean-room CP implementation)
