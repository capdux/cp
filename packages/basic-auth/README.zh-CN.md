[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# basic-auth

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7617 HTTP Basic Authentication parser, encoder, constant-time verifier, and WWW-Authenticate challenge formatter.

## 安装与导入

```bash
cap add basic-auth
```

```cp
import { isWs } from "basic-auth";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn b64EncChar(v: u8): u8;`
- `export fn b64DecVal(c: u8): i32;`
- `export fn b64Encode(src: &[]u8, dst: []u8): i64;`
- `export fn b64Decode(src: &[]u8, dst: []u8): i64;`
- `export fn decodeHeader(header: string, out_user: []u8, out_pass: []u8): BasicCredentials;`
- `export fn encodeHeader(user: string, pass: string, out: []u8): usize;`
- `export fn constantTimeEq(a: &[]u8, b: &[]u8): bool;`
- `export fn verifyHeader(header: string, expected_user: string, expected_pass: string): bool;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatChallenge(realm: string, utf8: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`basic-auth@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`http-auth-basic@1.0.0`)，许可证：MIT
