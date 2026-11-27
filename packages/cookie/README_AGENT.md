# cookie@1.0.0 (Agent Quickref)

> Zero-allocation Rust-style CookieBuilder, percent-encoded Set-Cookie helpers, and removal cookie generator for CP. | License: MIT
> Import: `import { ... } from "cookie";` | Full types: `src/index.d.cp`
> Deps: `percent-encoding`, `tough-cookie`

## Functions
- `export fn ckcDefaultCookie(name: string, value: string): Cookie;`
- `export fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta;`
- `export fn formatEncodedCookie(c: Cookie, out: []u8): usize;`
- `export fn makeRemovalCookie(name: string, path: string, out: []u8): usize;`
