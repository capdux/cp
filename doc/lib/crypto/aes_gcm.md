# `cap:aes_gcm` — AES-GCM & AES-GCM-SIV 认证加密标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 1. 概述

`cap:aes_gcm`（支持 `import * from "cap:crypto"` 与 `import cap from "cap"` 动态挂载 `cap.aes_gcm`）提供符合 **NIST SP 800-38D** 标准的 **AES-128-GCM**、**AES-256-GCM** 以及符合 **RFC 8452** 标准的抗 Nonce 重用 **AES-256-GCM-SIV** AEAD 认证加密原语，并提供自带 12 字节随机 Nonce 封装的 `seal256Alloc` / `open256Alloc` 便捷接口。

## 2. 核心类型与常量

| 名称 | 类型 | 说明 |
| --- | --- | --- |
| `AesGcmError` | `error` | `AuthenticationFailed`（Tag 校验失败或密文格式非法）、`BufferTooSmall`（输出缓冲区不足） |
| `KEY_LEN_128` | `usize` | AES-128 密钥长度（`16` 字节） |
| `KEY_LEN_256` | `usize` | AES-256 密钥长度（`32` 字节） |
| `NONCE_LEN` | `usize` | 标准 96-bit Nonce 长度（`12` 字节） |
| `TAG_LEN` | `usize` | 128-bit 认证标签长度（`16` 字节） |
| `Aes128Gcm` / `Aes256Gcm` / `Aes256GcmSiv` | `type` | 底层 Zig 标准库 AEAD 原语类型别名 |

## 3. API 速查

- `encrypt128(ciphertext: []u8, tag: *[16]u8, plaintext: []const u8, ad: []const u8, nonce: [12]u8, key: [16]u8): AesGcmError!void`
- `decrypt128(plaintext: []u8, ciphertext: []const u8, tag: [16]u8, ad: []const u8, nonce: [12]u8, key: [16]u8): AesGcmError!void`
- `encrypt256(ciphertext: []u8, tag: *[16]u8, plaintext: []const u8, ad: []const u8, nonce: [12]u8, key: [32]u8): AesGcmError!void`
- `decrypt256(plaintext: []u8, ciphertext: []const u8, tag: [16]u8, ad: []const u8, nonce: [12]u8, key: [32]u8): AesGcmError!void`
- `encrypt256Siv(ciphertext: []u8, tag: *[16]u8, plaintext: []const u8, ad: []const u8, nonce: [12]u8, key: [32]u8): AesGcmError!void`
- `decrypt256Siv(plaintext: []u8, ciphertext: []const u8, tag: [16]u8, ad: []const u8, nonce: [12]u8, key: [32]u8): AesGcmError!void`
- `seal256Alloc(alloc: std.mem.Allocator, plaintext: []const u8, ad: []const u8, key: [32]u8): ![]u8`（输出 `nonce(12B) || ciphertext || tag(16B)`）
- `open256Alloc(alloc: std.mem.Allocator, sealed: []const u8, ad: []const u8, key: [32]u8): ![]u8`

## 4. 示例

```cp
import { seal256Alloc, open256Alloc } from "cap:crypto";

const std = @import("std");
const key: [32]u8 = [_]u8{0x42} ** 32;
const sealed = try seal256Alloc(alloc, "secret payload", "header-aad", key);
defer alloc.free(sealed);

const plain = try open256Alloc(alloc, sealed, "header-aad", key);
defer alloc.free(plain);
```
