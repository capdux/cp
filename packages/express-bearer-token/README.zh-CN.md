[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# express-bearer-token

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6750 HTTP Bearer token extractor, JWT structure precheck, constant-time verifier, and WWW-Authenticate challenge formatter in pure CP

## 安装与导入

```bash
cap add express-bearer-token
```

```cp
import { isAuthSpace } from "express-bearer-token";
```

## 核心导出 API

- `export fn isAuthSpace(b: u8): bool;`
- `export fn isB64TokenCore(b: u8): bool;`
- `export fn isValidTokenChars(token: string): bool;`
- `export fn extractBearer(header: string): string;`
- `export fn formatBearer(token: string, out: []u8): usize;`
- `export fn isBase64UrlChar(b: u8): bool;`
- `export fn isJwtLike(token: string): bool;`
- `export fn constantTimeEq(a: string, b: string): bool;`
- `export fn verifyBearer(header: string, expected_token: string): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushQuotedParam(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;`
- `export fn formatChallenge(realm: string, err_code: string, err_desc: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`express-bearer-token@1.0.0`)，许可证：MIT
