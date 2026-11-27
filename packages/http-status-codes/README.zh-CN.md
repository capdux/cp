[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http-status-codes

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional phrase lookup, and semantic predicates for CP

## 安装与导入

```bash
cap add http-status-codes
```

```cp
import { reasonPhrase } from "http-status-codes";
```

## 核心导出 API

- `export enum StatusClass`
- `export fn reasonPhrase(code: u16): string;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn isSep(c: u8): bool;`
- `export fn eqNormalizedPhrase(input: string, target: string): bool;`
- `export fn codeFromPhrase(phrase: string): u16?;`
- `export fn statusClass(code: u16): StatusClass;`
- `export fn isValidStatus(code: u16): bool;`
- `export fn isKnownStatus(code: u16): bool;`
- `export fn isInformational(code: u16): bool;`
- `export fn isSuccess(code: u16): bool;`
- `export fn isRedirect(code: u16): bool;`
- `export fn isClientError(code: u16): bool;`
- `export fn isServerError(code: u16): bool;`
- `export fn isEmptyBody(code: u16): bool;`
- `export fn isRetryable(code: u16): bool;`
- `export fn isCacheableByDefault(code: u16): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`http-status-codes@1.0.0`)，许可证：MIT
