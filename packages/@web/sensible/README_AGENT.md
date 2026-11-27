# @web/sensible — AI Agent Quick Ref

Official RFC 9457 Problem Details & standard HTTP error helpers plugin for cap:web (inspired by @fastify/sensible) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SensibleConfig,
    sensWriteRaw,
    sensWriteEscaped,
    sensWriteU16,
    problemResponse,
    httpBadRequest,
    httpUnauthorized,
    httpForbidden,
} from "@web/sensible";
```

## Core Types & Constants

```cp
class SensibleConfig { expose_problem_type_base: string, default_error_header: string }
```

## API Signatures

```cp
fn sensWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;
fn sensWriteEscaped(dst: []u8, pos: &mut usize, s: string): bool;
fn sensWriteU16(dst: []u8, pos: &mut usize, v: u16): bool;
fn problemResponse(status_code: u16, title: string, detail: string): Response;
fn httpBadRequest(detail: string): Response;
fn httpUnauthorized(detail: string): Response;
fn httpForbidden(detail: string): Response;
fn httpNotFound(detail: string): Response;
fn httpConflict(detail: string): Response;
fn httpUnprocessable(detail: string): Response;
fn httpTooManyRequests(detail: string): Response;
fn httpInternalError(detail: string): Response;
fn sensibleAssert(ctx: &mut Context, cond: bool, status_code: u16, title: string, detail: string): bool;
fn sensibleAfterHook(ctx: &mut Context, resp: &mut Response);
fn createSensiblePlugin(cfg: SensibleConfig): Plugin;
fn registerSensible(app: &mut App, cfg: SensibleConfig): &mut App;
```

## Minimal Usage

```cp
import { sensWriteRaw } from "@web/sensible";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sensWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/sensible"`; install via `cap add @web/sensible`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
