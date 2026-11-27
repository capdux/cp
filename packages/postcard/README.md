[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# postcard

Zero-heap compact binary wire protocol codecs in pure CP implementing unsigned/signed LEB128 varints, ZigZag encoding, and COBS (Consistent Overhead Byte Stuffing) framing.

## API

- `encodeVarintU64(val: u64, out: []u8): usize`
- `decodeVarintU64(data: []const u8): VarintU64Res` (`{ ok, value, consumed }`)
- `zigzagEncode(val: i64): u64`
- `zigzagDecode(val: u64): i64`
- `encodeVarintI64(val: i64, out: []u8): usize`
- `decodeVarintI64(data: []const u8): VarintI64Res` (`{ ok, value, consumed }`)
- `cobsEncode(src: []const u8, out: []u8): usize`
- `cobsDecode(src: []const u8, out: []u8): i64` (returns `-1` on invalid frame or insufficient output buffer)

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`postcard@1.1.3`)
- **Upstream Repository**: https://github.com/jamesmunns/postcard
- **License**: MIT (clean-room CP implementation)
