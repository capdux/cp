[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/base16ct

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base16ct` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Pure CP, zero-allocation, constant-time (table-free) Base16 / Hex encoder and strict decoder (parity with RustCrypto `base16ct`).

## Security & Design

- **Zero Lookup Tables**: Uses branchless bitwise arithmetic masks for both nibble encoding and decoding to prevent cache-timing side-channel leakage.
- **Strict Case Enforcement**: Separate `decodeLower` (`0-9a-f`), `decodeUpper` (`0-9A-F`), and `decodeMixed` (`0-9a-fA-F`) decoders with automatic output buffer zeroing on failure.
- **Zero Heap Allocation**: Writes directly into caller-provided `[]u8` slices.

## API

- `encodedLen(n: usize): usize`
- `encodeLower(src: []u8, out: []u8): usize`
- `encodeUpper(src: []u8, out: []u8): usize`
- `decodeLower(hex: string, out: []u8): HexDecodeResult`
- `decodeUpper(hex: string, out: []u8): HexDecodeResult`
- `decodeMixed(hex: string, out: []u8): HexDecodeResult`

## License

MIT
