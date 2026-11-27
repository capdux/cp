[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/sodium

> 版本：`1.0.20` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

libsodium 1.0.20 modern cryptography suite (Ed25519, XChaCha20-Poly1305 AEAD, BLAKE2b, CSPRNG) for CP

## 安装与导入

```bash
cap add @cap/sodium
```

```cp
import { sodium_init } from "@cap/sodium";
```

## 核心导出 API

- `export fn sodium_init(): i32;`
- `export fn sodium_version_string(): [*:0]&u8!?;`
- `export fn randombytes_buf(buf: [*]u8!, size: usize);`
- `export fn crypto_sign_ed25519_keypair(pk: [*]u8!, sk: [*]u8!): i32;`
- `export fn crypto_sign_ed25519_detached(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32;`
- `export fn crypto_sign_ed25519_verify_detached(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32;`
- `export fn crypto_aead_xchacha20poly1305_ietf_encrypt(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32;`
- `export fn crypto_aead_xchacha20poly1305_ietf_decrypt(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32;`
- `export fn crypto_generichash(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`libsodium@1.0.20`)，许可证：ISC
