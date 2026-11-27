[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bl

Zero-heap segmented `BufferList` in pure CP for streaming byte buffers, cross-chunk `indexOf`, `copyRange`, `consume`, and big/little-endian integer reads.

## API

- `newBufferList(): BufferList`
- `bl.append(chunk: []const u8): bool`
- `bl.get(index: usize): u8?`
- `bl.copyRange(start, end, dst): usize`
- `bl.consume(n: usize): void`
- `bl.readU16BE(offset)` / `readU16LE(offset)` / `readU32BE(offset)` / `readU32LE(offset)`
- `bl.indexOf(needle: []const u8, from_offset: usize): usize?`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`bl@6.1.0`)
- **Upstream Repository**: https://github.com/rvagg/bl
- **License**: MIT (clean-room CP implementation)
