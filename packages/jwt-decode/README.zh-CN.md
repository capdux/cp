[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jwt-decode

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7519 JWT segment splitter, Base64URL decoder, exp/nbf leeway validator, and alg:none guard in pure CP

## 安装与导入

```bash
cap add jwt-decode
```

```cp
import { invalidParts } from "jwt-decode";
```

## 核心导出 API

- `export fn invalidParts(): JwtParts;`
- `export fn b64UrlVal(b: u8): i32;`
- `export fn splitToken(token: string): JwtParts;`
- `export fn decodeBase64Url(input: string, out: []u8): i64;`
- `export fn decodeHeader(token: string, out: []u8): i64;`
- `export fn decodePayload(token: string, out: []u8): i64;`
- `export fn isExpired(exp_unix: i64, now_unix: i64, leeway_secs: i64): bool;`
- `export fn isNotYetValid(nbf_unix: i64, now_unix: i64, leeway_secs: i64): bool;`
- `export fn isJsonSpace(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn isAlgNone(header_json: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jwt-decode@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`jsonwebtoken@1.0.0`)，许可证：MIT
