# `cap:jwt` — RFC 7519 JWT Claims Zero-Allocation Parser

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

Zero-allocation, pure-CP RFC 7519 JSON Web Token (JWT) header & claims parser, Base64URL decoder, and temporal claim (`exp`/`nbf`/`iat`) validator.

## Module Layout

| File | Source | Responsibilities |
| :--- | :--- | :--- |
| `lib/jwt/claims.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 7519 JWT Claims Barrel)` | `decodeBase64Url`, `JwtParts`, `splitJwt`, `extractStringClaim`, `extractIntClaim`, `JwtHeader`, `ClaimStatus`, `JwtClaims` |
| `lib/jwt/index.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 7519 JWT Claims Barrel)` | Unified `cap:jwt` exports |

## 上游行为契约与安全边界注意事项

1. **三段式结构校验 (`splitJwt` & `decodeBase64Url`)**：
   - 严格要求精确 2 个 `.` 分隔符（`header.payload.signature`），拒绝空段或超过 3 段的畸形令牌；
   - `decodeBase64Url` 采用 RFC 4648 URL-safe 字符表（`-`/`_`，无 `=` 填充要求），遇到非法字符或输出缓冲不足立即返回失败。
2. **时间声明校验 (`exp` / `nbf` / `iat` 与 `ClaimStatus`)**：
   - `JwtClaims` 对过期时间 `exp`、生效时间 `nbf` 与签发时间 `iat` 执行确定性时钟边界判定，支持时钟偏移容忍度（leeway），明确区分合法、过期与尚未生效状态。
3. **签名验证解耦契约**：
   - 本模块专注于零堆分配 JWT 头部/载荷解析与声明校验，不隐式信任未验签载荷；生产鉴权须结合 `cap:crypto`（如 HMAC-SHA256 / Ed25519 / RSA）对 `header.payload` 原始字节与 `signature` 执行密码学验签后再采信 `JwtClaims`，并警惕 `alg: "none"` 降级。
