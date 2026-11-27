[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ctr

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ctr` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes)) — `MIT OR Apache-2.0`

Zero-allocation NIST SP 800-38A CTR counter mode stream ciphers built on `@cap/aes` in pure CP.

## Features

- Reuses `@cap/aes` (`Aes128`, `Aes256`) with zero heap allocation
- `Aes128Ctr128Be`, `Aes128Ctr128Le`, `Aes256Ctr128Be`, `Aes256Ctr128Le`
- Streaming `.applyKeystream(*_, data: []u8)` across arbitrary chunk boundaries
- O(1) random-access `.seek(*_, byte_pos: u64)` & `.currentPos(): u64`
- Verified against NIST SP 800-38A Appendix F.5.1 (AES-128-CTR) and F.5.5 (AES-256-CTR) official test vectors

## License

MIT OR Apache-2.0
