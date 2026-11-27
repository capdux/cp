# humanize-url@1.0.0 (Agent Quickref)

> Zero-allocation URL humanizer stripping protocol, www., and trailing slash (npm humanize-url v3 parity) for CP | License: MIT
> Import: `import { ... } from "humanize-url";` | Full types: `src/index.d.cp`

## Functions
- `export fn hurlIsSpace(c: u8): bool;`
- `export fn hurlTrim(s: string): string;`
- `export fn hurlToLowerAscii(c: u8): u8;`
- `export fn hurlStartsWithLower(s: string, lower_prefix: string): bool;`
- `export fn hurlHasDot(s: string): bool;`
- `export fn humanizeUrlSlice(url: string): string;`
- `export fn humanizeUrl(url: string, out: []u8): usize;`
