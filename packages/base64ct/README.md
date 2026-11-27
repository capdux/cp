[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/base64ct

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base64ct` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats)) / `npm:base64-arraybuffer` ([https://github.com/niklasvh/base64-arraybuffer](https://github.com/niklasvh/base64-arraybuffer))

Pure CP, zero-allocation, constant-time (table-free) Base64 encoder and strict canonical RFC 4648 decoder (parity with RustCrypto `base64ct`).

## Security & Design

- **Zero Lookup Tables**: Uses branchless bitwise arithmetic for both 6-bit encoding and decoding to prevent cache-timing side-channel leakage.
- **Strict Canonical Decoding**: Rejects invalid characters, misplaced padding, and non-zero trailing bits per RFC 4648 Section 3.5, and zeroes the output slice on failure.
- **Zero Heap Allocation**: All operations write directly into caller-provided `[]u8` buffers.

## API

- `encodedLen(n: usize, padded: bool): usize`
- `encodeStd(src: []u8, out: []u8): usize`
- `encodeStdUnpadded(src: []u8, out: []u8): usize`
- `encodeUrl(src: []u8, out: []u8): usize`
- `encodeUrlUnpadded(src: []u8, out: []u8): usize`
- `decodeStd(src: string, out: []u8): DecodeResult`
- `decodeUrlUnpadded(src: string, out: []u8): DecodeResult`

## License

MIT
