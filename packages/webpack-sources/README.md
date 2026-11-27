[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# webpack-sources

Zero-heap source concatenation and range replacement engine in pure CP.

## API

- `SourceEdit { start: usize, end: usize, replacement: string }`
- `ReplaceSource` (fixed-capacity 64 edits)
  - `static fn init(original: string): ReplaceSource`
  - `fn replace(*_, start: usize, end: usize, replacement: string): bool`
  - `fn insert(*_, pos: usize, text: string): bool`
  - `fn remove(*_, start: usize, end: usize): bool`
  - `fn render(out: []u8): usize`
- `concatSources(parts: []const string, sep: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`webpack-sources@3.2.3`)
- **Upstream Repository**: https://github.com/webpack/webpack-sources
- **License**: MIT (clean-room CP implementation)
