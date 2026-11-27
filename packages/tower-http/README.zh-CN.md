[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tower-http

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP (compatible with npm cors & Rust tower-http::cors)

## 安装与导入

```bash
cap add tower-http
```

```cp
import { toLower } from "tower-http";
```

## 核心导出 API

- `export fn toLower(c: u8): u8;`
- `export fn isSpace(c: u8): bool;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn trimSpaces(s: string): string;`
- `export fn findSchemeSep(s: string): i64;`
- `export fn isValidOrigin(origin: string): bool;`
- `export fn matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool;`
- `export fn matchOriginList(origin: string, patterns: &[]string, allow_credentials: bool): bool;`
- `export fn isMethodAllowed(method: string, allowed_methods: string): bool;`
- `export fn areHeadersAllowed(req_headers: string, allowed_headers: string): bool;`
- `export fn isSimpleMethod(method: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tower-http@1.0.0`)，许可证：MIT
