# @web/cors@1.0.0 (Agent Quickref)

> Official CORS middleware plugin for cap:web (preflight OPTIONS, origin allowlist, credentials, exposed headers) | License: MIT
> Import: `import { ... } from "@web/cors";` | Full types: `src/index.d.cp`

## Functions
- `export fn corsHasCtlOrCrlf(s: string): bool;`
- `export fn corsBytesEql(a: string, b: string): bool;`
- `export fn isOriginAllowed(allow_origin: string, origin: string): bool;`
- `export fn applyCorsHeaders(ctx: &mut Context, origin: string);`
- `export fn corsOnRequest(ctx: &mut Context): bool;`
- `export fn createCorsPlugin(cfg: CorsConfig): Plugin;`
- `export fn registerCors(app: &mut App, cfg: CorsConfig): &mut App;`
