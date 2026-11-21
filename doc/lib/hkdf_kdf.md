# `cap:hkdf_kdf` — 纯 CP RFC 5869 HKDF 密钥派生引擎

## 概述

`cap:hkdf_kdf`（亦支持 `cap.hkdf_kdf` 动态挂载）基于 RFC 5869 提供零堆固定缓冲区与按需分配两种模式的 HKDF-Extract / HKDF-Expand / 一步派生 API，覆盖 SHA-256、SHA-384、SHA-512，并内置 TLS 1.3 `HKDF-Expand-Label` 辅助函数。

## 核心类型

- `HkdfHash`: `.Sha256` | `.Sha384` | `.Sha512`，提供 `prkLen()` 与 `maxOutputLen()` (`255 * HashLen`)。
- `HkdfError`: `OutputTooLong`、`InvalidPrkLength`。

## API 速查

- `extractSha256(salt, ikm) -> [32]u8` / `expandSha256(prk, info, out)` / `deriveSha256(salt, ikm, info, out)`
- `extractSha384(salt, ikm) -> [48]u8` / `expandSha384(prk, info, out)` / `deriveSha384(salt, ikm, info, out)`
- `extractSha512(salt, ikm) -> [64]u8` / `expandSha512(prk, info, out)` / `deriveSha512(salt, ikm, info, out)`
- `deriveAlloc(alloc, alg, salt, ikm, info, out_len) -> ![]u8`
- `expandLabelSha256(prk, label, context, out)`

## 示例

```cp
import { deriveSha256 } from "cap:hkdf_kdf";

let okm: [42]u8 = undefined;
try deriveSha256("salt", "ikm", "info", &okm);
```
