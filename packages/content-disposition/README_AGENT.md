# content-disposition@1.0.0 (Agent Quickref)

> Zero-allocation RFC 6266 / RFC 5987 HTTP Content-Disposition header parser, formatter, filename* decoder, and filename sanitizer for CP | License: MIT
> Import: `import { ... } from "content-disposition";` | Full types: `src/index.d.cp`
> Deps: `percent-encoding`, `unicase`

## Functions
- `export fn parseContentDisposition(header: string): ContentDisposition;`
- `export fn sanitizeFilename(raw: string, out: []u8): usize;`
- `export fn decodeFilenameStar(raw_star: string, out: []u8): usize;`
- `export fn formatContentDisposition(disp_type: string, filename: string, out: []u8): usize;`
