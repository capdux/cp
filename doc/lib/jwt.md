# `cap:jwt` — RFC 7519 JWT Claims Zero-Allocation Parser

Zero-allocation, pure-CP RFC 7519 JSON Web Token (JWT) header & claims parser, Base64URL decoder, and temporal claim (`exp`/`nbf`/`iat`) validator.

## Module Layout

| File | Source | Responsibilities |
| :--- | :--- | :--- |
| `lib/jwt/claims.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 7519 JWT Claims Barrel)` | `decodeBase64Url`, `JwtParts`, `splitJwt`, `extractStringClaim`, `extractIntClaim`, `JwtHeader`, `ClaimStatus`, `JwtClaims` |
| `lib/jwt/index.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 7519 JWT Claims Barrel)` | Unified `cap:jwt` exports |
