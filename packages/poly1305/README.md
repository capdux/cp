[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# poly1305

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:poly1305` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))

Pure CP, zero-allocation RFC 8439 Poly1305 one-time message authentication code (parity with RustCrypto `poly1305`).

Uses Donna 26-bit limb arithmetic mod `2^130 - 5` with overflow-free constant-time reduction, and reuses `ctEqBytes` from `@cap/subtle-ct` for constant-time tag verification.

## API

- `Poly1305`: `init(key32: []u8)`, `update(*_, data: []u8)`, `finalize(*_, out_tag16: []u8)`
- `poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool`
- `poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool`

## License

MIT
