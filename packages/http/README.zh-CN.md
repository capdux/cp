[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types (crates.io http v1 parity) composing unicase for CP.

## 安装与导入

```bash
cap add http
```

```cp
import { parseMethod } from "http";
```

## 核心导出 API

- `export fn parseMethod(s: string): u8;`
- `export fn methodAsStr(m: u8): string;`
- `export fn isSafeMethod(m: u8): bool;`
- `export fn isIdempotentMethod(m: u8): bool;`
- `export fn parseHttpVersion(s: string): u8;`
- `export fn httpVersionAsStr(v: u8): string;`
- `export fn isValidHeaderName(name: string): bool;`
- `export fn headerNameEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`http@1.0.0`)，许可证：MIT
