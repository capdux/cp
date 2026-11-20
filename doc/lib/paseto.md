# `cap:paseto` — PASETO v4 安全令牌标准库

> **模块路径**：`lib/paseto/`（`cap:paseto` & `cap.paseto` 动态挂载）  
> **定位**：纯 CP、零 C 依赖的 **PASETO (Platform-Agnostic Security Tokens) v4** 安全令牌引擎，为 CP 零信任 API 鉴权与 Agent 间安全凭证提供抗降级、防算法混淆的现代替代方案。

---

## 核心特性

1. **彻底免疫 JWT 经典漏洞**：
   - 协议头强绑定版本与用途（`v4.public.` / `v4.local.`），不支持 `alg:none`，杜绝密钥算法混淆（Key Confusion）攻击。
2. **Pre-Authentication Encoding (`paeEncode`)**：
   - 严格实现 `LE64(count) || LE64(len(p_i)) || p_i ...` 编码，将 Header、载荷/密文、Footer 与 Implicit Assertion 显式长度绑定认证，杜绝长度扩展与尾部替换攻击。
3. **`v4.public` 非对称签名**：
   - 基于 Ed25519（64 字节私钥签名 / 32 字节公钥验签），输出 `v4.public.<b64url>[.<b64url_footer>]`。
4. **`v4.local` 对称认证加密**：
   - 32 字节 CSPRNG 随机 Nonce + BLAKE3 密钥派生（派生 32B 加密密钥 `Ek` 与 24B 扩展 Nonce `n2`）+ `XChaCha20-Poly1305` AEAD 认证加密，输出 `v4.local.<b64url>[.<b64url_footer>]`。
5. **标准声明时间窗校验 (`verifyClaims`)**：
   - 结合 `cap:json` 校验 `exp`（过期时间）、`nbf`（生效时间）、`iat`、`jti`、`iss`、`aud`、`sub`，支持 `clock_skew_sec` 时钟漂移容忍，内部使用 `ArenaAllocator` 保证零泄漏。

---

## 快速上手

```typescript
import { PasetoV4, PasetoError, VerifyClaimsOptions } from "cap:paseto";
import { Ed25519KeyPair } from "cap:crypto";
import cap from "cap";

// 1. v4.public (Ed25519 非对称签名与验签)
const kp = Ed25519KeyPair.generate();
const token = PasetoV4.signPublic(
    alloc,
    kp.secret_key,
    "{\"sub\":\"agent-01\",\"iss\":\"cp-auth\",\"exp\":1900000000}",
    "{\"kid\":\"k1\"}",
    "tenant-prod",
)?;
defer alloc.free(token);

let parsed = PasetoV4.verifyPublic(alloc, kp.public_key, token, "tenant-prod")?;
defer parsed.deinit();

// 2. 标准声明校验
PasetoV4.verifyClaims(alloc, parsed.payload_json, VerifyClaimsOptions{
    now_unix: 1800000000,
    expected_iss: "cp-auth",
    expected_sub: "agent-01",
    clock_skew_sec: 30,
})?;

// 3. 动态挂载调用 (import cap from "cap")
const local_tok = cap.paseto.PasetoV4.encryptLocal(alloc, key32, "{\"role\":\"admin\"}", "", "ctx")?;
defer alloc.free(local_tok);
```

---

## API 契约

### 错误集与类型
- `PasetoError`：`InvalidTokenFormat` | `InvalidHeader` | `InvalidSignature` | `DecryptionFailed` | `TokenExpired` | `TokenNotYetValid` | `ClaimMismatch` | `InvalidKey` | `OutOfMemory`
- `PasetoPurpose`：`Local` | `Public`
- `PasetoParsed`：`{ header: string, payload_json: []u8, footer: []u8, purpose: PasetoPurpose, allocator: std.mem.Allocator }`，提供 `deinit(*_)` 释放内部分配的切片。
- `VerifyClaimsOptions`：`{ now_unix: i64 = 0, expected_iss: string? = null, expected_aud: string? = null, expected_sub: string? = null, clock_skew_sec: u64 = 0 }`

### 公开函数与方法
- `paeEncode(alloc: std.mem.Allocator, pieces: []const []const u8): PasetoError![]u8`
- `PasetoV4.encryptLocal(alloc, key32: [32]u8, payload_json: string, footer: string, implicit: string): PasetoError![]u8`
- `PasetoV4.decryptLocal(alloc, key32: [32]u8, token: string, implicit: string): PasetoError!PasetoParsed`
- `PasetoV4.signPublic(alloc, ed25519_secret64: [64]u8, payload_json: string, footer: string, implicit: string): PasetoError![]u8`
- `PasetoV4.verifyPublic(alloc, ed25519_pub32: [32]u8, token: string, implicit: string): PasetoError!PasetoParsed`
- `PasetoV4.verifyClaims(alloc, payload_json: string, opts: VerifyClaimsOptions): PasetoError!void`
