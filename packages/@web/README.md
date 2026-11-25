# Official `@web/*` Plugin Ecosystem for `cap:web`

Production-ready, zero-GC, pure-CP plugins for the built-in `cap:web` framework. Every plugin is packaged as an independent `@web/<name>` workspace package with its own `cap.json`, zero external C runtime requirements, and 4-Gate `0/0/0/0` compliance.

## Installed Official Plugins (P0 Core Suite)

| Package | Path | Purpose |
| :--- | :--- | :--- |
| **`@web/cors`** | `packages/@web/cors` | W3C Fetch CORS: `OPTIONS` 204 preflight, exact/wildcard Origin allowlist, credentials, CRLF defense |
| **`@web/security`** | `packages/@web/security` | Helmet headers (`HSTS`/`CSP`/`X-Frame-Options`/`nosniff`/`Referrer-Policy`) + constant-time Double-Submit Cookie CSRF |
| **`@web/jwt`** | `packages/@web/jwt` | Zero-heap HS256 JWT sign/verify (pure-CP HMAC-SHA256 + Base64URL) + `Authorization: Bearer` auth middleware |
| **`@web/ratelimit`** | `packages/@web/ratelimit` | Fixed-slot IP/Header rate limiter with `RateLimit-Limit` / `RateLimit-Remaining` / `Retry-After` & `429` |
| **`@web/logger`** | `packages/@web/logger` | `X-Request-Id` & W3C `traceparent`, structured JSON access log formatting, and `/metrics` OpenMetrics endpoint |
| **`@web/static`** | `packages/@web/static` | Static directory serving with path traversal defense, extension + `cap:mime/fileType` sniffing, ETag / `304` |
| **`@web/swagger`** | `packages/@web/swagger` | Automatic OpenAPI 3.1.0 `GET /openapi.json` + interactive Scalar/Swagger UI `GET /docs` |

## Usage

```typescript
import { App, Context, Response } from "cap:web";
import { CorsConfig, registerCors } from "@web/cors";
import { SecurityConfig, registerSecurity } from "@web/security";
import { JwtConfig, registerJwt } from "@web/jwt";
import { RateLimitConfig, registerRateLimit } from "@web/ratelimit";
import { LoggerConfig, registerLogger } from "@web/logger";
import { StaticConfig, registerStatic } from "@web/static";
import { SwaggerConfig, registerSwagger } from "@web/swagger";

let app = App.init();
* = registerCors(*app, CorsConfig{ allow_origin: "https://app.cp.dev", allow_credentials: true });
* = registerSecurity(*app, SecurityConfig{ enable_csrf: false });
* = registerRateLimit(*app, RateLimitConfig{ max_requests: 1000, window_secs: 60 });
* = registerLogger(*app, LoggerConfig{ enable_trace: true });
* = registerJwt(*app, JwtConfig{ secret: "my-secret", optional_auth: true });
* = registerStatic(*app, StaticConfig{ root_dir: "./public", url_prefix: "/static" });
* = registerSwagger(*app, SwaggerConfig{ title: "Production API", version: "1.0.0" });
```
