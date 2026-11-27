[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tough-cookie

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 6265 HTTP Cookie, Set-Cookie parser, serializer, and domain/path matching jar (clean-room port of npm cookie + tough-cookie)

## 安装与导入

```bash
cap add tough-cookie
```

```cp
import { parseSetCookie } from "tough-cookie";
```

## 核心导出 API

- `export fn parseSetCookie(header: string): Cookie;`
- `export fn serializeSetCookie(c: Cookie, out: []u8): usize;`
- `export fn getCookieHeaderValue(header: string, name: string): string?;`
- `export fn domainMatches(host: string, cookie_domain: string): bool;`
- `export fn pathMatches(req_path: string, cookie_path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`tough-cookie@1.0.0`)，许可证：BSD-3-Clause
- **上游生态**：`crates.io` (`cookie_store@1.0.0`)，许可证：MIT
