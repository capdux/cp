# CP 标准库：认证、授权、访问控制与 PKI/X.509 证书手册 (`cap:auth` / `cap.auth`)

## 一、 架构总览与设计准则

`cap:auth` 是 CP 语言官方内置的纯原生零 C 依赖认证、授权、访问控制与 PKI/X.509 身份基础设施标准库，覆盖：
1. **OAuth 2.0 / OIDC、RBAC 访问控制、OPA Rego 与 TOTP/HOTP**（`oauth_pkce.cp`, `oauth_flow.cp`, `rbac_permission.cp`, `rbac_engine.cp`, `rego.cp`, `totp_hotp.cp`）
2. **企业网络认证协议**（`kerberos.cp`, `ntlm.cp`, `tacacs.cp`, `eap.cp`）
3. **PKI 与 X.509 证书链体系**（`asn1_der.cp`, `asn1_x509.cp`, `pem.cp`, `x509.cp`, `csr.cp`, `crl.cp`, `ocsp.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:auth"` 与 `import cap from "cap"; cap.auth.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/auth/oauth_pkce.cp` | RFC 7636 PKCE S256 挑战生成与校验（`Pkce`, `PkceChallenge`, `OAuthError`） |
| `lib/auth/oauth_flow.cp` | RFC 6749 OAuth 2.0 / OIDC 授权流、Token 响应解析与 Basic/Bearer 认证头 |
| `lib/auth/rbac_permission.cp` | 层次化通配符权限匹配器（`PermissionMatcher`, `PolicyEffect`, `RbacError`） |
| `lib/auth/rbac_engine.cp` | 企业级 RBAC / IAM 策略引擎（角色继承、环检测、显式拒绝优先） |
| `lib/auth/rego.cp` | OPA Rego 策略语言词法扫描器、包名提取与定界符校验 |
| `lib/auth/totp_hotp.cp` | RFC 4226 HOTP / RFC 6238 TOTP 动态口令、Base32 与 `otpauth://` URI |
| `lib/auth/kerberos.cp` | RFC 4120 Kerberos v5 ASN.1 DER 线协议解析器（Ticket/AP-REQ/KRB-ERROR） |
| `lib/auth/ntlm.cp` | MS-NLMP NTLM v1/v2 认证消息解析（Negotiate/Challenge/Authenticate/AV_PAIR） |
| `lib/auth/tacacs.cp` | RFC 8907 TACACS+ AAA 认证/授权/审计线协议解析器 |
| `lib/auth/eap.cp` | RFC 3748 EAP 与 IEEE 802.1X EAPOL 帧解析器 |
| `lib/auth/asn1_der.cp` | ITU-T X.690 ASN.1 DER 严格读写编解码器与 `OidCodec` 注册表 |
| `lib/auth/asn1_x509.cp` | Arena 驱动的 X.509 v3 证书元数据与 SHA-256 指纹检查器（`X509Inspector`） |
| `lib/auth/pem.cp` | RFC 7468 PEM 块与证书链多块解析/编码器 |
| `lib/auth/x509.cp` | RFC 5280 X.509 v3 证书解析、RFC 6125 SAN/主机名匹配与有效期校验 |
| `lib/auth/csr.cp` | RFC 2986 PKCS#10 证书签名请求（CSR）DER 解析器 |
| `lib/auth/crl.cp` | RFC 5280 X.509 证书吊销列表（CRL）DER 解析与统计器 |
| `lib/auth/ocsp.cp` | RFC 6960 在线证书状态协议（OCSP）请求与响应 DER 解析器 |
| `lib/auth/index.cp` | `cap:auth` 与 `cap.auth` 统一零冲突公开导出入口（共 243 符号） |

---

## 二、 核心 API 索引

### 1. OAuth 2.0 / RBAC / Rego / TOTP
- **OAuth 2.0（`oauth_pkce.cp`, `oauth_flow.cp`）**：`OAuthError`, `PkceChallenge`, `Pkce`, `OAuthConfig`, `OAuthFlow`, `OAuthTokenResponse`, `BasicCredentials`, `AuthHeader`
- **RBAC（`rbac_permission.cp`, `rbac_engine.cp`）**：`RbacError`, `PolicyEffect`, `PermissionMatcher`, `AccessDecision`, `RbacEngine`
- **OPA Rego（`rego.cp`）**：`REGO_MAX_DEPTH`, `RegoTokenKind`, `RegoToken`, `RegoIter`, `RegoImportIter`, `RegoRuleIter`, `extractRegoPackage`, `validateRegoDelimiters`
- **TOTP / HOTP（`totp_hotp.cp`）**：`HashAlg`, `OtpType`, `OtpAuthUri`, `hotpAtWithAlg`, `hotpAt`, `totpAt`, `formatCode`, `constantTimeEq`, `verifyHotp`, `verifyTotp`, `base32Encode`, `base32Decode`, `generateSecret`, `buildOtpAuthUri`, `parseOtpAuthUri`

### 2. 企业网络认证协议
- **Kerberos v5（`kerberos.cp`）**：`KRB_PORT`, `KRB_VERSION`, `KrbPrincipalName`, `KrbEncryptedData`, `KrbTicket`, `KrbMessageInfo`, `KrbError`, `unwrapKrbTcpRecord`, `peekKrbAppTag`, `isKerberos`, `parseKrbMessageInfo`, `parseKrbTicket`, `parseKrbError`
- **NTLM（`ntlm.cp`）**：`NTLM_SIGNATURE`, `NtlmVersion`, `NtlmAvPair`, `NtlmNegotiate`, `NtlmChallenge`, `NtlmAuthenticate`, `peekNtlmType`, `isNtlm`, `parseNtlmNegotiate`, `parseNtlmChallenge`, `parseNtlmAuthenticate`
- **TACACS+（`tacacs.cp`）**：`TACACS_TCP_PORT`, `TacacsType`, `TacacsHeader`, `TacacsPacket`, `parseTacacs`, `isTacacs`, `parseAuthenStart`, `parseAuthenReply`, `parseAuthorRequest`, `parseAuthorResponse`, `parseAcctRequest`, `parseAcctReply`
- **EAP / 802.1X（`eap.cp`）**：`EAPOL_ETHERTYPE`, `EapolType`, `EapCode`, `EapolFrame`, `EapPacket`, `EapExpanded`, `parseEapol`, `isEapol`, `parseEap`, `isEap`, `parseEapExpanded`

### 3. PKI 与 X.509 证书体系
- **ASN.1 DER（`asn1_der.cp`, `asn1_x509.cp`）**：`Asn1Error`, `DerElement`, `DerReader`, `DerWriter`, `OidCodec`, `X509CertInfo`, `X509Inspector`
- **PEM（`pem.cp`）**：`PemError`, `PemHeader`, `PemBlock`, `parse`, `parseWithMaxBlocks`, `parseFirst`, `encode`, `encodeWithHeaders`, `encodeChain`, `freeBlocks`
- **X.509 / CSR / CRL / OCSP**：`X509Certificate`, `parseAsn1Time`, `parseCsrDer`, `isCsrDer`, `parseCrlDer`, `summarizeCrl`, `parseOcspRequestDer`, `parseOcspResponseDer`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    RbacEngine,
    PolicyEffect,
    Pkce,
    totpAt,
    HashAlg,
} from "cap:auth";

pub fn demoAuth(alloc: std.mem.Allocator): !void {
    // 1. RBAC 权限评估
    let engine = RbacEngine.init(alloc);
    defer engine.deinit();
    engine.grantRole("admin", "agent:**", PolicyEffect.Allow)?;
    engine.assignUser("alice", "admin")?;
    const decision = engine.evaluate("alice", "agent:tool:read");
    * = decision;

    // 2. OAuth 2.0 PKCE S256 & cap.auth 动态挂载
    let pkce = cap.auth.Pkce.generate(alloc)?;
    defer pkce.deinit();
    * = Pkce.verifyS256(pkce.verifier, pkce.challenge);

    // 3. RFC 6238 TOTP
    const code = totpAt("12345678901234567890", 59, 30, 8, HashAlg.Sha1);
    * = code;
}
```
