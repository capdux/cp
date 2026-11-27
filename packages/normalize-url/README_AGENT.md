# normalize-url@1.0.0 (Agent Quickref)

> Zero-allocation URL normalization utility (npm normalize-url parity) composing @cap/prepend-http for CP | License: MIT
> Import: `import { ... } from "normalize-url";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultNormalizeUrlOptions(): NormalizeUrlOptions;`
- `export fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn nurlWriteLower(out: []u8, pos: usize, s: string): usize;`
- `export fn nurlFindSchemeSep(s: string): usize;`
- `export fn nurlIsDefaultPort(scheme: string, port: string): bool;`
- `export fn nurlNormalizePath(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize;`
- `export fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize;`
- `export fn normalizeUrl(url: string, out: []u8): usize;`
