# mediatype@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP. | License: MIT
> Import: `import { ... } from "mediatype";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidToken(s: string): bool;`
- `export fn parseMediaType(input: string, out: &mut MediaType): bool;`
- `export fn getParam(mt: MediaType, name: string, out: []u8): usize;`
- `export fn formatMediaType(mt: MediaType, out: []u8): usize;`
- `export fn isJsonLike(mt: MediaType): bool;`
- `export fn isTextLike(mt: MediaType): bool;`
