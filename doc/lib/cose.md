# `cap:cose` / `cap.cose` — COSE (RFC 9052 / RFC 9053) 标准库

## 1. 概述

`lib/cose` 提供纯 CP 实现的工业级 COSE（CBOR Object Signing and Encryption，RFC 9052 / RFC 9053）标准库，全面支撑 WebAuthn / Passkeys / FIDO2 凭证验证与 IoT 轻量级签名认证。

核心能力：
- **确定性 CBOR 编解码**：内置零外部耦合的 RFC 8949 确定性 CBOR `CborWriter` / `CborReader`。
- **COSE_Key (RFC 9052 §7)**：支持 `OKP (Ed25519)`、`EC2 (P-256)` 与 `Symmetric` 密钥的标准整数键 CBOR Map 往返与严格坐标校验。
- **COSE_Sign1 (RFC 9052 §4.2, Tag 18)**：严格遵循 `Sig_structure = ["Signature1", body_protected, external_aad, payload]`，支持 `EdDSA (-8)` 与 `ES256 (-7)`。
- **COSE_Mac0 (RFC 9052 §6.2, Tag 17)**：严格遵循 `MAC_structure = ["MAC0", body_protected, external_aad, payload]`，支持 `HS256 (5)` 常量时间 MAC 验证。

## 2. 核心类型

| 类型 | 说明 |
| --- | --- |
| `CoseError` | 错误集合（`InvalidCbor`、`UnexpectedType`、`UnexpectedTag`、`MissingField`、`InvalidAlgorithm`、`InvalidKeyType`、`InvalidCurve`、`InvalidKeyLength`、`InvalidSignature`、`InvalidMac`、`UnsupportedAlgorithm`、`OutOfMemory`） |
| `CoseAlgorithm` | 算法枚举：`ES256 = -7`、`EdDSA = -8`、`HS256 = 5`、`RS256 = -257` |
| `CoseKeyType` | 密钥类型枚举：`OKP = 1`、`EC2 = 2`、`RSA = 3`、`Symmetric = 4` |
| `CoseCurve` | 曲线枚举：`P256 = 1`、`P384 = 2`、`P521 = 3`、`X25519 = 4`、`X448 = 5`、`Ed25519 = 6`、`Ed448 = 7` |
| `CoseHeader` | 头部参数：`alg: i32?`、`kid: ([]const u8)?`、`content_type: ([]const u8)?` |
| `CoseKey` | COSE 密钥结构（`kty`, `alg`, `kid`, `crv`, `x`, `y`, `d`, `k`） |
| `CoseSign1` | COSE_Sign1 解码结构（`protected_bytes`, `unprotected`, `payload`, `signature`） |
| `CoseMac0` | COSE_Mac0 解码结构（`protected_bytes`, `unprotected`, `payload`, `tag`） |

## 3. API 速查

- **密钥编解码**：
  - `encodeCoseKey(alloc, key): CoseError![]u8`
  - `decodeCoseKey(alloc, cbor_bytes): CoseError!CoseKey`
- **COSE_Sign1**：
  - `encodeSigStructure(alloc, protected_bytes, external_aad, payload): CoseError![]u8`
  - `encodeCoseSign1(alloc, sign1, with_tag): CoseError![]u8`
  - `decodeCoseSign1(alloc, cbor_bytes): CoseError!CoseSign1`
  - `sign1Ed25519(alloc, secret_seed, payload, external_aad, kid, with_tag): CoseError![]u8`
  - `verify1Ed25519(alloc, public_key, cbor_bytes, external_aad): CoseError![]u8`
  - `sign1Es256(alloc, secret_key, payload, external_aad, kid, with_tag): CoseError![]u8`
  - `verify1Es256(alloc, x, y, cbor_bytes, external_aad): CoseError![]u8`
- **COSE_Mac0**：
  - `encodeMacStructure(alloc, protected_bytes, external_aad, payload): CoseError![]u8`
  - `encodeCoseMac0(alloc, mac0, with_tag): CoseError![]u8`
  - `decodeCoseMac0(alloc, cbor_bytes): CoseError!CoseMac0`
  - `mac0Hs256(alloc, key, payload, external_aad, kid, with_tag): CoseError![]u8`
  - `verifyMac0Hs256(alloc, key, cbor_bytes, external_aad): CoseError![]u8`

## 4. 示例

```cp
import { sign1Ed25519, verify1Ed25519 } from "cap:cose";
const std = @import("std");

pub fn demo(alloc: std.mem.Allocator): !void {
    const kp = std.crypto.sign.Ed25519.KeyPair.generate();
    const msg = "webauthn-challenge";
    const aad = "origin:https://example.com";

    const signed = try sign1Ed25519(alloc, kp.secret_key.seed(), msg, aad, "kid-1", true);
    defer alloc.free(signed);

    const verified = try verify1Ed25519(alloc, kp.public_key.toBytes(), signed, aad);
    defer alloc.free(verified);
}
```
