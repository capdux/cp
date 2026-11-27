[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/basic-auth

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official HTTP Basic Auth (RFC 7617) and X-API-Key constant-time authentication plugin for cap:web

## 安装与导入

```bash
cap add @web/basic-auth
```

```cp
import { baConstTimeEql } from "@web/basic-auth";
```

## 核心导出 API

- `export fn baConstTimeEql(a: string, b: string): bool;`
- `export fn baDecodeVal(c: u8): u8?;`
- `export fn basicAuthDecodeBase64(input: string, out_buf: []u8): usize?;`
- `export fn baStartsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn basicAuthHook(ctx: &mut Context): bool;`
- `export fn createBasicAuthPlugin(cfg: BasicAuthConfig): Plugin;`
- `export fn registerBasicAuth(app: &mut App, cfg: BasicAuthConfig): &mut App;`
