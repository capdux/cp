# @web/security@1.0.0 (Agent Quickref)

> Official Helmet security headers (HSTS/CSP/X-Frame-Options/Referrer-Policy) and Double-Submit CSRF plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/security";` | Full types: `src/index.d.cp`

## Functions
- `export fn constantTimeEql(a: string, b: string): bool;`
- `export fn isSafeHttpMethod(m: AppMethod): bool;`
- `export fn securityOnRequest(ctx: &mut Context): bool;`
- `export fn createSecurityPlugin(cfg: SecurityConfig): Plugin;`
- `export fn registerSecurity(app: &mut App, cfg: SecurityConfig): &mut App;`
