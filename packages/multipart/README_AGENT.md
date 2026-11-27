# multipart@1.0.0 (Agent Quickref)

> Zero-heap RFC 7578 multipart/form-data streaming reader, writer, and boundary extractor built on @cap/mediatype and @cap/unicase (clean-room port of Rust multipart/multer) | License: MIT
> Import: `import { ... } from "multipart";` | Full types: `src/index.d.cp`
> Deps: `mediatype`, `unicase`

## Functions
- `export fn mpartIsSpace(c: u8): bool;`
- `export fn mpartTrim(s: string): string;`
- `export fn mpartMatchAt(haystack: string, pos: usize, needle: string): bool;`
- `export fn mpartIsBoundaryAt(body: string, pos: usize, boundary: string): bool;`
- `export fn mpartFindFirstBoundary(body: string, boundary: string): usize;`
- `export fn mpartFindNextBoundary(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool;`
- `export fn mpartSkipLineEnd(body: string, pos: &mut usize): bool;`
- `export fn mpartParseDisposition(val: string, part: &mut MultipartPart);`
- `export fn mpartWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn extractBoundary(content_type: string): string?;`
- `export fn formatContentTypeHeader(boundary: string, out: []u8): usize;`
