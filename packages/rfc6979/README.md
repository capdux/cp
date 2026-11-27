[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rfc6979

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc6979` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))

Pure CP, zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, parity with RustCrypto `rfc6979`).

Reuses `hmacSha256` from `@cap/cookie-signature`.

## Features

- **`HmacDrbg256`**:
  - `init(priv_key_32: []u8, msg_hash_32: []u8, extra: []u8): HmacDrbg256` — Executes RFC 6979 Section 3.2 steps `b` through `g` (plus Section 3.6 extra data support for fault-injection resistance)
  - `nextBytes(*_, out: []u8): usize` — Produces deterministic 32-byte candidate and steps `(K, V)` per RFC 6979 Section 3.2.h.3
- **`generateK(priv_key_32, msg_hash_32, order_32, out_k32): bool`**: Generates a deterministic 256-bit big-endian ECDSA nonce `k` strictly satisfying `1 <= k < q`

## License

MIT
