# Official `@web/*` Plugin Ecosystem for `cap:web` (17 Production Packages)

Production-ready, zero-GC, pure-CP plugins fusing the best of **ElysiaJS**, **Fastify**, and **Axum (`tower-http`)**. Every package lives under `packages/@web/<name>` with its own `cap.json`, zero external C runtime requirements, unique collision-free entry names, and 4-Gate `0/0/0/0` compliance.

## Complete Official Plugin Matrix (17 Packages)

| Package | Path | Entry Function | Purpose |
| :--- | :--- | :--- | :--- |
| **`@web/cors`** | `packages/@web/cors` | `registerCors` | W3C Fetch CORS: `OPTIONS` 204 preflight, Origin allowlist, credentials, CRLF defense |
| **`@web/security`** | `packages/@web/security` | `registerSecurity` | Helmet security headers (`HSTS`/`CSP`/`X-Frame-Options`/`nosniff`) + constant-time Double-Submit CSRF |
| **`@web/jwt`** | `packages/@web/jwt` | `registerJwt` | Zero-heap HS256 JWT sign/verify (pure-CP HMAC-SHA256 + Base64URL) + `Bearer` auth middleware |
| **`@web/ratelimit`** | `packages/@web/ratelimit` | `registerRateLimit` | Fixed-slot IP/Header rate limiter with `RateLimit-*` / `Retry-After` & `429` |
| **`@web/logger`** | `packages/@web/logger` | `registerLogger` | `X-Request-Id` & W3C `traceparent`, structured JSON access logs, OpenMetrics `/metrics` |
| **`@web/static`** | `packages/@web/static` | `registerStatic` | Static directory serving, path traversal `403` defense, `cap:mime/fileType` sniffing, ETag/`304` |
| **`@web/swagger`** | `packages/@web/swagger` | `registerSwagger` | Automatic OpenAPI 3.1.0 `GET /openapi.json` + interactive Scalar/Swagger UI `GET /docs` |
| **`@web/multipart`** | `packages/@web/multipart` | `registerMultipart` | Multipart file upload parsing, file size/count limits (`413`), MIME prefix allowlist (`415`) |
| **`@web/compress`** | `packages/@web/compress` | `registerCompress` | `Accept-Encoding` negotiation + pure-CP RFC 1952 Gzip framing + `Vary: Accept-Encoding` |
| **`@web/cache`** | `packages/@web/cache` | `registerCache` | GET response cache (`X-Cache: HIT/MISS` + TTL) + POST `Idempotency-Key` replay protection |
| **`@web/sensible`** | `packages/@web/sensible` | `registerSensible` | RFC 9457 `application/problem+json` error factories (`httpNotFound`, `httpConflict`, `sensibleAssert`) |
| **`@web/pressure`** | `packages/@web/pressure` | `registerPressure` | Max body size (`413`), concurrency load shedding (`503` + `Retry-After`), K8s `/healthz` & `/readyz` |
| **`@web/basic-auth`** | `packages/@web/basic-auth` | `registerBasicAuth` | RFC 7617 HTTP Basic (`WWW-Authenticate`) + `X-API-Key` constant-time dual-mode auth |
| **`@web/sse`** | `packages/@web/sse` | `registerSse` | Server-Sent Events (`text/event-stream`), `Last-Event-ID` ring-buffer replay, heartbeat & AI streaming |
| **`@web/session`** | `packages/@web/session` | `registerSession` | Signed tamper-proof Cookie Session (`<id>.<sig>`) + fixed-slot KV store + TTL + `HttpOnly`/`SameSite` |
| **`@web/proxy`** | `packages/@web/proxy` | `registerProxy` | HTTP reverse proxy, safe prefix stripping, `X-Forwarded-*` injection & pluggable transport |
| **`@web/cron`** | `packages/@web/cron` | `registerCron` | 5-field Cron parser (`*`/`*/n`/`a-b`/`a,b`), O(1) bitmask matcher, scheduler & `GET /_cron` status |
