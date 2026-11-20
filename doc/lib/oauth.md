# CP 标准库：OAuth 2.0 与 OIDC 认证客户端原语 (`cap:oauth` / `cap.oauth`)

## 一、 架构总览

`cap:oauth` 是 CP 语言官方内置的零外部依赖 OAuth 2.0 与 OpenID Connect (OIDC) 认证客户端核心库，完整覆盖 **RFC 7636** PKCE (`S256`) 挑战生成与恒定时间校验、**RFC 6749** 授权 URL 构建与表单令牌请求编解码、OIDC/OAuth Token JSON 响应解析与时钟偏移过期判定，以及 **RFC 6750 / RFC 7617** `Bearer` / `Basic` HTTP `Authorization` 头编解码。全模块**零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:oauth"` 与 `import cap from "cap"; cap.oauth.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/oauth/pkce.cp` | `// Source: cp/lib/oauth/pkce.cp (RFC 7636 PKCE S256 & State Token Generator)` | `OAuthError`、`PkceChallenge`、`Pkce`（CSPRNG `S256` 生成/验证与高熵 `state`） |
| `lib/oauth/flow.cp` | `// Source: cp/lib/oauth/flow.cp (RFC 6749 OAuth 2.0 Authorization URL, Token Codec & AuthHeader)` | `OAuthConfig`、`OAuthFlow`、`OAuthTokenResponse`、`BasicCredentials`、`AuthHeader` |
| `lib/oauth/index.cp` | `// Source: cp/lib/oauth/index.cp (Unified Exports for cap:oauth)` | `cap:oauth` 与 `cap.oauth` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. RFC 7636 PKCE S256 与 CSRF State（`pkce.cp`）

- **`OAuthError`**：`InvalidVerifier`、`InvalidConfig`、`InvalidTokenResponse`、`InvalidAuthHeader`、`OutOfMemory`
- **`PkceChallenge`**（`verifier: string`、`challenge: string`、`method: string = "S256"`、`deinit(*_)`）
- **`Pkce`**：
  - `Pkce.generate(alloc): PkceChallenge ! OAuthError`：使用 `std.os.linux.getrandom` 生成 32 字节高熵 `verifier`（43 字符无填充 Base64URL），计算 `BASE64URL(SHA256(verifier))` 作为 `challenge`
  - `Pkce.fromVerifier(alloc, verifier): PkceChallenge ! OAuthError`：校验 RFC 7636 长度 `[43, 128]` 与非保留字符集 `[A-Za-z0-9\-._~]` 并派生 S256 challenge
  - `Pkce.computeS256Challenge(alloc, verifier): ([]u8) ! OAuthError`：计算并返回 S256 `code_challenge`
  - `Pkce.isValidVerifier(verifier): bool`：无分配快速校验 `code_verifier` 合法性
  - `Pkce.verifyS256(verifier, challenge): bool`：使用 `std.crypto.timing_safe.eql` 执行恒定时间 S256 校验
  - `Pkce.generateState(alloc): ([]u8) ! OAuthError`：生成 256-bit 高熵 Base64URL 防 CSRF `state` 参数

### 2. RFC 6749 授权 URL、Token 请求体与响应解析（`flow.cp`）

- **`OAuthConfig`**：`client_id`、`client_secret: string?`、`authorize_url`、`token_url`、`redirect_uri`、`scopes: []const string`
- **`OAuthFlow`**：
  - `buildAuthorizeUrl(alloc, cfg, state, pkce: PkceChallenge?): ([]u8) ! OAuthError`：对所有查询参数执行 RFC 3986 百分号编码，自动兼容基址是否已含 `?` / `&`，禁止控制字符及 `#` 片段
  - `buildAuthorizationCodeBody(alloc, cfg, code, pkce_verifier: string?): ([]u8) ! OAuthError`：生成 `application/x-www-form-urlencoded` 授权码换取请求体（含可选 `client_secret` 与 PKCE `code_verifier` 校验）
  - `buildRefreshTokenBody(alloc, cfg, refresh_token): ([]u8) ! OAuthError`：生成 `refresh_token` 刷新请求体
  - `buildClientCredentialsBody(alloc, cfg): ([]u8) ! OAuthError`：生成 `client_credentials` 客户端凭证请求体
- **`OAuthTokenResponse`**（`access_token`、`token_type`、`expires_in: i64?`、`refresh_token: string?`、`scope: string?`、`id_token: string?`、`deinit(*_)`）：
  - `OAuthTokenResponse.parseJson(alloc, json_str): OAuthTokenResponse ! OAuthError`：基于 `cap:json` 严格解析 Token 端点 JSON 响应，强制要求非空 `access_token` 与 `token_type`
  - `isExpiredAt(issued_at_sec, now_sec, skew_sec): bool`：支持时钟偏移容差（Clock Skew）的过期判定
  - `hasScope(target): bool`：按空格精确分词检查是否包含目标 `scope`
- **`AuthHeader` & `BasicCredentials`**：
  - `formatBearer(alloc, token)` / `parseBearer(header): string?`（大小写不敏感匹配 `Bearer `）
  - `formatBasic(alloc, user, pass)` / `parseBasic(alloc, header): BasicCredentials ! OAuthError`（RFC 7617 Base64 编解码 `user:pass`，禁止用户名含 `:` 与控制字符）

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Pkce, OAuthConfig, OAuthFlow, OAuthTokenResponse, AuthHeader } from "cap:oauth";

pub fn demoOAuth(): !void {
    const alloc = std.heap.page_allocator;

    let pkce = Pkce.generate(alloc)?;
    defer pkce.deinit();

    const state = Pkce.generateState(alloc)?;
    defer alloc.free(state);

    const scopes: [2]string = ["openid", "profile"];
    const cfg = OAuthConfig{
        client_id: "cp-cli-client",
        authorize_url: "https://idp.example.com/oauth2/authorize",
        token_url: "https://idp.example.com/oauth2/token",
        redirect_uri: "http://localhost:8080/callback",
        scopes: &scopes,
    };

    const url = OAuthFlow.buildAuthorizeUrl(alloc, cfg, state, pkce)?;
    defer alloc.free(url);

    let tok = OAuthTokenResponse.parseJson(
        alloc,
        "{\"access_token\":\"at_123\",\"token_type\":\"Bearer\",\"expires_in\":3600,\"scope\":\"openid profile\"}",
    )?;
    defer tok.deinit();

    const header = cap.oauth.AuthHeader.formatBearer(alloc, tok.access_token)?;
    defer alloc.free(header);
}
```
