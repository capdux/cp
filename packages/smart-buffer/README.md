[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/smart-buffer

Zero-heap cursor-based binary reader/writer buffer (1024-byte stack backing) in pure CP.

## API

- `SmartBuffer.new()` / `SmartBuffer.fromSlice(src)`
- Writers: `writeU8`, `writeU16BE`, `writeU16LE`, `writeU32BE`, `writeU32LE`, `writeString`, `writeStringNT`
- Readers: `readU8`, `readU16BE`, `readU16LE`, `readU32BE`, `readU32LE`, `readString(len)`
- Cursor: `remaining()`, `len()`, `rewind()`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`smart-buffer@4.2.0`)
- **Upstream Repository**: https://github.com/JoshGlazebrook/smart-buffer
- **License**: MIT (clean-room CP implementation)
