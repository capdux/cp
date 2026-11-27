[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/jwt

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official zero-heap HS256 JWT authentication plugin for cap:web (pure-CP SHA-256, HMAC-SHA256, Base64URL)

## 安装与导入

```bash
cap add @web/jwt
```

```cp
import { rotr32 } from "@web/jwt";
```

## 核心导出 API

- `export fn rotr32(x: u32, n: u5): u32;`
- `export fn sha256Compress(state: &mut [8]u32, block: &[64]u8);`
- `export fn sha256(data: &[]u8, out: &mut [32]u8);`
- `export fn hmacSha256(key: string, msg: &[]u8, out: &mut [32]u8);`
- `export fn base64UrlEncode(input: &[]u8, out_buf: []u8): usize?;`
- `export fn base64UrlCharVal(ch: u8): u8?;`
- `export fn base64UrlDecode(input: string, out_buf: []u8): usize?;`
- `export fn jwtConstantTimeEql(a: &[]u8, b: &[]u8): bool;`
- `export fn jwtAsciiLower(c: u8): u8;`
- `export fn jwtAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn jwtTrimSpaces(s: string): string;`
- `export fn jwtContainsSubstr(haystack: string, needle: string): bool;`
- `export fn jwtWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn signHs256(secret: string, payload_json: string, out_buf: []u8): usize?;`
- `export fn verifyHs256(secret: string, token: string, out_payload: []u8): usize?;`
- `export fn jwtCheckIssuerClaim(payload: string, issuer: string): bool;`
- `export fn configure(cfg: JwtConfig);`
- `export fn authHook(ctx: &mut Context): bool;`
- `export fn createJwtPlugin(cfg: JwtConfig): Plugin;`
- `export fn registerJwt(app: &mut App, cfg: JwtConfig): &mut App;`
