# CP 标准库：OAuth 2.0 / PKCE & OIDC 协议引擎手册 (`cap:oauth2` / `cap.oauth2`)

## 一、 架构总览与设计准则

`cap:oauth2` 是 CP 语言官方内置的纯 CP 零 C 依赖 OAuth 2.0 (RFC 6749)、PKCE (RFC 7636)、Token 吊销 (RFC 7009) 与 OpenID Connect (OIDC) 协议引擎标准库。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                     CP 用户层应用 (Application)                         │
├─────────────────────────────┬──────────────────────────────────────────┤
│   PKCE 安全引擎 (pkce.cp)    │   授权 URL 与回调解析 (authorize.cp)      │
│  generateCodeVerifier       │  buildAuthorizeUrl (RFC 3986)            │
│  deriveCodeChallengeS256    │  parseCallbackQuery (code/state/error)   │
│  verifyPkceChallenge        │  verifyCallbackState (Constant-Time)     │
├─────────────────────────────┼──────────────────────────────────────────┤
│   Token 编解码 (token.cp)    │   基础类型与安全原语 (types.cp)          │
│  buildAuthorizationCodeBody │  PkceMethod / PkcePair / OAuth2Config    │
│  buildClientCredentialsBody │  CallbackResult / TokenResponse          │
│  buildRefreshTokenBody      │  constantTimeEql / percentEncode/Decode  │
│  buildRevocationBody        │  OAuth2Error                             │
│  buildBasicAuthHeader       │                                          │
│  parseTokenResponse[OrError]│                                          │
└─────────────────────────────┴──────────────────────────────────────────┘
```

### 1. 模块文件拓扑

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/oauth2/types.cp` | 核心错误集 `OAuth2Error`、配置/响应结构体、常量时间比对 `constantTimeEql` 与 RFC 3986 百分号编解码 |
| `lib/oauth2/pkce.cp` | RFC 7636 PKCE `code_verifier` CSPRNG 生成、S256 摘要派生与零堆分配常量时间验证 |
| `lib/oauth2/authorize.cp` | 授权 URL 构建（支持 `state`、PKCE、`nonce`、`extra_params`）与回调查询解析/CSRF 防护 |
| `lib/oauth2/token.cp` | 三种 Grant + Revocation 表单请求体构建、RFC 6749 Basic Auth 头生成与 Token JSON 响应解析 |
| `lib/oauth2/index.cp` | `cap:oauth2` & `cap.oauth2` 对外统一导出入口 |

---

## 二、 核心 API 参考

### 1. PKCE (RFC 7636)

- `isValidCodeVerifier(verifier: []const u8): bool`：校验 `43 <= len <= 128` 及 unreserved 字符集 `[A-Za-z0-9-._~]`。
- `generateCodeVerifier(alloc, len_43_to_128): ([]u8)!OAuth2Error`：使用 `getrandom` CSPRNG + 拒绝采样生成无偏差高熵 `code_verifier`。
- `deriveCodeChallengeS256(alloc, verifier): ([]u8)!OAuth2Error`：计算 `BASE64URL-NOPAD(SHA256(verifier))`。
- `generatePkcePair(alloc, method, len_43_to_128): PkcePair!OAuth2Error`：一次生成 `PkcePair`（使用完需调用 `.deinit(alloc)`）。
- `verifyPkceChallenge(verifier, challenge, method): bool`：栈上零堆分配 + 常量时间比对验证。

### 2. 授权 URL 构建与回调解析

- `buildAuthorizeUrl(alloc, config, state, pkce_challenge, nonce, extra_params): ([]u8)!OAuth2Error`
  - 自动拼接 `response_type=code`、`client_id`、`redirect_uri`、`scope`、`state`、`code_challenge` & `code_challenge_method=S256`、`nonce` 与 `extra_params`，全部按 RFC 3986 百分号编码。
- `parseCallbackQuery(alloc, query_or_url): CallbackResult!OAuth2Error`
  - 兼容完整回调 URL、`?code=...` 或纯查询串；自动拒绝同时包含 `code` 与 `error` 或重复关键参数的畸形回调；使用完需调用 `.deinit(alloc)`。
- `verifyCallbackState(expected_state, actual_state): bool` / `result.verifyState(expected_state): bool`
  - 常量时间比对防 CSRF 攻击。

### 3. Token 请求体与响应解析

- `buildAuthorizationCodeBody(alloc, config, code, code_verifier)`
- `buildClientCredentialsBody(alloc, config, scopes)`
- `buildRefreshTokenBody(alloc, config, refresh_token, scopes)`
- `buildRevocationBody(alloc, token, token_type_hint, client_id, client_secret)`
- `buildBasicAuthHeader(alloc, client_id, client_secret)`：严格按 RFC 6749 Section 2.3.1 对 `client_id` 与 `client_secret` 先百分号编码再 Base64 编码。
- `parseTokenResponse(alloc, json_bytes): TokenResponse!OAuth2Error`：解析标准 Token JSON；遇到 OAuth2 `{"error":"..."}` 响应时返回 `OAuth2Error.OAuthErrorResponse`。
- `parseTokenResponseOrError(alloc, json_bytes): TokenResponse!OAuth2Error`：遇到 OAuth2 `error` 响应时填充 `oauth_error` / `error_description` / `error_uri` 返回。
- `TokenResponse` 方法：
  - `setIssuedAt(issued_at_sec)` / `isExpired(now_sec, skew_sec)`
  - `hasScope(target_scope)`（精确按空格分词匹配）
  - `deinit(alloc)`

---

## 三、 完整使用示例

```typescript
import {
    OAuth2Config,
    ExtraParam,
    PkceMethod,
    generatePkcePair,
    buildAuthorizeUrl,
    parseCallbackQuery,
    buildAuthorizationCodeBody,
    parseTokenResponse,
} from "cap:oauth2";
```
