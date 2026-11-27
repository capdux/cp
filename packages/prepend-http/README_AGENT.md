# prepend-http@1.0.0 (Agent Quickref)

> Zero-allocation utility to prepend https:// or http:// to humanized URLs (npm prepend-http v4 parity) for CP | License: MIT
> Import: `import { ... } from "prepend-http";` | Full types: `src/index.d.cp`

## Functions
- `export fn phtIsSchemeChar(c: u8): bool;`
- `export fn isProtocolRelative(url: string): bool;`
- `export fn hasScheme(url: string): bool;`
- `export fn prependHttp(url: string, https: bool, out: []u8): usize;`
