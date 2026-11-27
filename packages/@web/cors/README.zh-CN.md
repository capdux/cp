[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/cors

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official CORS middleware plugin for cap:web (preflight OPTIONS, origin allowlist, credentials, exposed headers)

## 安装与导入

```bash
cap add @web/cors
```

```cp
import { corsHasCtlOrCrlf } from "@web/cors";
```

## 核心导出 API

- `export fn corsHasCtlOrCrlf(s: string): bool;`
- `export fn corsBytesEql(a: string, b: string): bool;`
- `export fn isOriginAllowed(allow_origin: string, origin: string): bool;`
- `export fn applyCorsHeaders(ctx: &mut Context, origin: string);`
- `export fn corsOnRequest(ctx: &mut Context): bool;`
- `export fn createCorsPlugin(cfg: CorsConfig): Plugin;`
- `export fn registerCors(app: &mut App, cfg: CorsConfig): &mut App;`
