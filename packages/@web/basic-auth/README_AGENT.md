# @web/basic-auth@1.0.0 (Agent Quickref)

> Official HTTP Basic Auth (RFC 7617) and X-API-Key constant-time authentication plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/basic-auth";` | Full types: `src/index.d.cp`

## Functions
- `export fn baConstTimeEql(a: string, b: string): bool;`
- `export fn baDecodeVal(c: u8): u8?;`
- `export fn basicAuthDecodeBase64(input: string, out_buf: []u8): usize?;`
- `export fn baStartsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn basicAuthHook(ctx: &mut Context): bool;`
- `export fn createBasicAuthPlugin(cfg: BasicAuthConfig): Plugin;`
- `export fn registerBasicAuth(app: &mut App, cfg: BasicAuthConfig): &mut App;`
