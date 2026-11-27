# cors@1.0.0 (Agent Quickref)

> Zero-heap Fetch/W3C CORS origin evaluator and preflight/actual response header builder (clean-room port of npm cors) | License: MIT
> Import: `import { ... } from "cors";` | Full types: `src/index.d.cp`
> Deps: `unicase`, `wildmatch`

## Functions
- `export fn corsPush(out: []u8, pos: usize, s: string): usize;`
- `export fn corsPushU32(out: []u8, pos: usize, v: u32): usize;`
- `export fn corsIsWs(b: u8): bool;`
- `export fn corsTrim(s: string): string;`
- `export fn isOriginAllowed(opts: CorsOptions, request_origin: string): bool;`
- `export fn isMethodAllowed(allowed_methods: string, method: string): bool;`
- `export fn evaluateCors(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision;`
- `export fn formatCorsHeaders(d: CorsDecision, out: []u8): usize;`
