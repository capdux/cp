[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/hmac-crate`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hmac` ([https://github.com/RustCrypto/MACs](https://github.com/RustCrypto/MACs))（原库名 `hmac` → CP 包名 `@cap/hmac-crate`）

Zero-heap RFC 2104 streaming HMAC-SHA256 MAC with constant-time `verifySlice`, arbitrary-length key hashing (>64B), and hex output — clean-room CP equivalent of crates.io `hmac`.

Reuses `@cap/cookie-signature` (`sha256` & `ctEqual`) as an independent package.
