# mime_guess@1.0.0 (Agent Quickref)

> Zero-allocation file path and extension MIME type guesser for CP (clean-room MIT, inspired by Rust mime_guess v2, powered by @cap/mime-types). | License: MIT
> Import: `import { ... } from "mime_guess";` | Full types: `src/index.d.cp`
> Deps: `mime-types`

## Functions
- `export fn mgsStartsWith(s: string, prefix: string): bool;`
- `export fn mgsHasPathSep(s: string): bool;`
- `export fn mgsIsCompressibleMime(mime_type: string): bool;`
- `export fn extractExt(path: string): string?;`
- `export fn getExtForMime(mime_type: string): string?;`
- `export fn getCharsetForMime(mime_type: string): string?;`
