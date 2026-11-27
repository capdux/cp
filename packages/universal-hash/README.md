[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ghash-gf128

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:universal-hash` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))（原库名 `universal-hash` → CP 包名 `@cap/ghash-gf128`）

Zero-heap-allocation NIST SP 800-38D GF(2^128) universal hash (`GHASH`, core authenticator for AES-GCM) in pure CP with branchless, overflow-free 128-round multiplication.

## Features

- `Block128 { hi, lo }`: `zero`, `one`, `fromBytes`, `toBytes`, `xor`, `eq`
- `gf128Mul(x, y): Block128`: branchless NIST SP 800-38D Algorithm 1 (`x^128 + x^7 + x^2 + x + 1`)
- `Ghash`: `new`, `updateBlock`, `updatePadded`, `updateLengths`, `finalize`, `reset`

## License

MIT
