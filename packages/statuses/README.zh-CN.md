[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# statuses

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 9110 / IANA HTTP status code lookup, reason phrases, reverse lookup, and category predicates (npm statuses parity).

## 安装与导入

```bash
cap add statuses
```

```cp
import { toLowerAscii } from "statuses";
```

## 核心导出 API

- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn message(code: u16): string;`
- `export fn codeFromMessage(msg: string): u16;`
- `export fn isValid(code: u16): bool;`
- `export fn isInformational(code: u16): bool;`
- `export fn isSuccess(code: u16): bool;`
- `export fn isRedirect(code: u16): bool;`
- `export fn isEmpty(code: u16): bool;`
- `export fn isRetry(code: u16): bool;`
- `export fn isClientError(code: u16): bool;`
- `export fn isServerError(code: u16): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`statuses@1.0.0`)，许可证：MIT
