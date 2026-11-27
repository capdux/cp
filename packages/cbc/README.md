[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cbc

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cbc` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes))

Zero-allocation pure CP implementation of AES-128-CBC and AES-256-CBC block cipher mode of operation with PKCS#7 padding and strict padding validation — clean-room parity with RustCrypto `cbc` (157.0M+ downloads on crates.io, MIT).

纯 CP 零堆分配 AES-128-CBC / AES-256-CBC 块加密模式实现，直接复用 `@cap/aes` 底层分组密码原语，支持整块无填充（通过 NIST SP 800-38A F.2.1 / F.2.5 官方测试向量验证）与标准 PKCS#7 填充及严格去填充校验。

## Install / 安装

```bash
cap add @cap/cbc
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `aes128CbcEncryptBlocks` | `(key: [16]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize` | AES-128-CBC whole-block encryption (no padding) |
| `aes128CbcDecryptBlocks` | `(key: [16]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): usize` | AES-128-CBC whole-block decryption (no padding) |
| `aes128CbcEncryptPkcs7` | `(key: [16]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize` | AES-128-CBC encryption with PKCS#7 padding |
| `aes128CbcDecryptPkcs7` | `(key: [16]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): i64` | AES-128-CBC decryption with strict PKCS#7 validation (`-1` on error) |
| `aes256CbcEncryptBlocks` | `(key: [32]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize` | AES-256-CBC whole-block encryption (no padding) |
| `aes256CbcDecryptBlocks` | `(key: [32]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): usize` | AES-256-CBC whole-block decryption (no padding) |
| `aes256CbcEncryptPkcs7` | `(key: [32]u8, iv: [16]u8, plaintext: []const u8, out: []u8): usize` | AES-256-CBC encryption with PKCS#7 padding |
| `aes256CbcDecryptPkcs7` | `(key: [32]u8, iv: [16]u8, ciphertext: []const u8, out: []u8): i64` | AES-256-CBC decryption with strict PKCS#7 validation (`-1` on error) |

## Usage / 使用示例

```cp
import { aes128CbcEncryptPkcs7, aes128CbcDecryptPkcs7 } from "@cap/cbc";

const key: [16]u8 = @as([16]u8, [
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
]);
const iv: [16]u8 = @as([16]u8, [
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
]);
let ct: [32]u8 = undefined;
let pt: [32]u8 = undefined;
const ctLen = aes128CbcEncryptPkcs7(key, iv, "hello cbc", ct[0..32]);
const ptLen = aes128CbcDecryptPkcs7(key, iv, ct[0..ctLen], pt[0..32]);
```
