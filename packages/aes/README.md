[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/aes

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aes` ([https://github.com/RustCrypto/block-ciphers](https://github.com/RustCrypto/block-ciphers))

Zero-allocation pure CP implementation of the FIPS-197 AES-128, AES-192, and AES-256 block ciphers — clean-room parity with RustCrypto `aes` (438M+ downloads on crates.io, MIT OR Apache-2.0).

纯 CP 零堆分配 FIPS-197 AES-128 / AES-192 / AES-256 分组密码实现，内置完整 S-Box / InvS-Box、Rcon、GF(2^8) 乘法 `xtime` 与标准 10 / 12 / 14 轮密钥扩展和加解密，已通过 FIPS-197 Appendix C.1 / C.2 / C.3 官方测试向量精确验证。

## Install / 安装

```bash
cap add @cap/aes
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Aes128` | `new(key: [16]u8)`, `encryptBlock(block: [16]u8): [16]u8`, `decryptBlock(block: [16]u8): [16]u8` | FIPS-197 AES-128 (10 rounds) |
| `Aes192` | `new(key: [24]u8)`, `encryptBlock(block: [16]u8): [16]u8`, `decryptBlock(block: [16]u8): [16]u8` | FIPS-197 AES-192 (12 rounds) |
| `Aes256` | `new(key: [32]u8)`, `encryptBlock(block: [16]u8): [16]u8`, `decryptBlock(block: [16]u8): [16]u8` | FIPS-197 AES-256 (14 rounds) |

## Usage / 使用示例

```cp
import { Aes128, Aes192, Aes256 } from "@cap/aes";

const key: [16]u8 = @as([16]u8, [
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
]);
const cipher = Aes128.new(key);
const ct = cipher.encryptBlock(key);
const pt = cipher.decryptBlock(ct);
```
