[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/sensible

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official RFC 9457 Problem Details & standard HTTP error helpers plugin for cap:web (inspired by @fastify/sensible)

## 安装与导入

```bash
cap add @web/sensible
```

```cp
import { sensWriteRaw } from "@web/sensible";
```

## 核心导出 API

- `export fn sensWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;`
- `export fn sensWriteEscaped(dst: []u8, pos: &mut usize, s: string): bool;`
- `export fn sensWriteU16(dst: []u8, pos: &mut usize, v: u16): bool;`
- `export fn problemResponse(status_code: u16, title: string, detail: string): Response;`
- `export fn httpBadRequest(detail: string): Response;`
- `export fn httpUnauthorized(detail: string): Response;`
- `export fn httpForbidden(detail: string): Response;`
- `export fn httpNotFound(detail: string): Response;`
- `export fn httpConflict(detail: string): Response;`
- `export fn httpUnprocessable(detail: string): Response;`
- `export fn httpTooManyRequests(detail: string): Response;`
- `export fn httpInternalError(detail: string): Response;`
- `export fn sensibleAssert(ctx: &mut Context, cond: bool, status_code: u16, title: string, detail: string): bool;`
- `export fn sensibleAfterHook(ctx: &mut Context, resp: &mut Response);`
- `export fn createSensiblePlugin(cfg: SensibleConfig): Plugin;`
- `export fn registerSensible(app: &mut App, cfg: SensibleConfig): &mut App;`
