[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/oauth2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official OAuth 2.0 + RFC 7636 PKCE authorization plugin for cap:web

## 安装与导入

```bash
cap add @web/oauth2
```

```cp
import { oaBytesEql } from "@web/oauth2";
```

## 核心导出 API

- `export fn oaBytesEql(a: string, b: string): bool;`
- `export fn oaWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn oaWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn oaHexUpper(n: u8): u8;`
- `export fn oaHexLower(n: u8): u8;`
- `export fn oaIsUnreserved(c: u8): bool;`
- `export fn oaWriteUrlEncoded(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn oaWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn oaWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn oaHasQueryMark(s: string): bool;`
- `export fn oauth2BuildAuthorizeUrl(cfg: OAuth2Config, state: string, code_challenge: string, out_buf: []u8): usize?;`
- `export fn oauth2RegisterState(state: string);`
- `export fn oauth2ConsumeState(state: string): bool;`
- `export fn oauth2ResetStates();`
- `export fn oauth2BuildTokenFormBody(cfg: OAuth2Config, code: string, code_verifier: string, out_buf: []u8): usize?;`
- `export fn oaLoginHandler(ctx: &mut Context): Response;`
- `export fn oaCallbackHandler(ctx: &mut Context): Response;`
- `export fn registerOAuth2(app: &mut App, cfg: OAuth2Config): &mut App;`
