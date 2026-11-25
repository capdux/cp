# Official `@web/*` Plugin Ecosystem for `cap:web` (25 Production Packages)

Production-ready, zero-GC, pure-CP plugins fusing the best of **ElysiaJS**, **Fastify**, and **Axum (`tower-http`)**. Every package lives under `packages/@web/<name>` with its own `cap.json`, zero external C runtime requirements, unique collision-free entry names, and 4-Gate `0/0/0/0` compliance.

## Complete Official Plugin Matrix (25 Packages)

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
| **`@web/env`** | `packages/@web/env` | `registerEnv` | Startup environment schema validation (required/default/int bounds/bool/enum) + App state injection |
| **`@web/i18n`** | `packages/@web/i18n` | `registerI18n` | `?lang=` > Cookie > `Accept-Language` negotiation, `Content-Language` header & zero-heap catalog |
| **`@web/oauth2`** | `packages/@web/oauth2` | `registerOAuth2` | OAuth2 / OIDC PKCE S256 authorize redirect, one-time anti-CSRF `state` & callback verification |
| **`@web/graphql`** | `packages/@web/graphql` | `registerGraphQL` | `POST /graphql` query parser + depth guard + resolver dispatch & `GET /graphql` Playground |
| **`@web/mcp`** | `packages/@web/mcp` | `registerMcp` | Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server (`initialize`/`tools/list`/`tools/call`) |
| **`@web/redis`** | `packages/@web/redis` | `registerRedis` | Thin Redis lifecycle adapter: namespaced keys, RESP2/3 encoder, App state & `/_redis/ping` |
| **`@web/db`** | `packages/@web/db` | `registerDb` | Thin SQLite/Postgres/MySQL lifecycle adapter: auto-TX (2xx COMMIT / 4xx-5xx ROLLBACK) & `X-DB-Tx` |
| **`@web/s3`** | `packages/@web/s3` | `registerS3` | Safe S3 object key normalization + pure-CP AWS SigV4 presigned URL endpoint (`/upload/presign`) |
