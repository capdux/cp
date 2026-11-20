# `cap:paseto` (`cap.paseto`) — PASETO v4 安全认证令牌标准库

## 1. 概述

`lib/paseto/` 是纯 CP 实现的工业级 PASETO v4（Platform-Agnostic Security Tokens）标准库，提供 `v4.public`（Ed25519 非对称签名）与 `v4.local`（XChaCha20 + BLAKE2b 密钥拆分与常量时间认证加密）完整协议栈，支持 Pre-Authentication Encoding (PAE)、可选 Footer、Implicit Assertion 与标准声明（`exp`、`nbf`、`iss`、`aud`）校验。

- 导入方式：`import { ... } from "cap:paseto";` 或 `import cap from "cap";`（通过 `cap.paseto` 访问）。

---

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `PasetoError` | 错误集合：`InvalidPrefix`、`InvalidPurpose`、`InvalidFormat`、`InvalidBase64`、`AuthenticationFailed`、`InvalidSignature`、`InvalidKey`、`ExpiredToken`、`NotYetValid`、`ClaimMismatch`、`OutOfMemory` |
| `PasetoPurpose` | 令牌用途枚举：`Local` / `Public` |
| `PasetoToken` | 解析后的令牌结构：`version`、`purpose`、`payload_b64`、`footer_b64` |
| `V4PublicKeyPair` | Ed25519 密钥对：`secret_key: [64]u8`、`public_key: [32]u8` |

---

## 3. API 速查

| 函数 | 签名摘要 | 说明 |
| :--- | :--- | :--- |
| `paeEncode` | `(alloc, pieces: []const []const u8) -> ([]u8) ! PasetoError` | 按 PASETO 规范执行 PAE 编码 |
| `generateV4PublicKeyPair` | `() -> V4PublicKeyPair` | 生成 Ed25519 公私钥对 |
| `signV4Public` | `(alloc, secret_key_64, payload, footer, implicit_assertion) -> ([]u8) ! PasetoError` | 签发 `v4.public.` 令牌 |
| `verifyV4Public` | `(alloc, public_key_32, token, implicit_assertion) -> ([]u8) ! PasetoError` | 验签并提取载荷 |
| `generateV4LocalKey` | `() -> [32]u8` | 生成 32 字节对称密钥 |
| `encryptV4Local` | `(alloc, key_32, payload, footer, implicit_assertion) -> ([]u8) ! PasetoError` | 加密签发 `v4.local.` 令牌 |
| `decryptV4Local` | `(alloc, key_32, token, implicit_assertion) -> ([]u8) ! PasetoError` | 认证并解密 `v4.local.` 令牌 |
| `extractFooter` | `(alloc, token) -> ([]u8) ! PasetoError` | 提取并解码可选 Footer |
| `validateClaims` | `(json_payload, now_iso_or_unix, issuer_opt, audience_opt) -> void ! PasetoError` | 校验 `exp`/`nbf`/`iss`/`aud` 声明 |

---

## 4. 示例

```cp
const std = @import("std");
import {
    generateV4PublicKeyPair,
    signV4Public,
    verifyV4Public,
    generateV4LocalKey,
    encryptV4Local,
    decryptV4Local,
    validateClaims,
} from "cap:paseto";

pub fn example(alloc: std.mem.Allocator): void ! anyerror {
    const kp = generateV4PublicKeyPair();
    const pub_tok = signV4Public(alloc, kp.secret_key, "{\"sub\":\"u1\",\"exp\":\"2099-01-01T00:00:00Z\"}", "kid:1", "ctx") ?;
    defer alloc.free(pub_tok);

    const payload = verifyV4Public(alloc, kp.public_key, pub_tok, "ctx") ?;
    defer alloc.free(payload);
    validateClaims(payload, "2026-01-01T00:00:00Z", null, null) ?;

    const key = generateV4LocalKey();
    const loc_tok = encryptV4Local(alloc, key, "secret-msg", "", "ctx") ?;
    defer alloc.free(loc_tok);

    const plain = decryptV4Local(alloc, key, loc_tok, "ctx") ?;
    defer alloc.free(plain);
}
```
