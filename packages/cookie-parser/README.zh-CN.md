[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cookie-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP Cookie header parser with HMAC-SHA256 signed cookie verification (s:), key rotation, and JSON cookie (j:) extraction (npm cookie-parser equivalent)

## 安装与导入

```bash
cap add cookie-parser
```

```cp
import { cprStartsWith2 } from "cookie-parser";
```

## 核心导出 API

- `export fn cprStartsWith2(s: string, c0: u8, c1: u8): bool;`
- `export fn isSignedCookie(val: string): bool;`
- `export fn isJsonCookie(val: string): bool;`
- `export fn stripJsonCookiePrefix(val: string): string?;`
- `export fn signCookieValue(val: string, secret: string, out: []u8): usize;`
- `export fn signedCookie(val: string, secret: string): string?;`
- `export fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?;`
- `export fn lookupRawCookie(header: string, name: string): string?;`
- `export fn cprParseInto(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;`
- `export fn parseCookieHeader(header: string, secret: string): ParsedCookies;`
- `export fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cookie-parser@1.0.0`)，许可证：MIT
