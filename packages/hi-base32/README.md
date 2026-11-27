[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/base32-rfc`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hi-base32` ([https://github.com/emn178/hi-base32](https://github.com/emn178/hi-base32))（原库名 `hi-base32` → CP 包名 `@cap/base32-rfc`）

Zero-heap-allocation RFC 4648 Section 6/7 standard Base32 (`A-Z2-7`) and Extended Hex Base32 (`0-9A-V`) encoder and decoder in pure CP.

## Features

- **Standard Base32 (`encode` / `decode`)**: RFC 4648 §6 `A-Z2-7`, optional `=` padding, case-insensitive decoding.
- **Extended Hex Base32 (`encodeHex` / `decodeHex`)**: RFC 4648 §7 `0-9A-V` lexicographically order-preserving encoding.
- **`encodedLen`**: Exact padded or unpadded length calculation.
- **Strict Validation**: Rejects non-alphabet characters (`0`, `1`, `8`, `9` in standard mode) and malformed padding.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { encode, decode, encodeHex, decodeHex, encodedLen } from "@cap/base32-rfc";
```

## License

MIT
