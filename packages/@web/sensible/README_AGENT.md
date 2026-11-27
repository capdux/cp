# @web/sensible@1.0.0 (Agent Quickref)

> Official RFC 9457 Problem Details & standard HTTP error helpers plugin for cap:web (inspired by @fastify/sensible) | License: MIT
> Import: `import { ... } from "@web/sensible";` | Full types: `src/index.d.cp`

## Functions
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
