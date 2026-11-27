[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/security

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official Helmet security headers (HSTS/CSP/X-Frame-Options/Referrer-Policy) and Double-Submit CSRF plugin for cap:web

## 安装与导入

```bash
cap add @web/security
```

```cp
import { constantTimeEql } from "@web/security";
```

## 核心导出 API

- `export fn constantTimeEql(a: string, b: string): bool;`
- `export fn isSafeHttpMethod(m: AppMethod): bool;`
- `export fn securityOnRequest(ctx: &mut Context): bool;`
- `export fn createSecurityPlugin(cfg: SecurityConfig): Plugin;`
- `export fn registerSecurity(app: &mut App, cfg: SecurityConfig): &mut App;`
