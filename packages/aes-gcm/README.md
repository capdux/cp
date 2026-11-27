[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# aes-gcm

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aes-gcm` ([https://github.com/RustCrypto/AEADs](https://github.com/RustCrypto/AEADs))

Zero-allocation NIST SP 800-38D AES-128-GCM and AES-256-GCM (96-bit nonce) AEAD authenticated encryption for CP — clean-room parity with RustCrypto `aes-gcm` (229M+ downloads on crates.io), directly composing `aes` and `universal-hash`.

纯 CP 零堆分配 AES-128-GCM / AES-256-GCM 认证加密库（96-bit Nonce + 128-bit 认证标签 + 恒定时间标签校验），直接复用 `aes` 与 `universal-hash`，并通过 NIST SP 800-38D 官方测试向量逐字节验证。

## Install / 安装

```bash
cap add aes-gcm
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Aes128Gcm.new` | `(key: [16]u8) -> Aes128Gcm` | Initializes AES-128-GCM cipher and GHASH subkey `H` |
| `Aes128Gcm.encrypt` | `(nonce: [12]u8, aad: []const u8, plaintext: []const u8, out_ct: []u8) -> [16]u8` | Encrypts into `out_ct` and returns 16-byte auth tag |
| `Aes128Gcm.decrypt` | `(nonce: [12]u8, aad: []const u8, ciphertext: []const u8, tag: [16]u8, out_pt: []u8) -> bool` | Constant-time verifies tag and decrypts into `out_pt` |
| `Aes256Gcm.new` | `(key: [32]u8) -> Aes256Gcm` | Initializes AES-256-GCM cipher and GHASH subkey `H` |
| `Aes256Gcm.encrypt` | `(nonce: [12]u8, aad: []const u8, plaintext: []const u8, out_ct: []u8) -> [16]u8` | Encrypts into `out_ct` and returns 16-byte auth tag |
| `Aes256Gcm.decrypt` | `(nonce: [12]u8, aad: []const u8, ciphertext: []const u8, tag: [16]u8, out_pt: []u8) -> bool` | Constant-time verifies tag and decrypts into `out_pt` |

## Usage / 使用示例

```cp
import { Aes128Gcm } from "aes-gcm";

const cipher = Aes128Gcm.new(key);
let ct: [32]u8 = undefined;
const tag = cipher.encrypt(nonce, aad, plaintext, ct[0..plaintext.len]);

let pt: [32]u8 = undefined;
const ok = cipher.decrypt(nonce, aad, ct[0..plaintext.len], tag, pt[0..plaintext.len]);
```
