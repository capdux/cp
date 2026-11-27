# normalize-url@1.0.0 (Agent Quickref)

> Zero-allocation URL normalization utility (npm normalize-url parity) composing @cap/prepend-http for CP | License: MIT
> Import: `import { ... } from "normalize-url";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultNormalizeUrlOptions(): NormalizeUrlOptions;`
- `export fn nurlIsSpace(c: u8): bool;`
- `export fn nurlTrim(s: string): string;`
- `export fn nurlToLowerAscii(c: u8): u8;`
- `export fn nurlEqStr(a: string, b: string): bool;`
- `export fn nurlEqLower(a: string, lower_b: string): bool;`
- `export fn nurlStartsWithLower(s: string, lower_prefix: string): bool;`
- `export fn nurlHasDot(s: string): bool;`
- `export fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn nurlWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn nurlWriteLower(out: []u8, pos: usize, s: string): usize;`
- `export fn nurlFindSchemeSep(s: string): usize;`
- `export fn nurlIsDefaultPort(scheme: string, port: string): bool;`
- `export fn nurlNormalizePath(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize;`
- `export fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize;`
- `export fn normalizeUrl(url: string, out: []u8): usize;`
