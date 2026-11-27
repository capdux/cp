# content-type@1.0.0 (Agent Quickref)

> Zero-heap RFC 7231 HTTP Content-Type header parser and formatter with quoted-string unescaping (clean-room port of content-type) | License: MIT
> Import: `import { ... } from "content-type";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn ctypIsOws(c: u8): bool;`
- `export fn ctypIsTokenChar(c: u8): bool;`
- `export fn ctypIsValidToken(s: string): bool;`
- `export fn ctypIsQuotedAllowed(c: u8): bool;`
- `export fn ctypWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn ctypWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn isValidMediaType(s: string): bool;`
- `export fn unescapeQuotedString(raw: string, out: []u8): usize;`
- `export fn parseContentType(header: string): ContentType;`
- `export fn formatContentType(media_type: string, params: &[]ContentTypeParam, out: []u8): usize;`
