# `cap:webauthn` (`cap.webauthn`) — FIDO2 / WebAuthn Level 3 服务端验证标准库

## 1. 概述

`cap:webauthn`（亦可通过 `import cap from "cap";` 访问 `cap.webauthn`）提供纯 CP 实现的工业级 FIDO2 / WebAuthn Level 3 Passkey 服务端验证能力，底层复用 `cap:cose` 进行 `COSE_Key` 与 CBOR 解码，并结合 Zig 标准密码学原语实现零外部依赖、零内存泄漏的注册与登录断言验签：

- **AuthenticatorData 二进制拆包**：解析 `rpIdHash`（32B）、`flags`（`up` / `uv` / `be` / `bs` / `at` / `ed`）、大端 `signCount`、可选 `AttestedCredentialData`（16B `aaguid` + 2B `credIdLen` + `credential_id` + `COSE_Key`）及扩展 CBOR 边界校验。
- **ClientDataJSON 解析与常量时间比对**：校验 `type`（`"webauthn.create"` / `"webauthn.get"`）、`challenge` Base64URL 常量时间比对、`origin` 精确匹配及 `crossOrigin` 拦截。
- **Passkey 注册与登录断言验签**：支持 `fmt = "none"` / `"packed"` 自签名注册验证，以及 `ES256`（DER 编码 ECDSA P-256）与 `EdDSA`（64B Ed25519）登录断言验签与 `signCount` 防克隆严格递增校验。

---

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `WebAuthnError` | 错误集合（`InvalidLength`, `InvalidCbor`, `InvalidCoseKey`, `InvalidClientData`, `TypeMismatch`, `ChallengeMismatch`, `OriginMismatch`, `CrossOriginNotAllowed`, `RpIdMismatch`, `UserPresenceRequired`, `UserVerificationRequired`, `SignCountRollback`, `InvalidSignature`, `UnsupportedAlgorithm`, `UnsupportedAttestationFormat`, `MissingCredentialData`, `TrailingBytes`, `OutOfMemory`） |
| `AuthFlags` | 认证器标志位（`up`, `uv`, `be`, `bs`, `at`, `ed`, `raw`），提供 `fromByte(b: u8)` 与 `toByte()` |
| `AttestedCredData` | 证明凭据数据（`aaguid: [16]u8`, `credential_id: []u8`, `credential_public_key: CoseKey`），需调用 `deinit(alloc)` |
| `AuthenticatorData` | 解析后的认证器数据（`rp_id_hash`, `flags`, `sign_count`, `attested_credential_data`, `extensions`），需调用 `deinit(alloc)` |
| `ClientData` | 解析后的 `CollectedClientData`（`type_str`, `challenge`, `origin`, `cross_origin`），需调用 `deinit(alloc)` |
| `ExpectedClientData` | 服务端预期比对参数（`expected_type`, `expected_challenge`, `expected_origin`, `allow_cross_origin`） |
| `RegistrationResult` | 注册验证结果（`rp_id_hash`, `flags`, `sign_count`, `aaguid`, `credential_id`, `credential_public_key`），需调用 `deinit(alloc)` |
| `AssertionResult` | 登录断言验证结果（`rp_id_hash`, `flags`, `sign_count`） |

---

## 3. API 速查

- `parseAuthenticatorData(alloc: std.mem.Allocator, raw: []const u8): WebAuthnError!AuthenticatorData`
  解析 WebAuthn `authData` 二进制载荷，自动提取可选 `AttestedCredData`（含 `CoseKey`）与扩展 CBOR。
- `encodeChallengeB64u(alloc: std.mem.Allocator, raw_challenge: []const u8): WebAuthnError![]u8`
  将原始随机 challenge 字节编码为 Base64URL（无填充）字符串。
- `constantTimeEql(a: []const u8, b: []const u8): bool`
  常量时间字节切片比对，防止计时侧信道。
- `parseClientDataJson(alloc: std.mem.Allocator, json_bytes: []const u8): WebAuthnError!ClientData`
  解析 `clientDataJSON` 并提取 `type`、`challenge`、`origin` 与可选 `crossOrigin`。
- `verifyClientData(client_data: ClientData, expected: ExpectedClientData): WebAuthnError!void`
  校验 `type`、`challenge`（常量时间）、`origin` 与 `crossOrigin`。
- `computeRpIdHash(rp_id: []const u8): [32]u8`
  计算 `SHA-256(rp_id)`。
- `computeClientDataHash(client_data_json: []const u8): [32]u8`
  计算 `SHA-256(client_data_json)`。
- `verifyRegistrationNone(alloc, attestation_object, client_data_json, rp_id, expected_challenge, expected_origin, require_uv): WebAuthnError!RegistrationResult`
  验证 `fmt = "none"` / `"packed"` 注册响应并提取凭据 ID 与 `CoseKey` 公钥。
- `verifyAssertion(alloc, auth_data_bytes, client_data_json, signature, public_key, rp_id, expected_challenge, expected_origin, stored_sign_count, require_uv): WebAuthnError!AssertionResult`
  验证登录断言：`clientDataJSON`、`rpIdHash`、`UP`/`UV`、`signCount` 防克隆严格递增及 `ES256`/`EdDSA` 签名。

---

## 4. 示例

```cp
const std = @import("std");
import {
    encodeChallengeB64u,
    verifyRegistrationNone,
    verifyAssertion,
} from "cap:webauthn";

pub fn verifyPasskeyFlow(
    alloc: std.mem.Allocator,
    attestation_obj: []const u8,
    reg_client_data: []const u8,
    auth_data: []const u8,
    login_client_data: []const u8,
    signature: []const u8,
    raw_challenge: []const u8,
): !void {
    const challenge_b64u = try encodeChallengeB64u(alloc, raw_challenge);
    defer alloc.free(challenge_b64u);

    let reg = try verifyRegistrationNone(
        alloc,
        attestation_obj,
        reg_client_data,
        "example.com",
        challenge_b64u,
        "https://example.com",
        true,
    );
    defer reg.deinit(alloc);

    const assertion = try verifyAssertion(
        alloc,
        auth_data,
        login_client_data,
        signature,
        reg.credential_public_key,
        "example.com",
        challenge_b64u,
        "https://example.com",
        reg.sign_count,
        true,
    );
    _ = assertion;
}
```
