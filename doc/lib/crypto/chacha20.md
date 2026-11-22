# `cap:chacha20` — 纯 CP RFC 8439 ChaCha20-Poly1305 / XChaCha20 AEAD 加密引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 概述

`cap:chacha20`（亦支持 `cap.chacha20` 动态挂载）基于 Zig 0.17 `std.crypto.aead.chacha_poly` 提供标准 12 字节 Nonce `ChaCha20Poly1305` 与 24 字节扩展 Nonce `XChaCha20Poly1305` 认证加密/解密，支持零堆缓冲区与 `sealXAlloc`/`openXAlloc` 一体化封装。

## 核心类型与 API

- `KEY_LEN` (`32`)、`NONCE_LEN` (`12`)、`XNONCE_LEN` (`24`)、`TAG_LEN` (`16`)
- `ChaChaError`: `AuthenticationFailed`、`BufferTooSmall`
- `encrypt` / `decrypt`、`encryptX` / `decryptX`、`sealXAlloc` / `openXAlloc`

## 示例

```cp
import { sealXAlloc, openXAlloc } from "cap:crypto";

const sealed = try sealXAlloc(alloc, key, "aad", "secret message");
defer alloc.free(sealed);
const plain = try openXAlloc(alloc, key, "aad", sealed);
defer alloc.free(plain);
```
