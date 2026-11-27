# busboy@1.0.0 (Agent Quickref)

> Zero-heap RFC 7578 multipart/form-data boundary extractor, part iterator, and Content-Disposition field/filename parser. | License: MIT
> Import: `import { ... } from "busboy";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn isBchar(c: u8): bool;`
- `export fn isValidBoundary(boundary: string): bool;`
- `export fn extractBoundary(content_type: string): string;`
- `export fn matchBoundaryAt(payload: string, pos: usize, boundary: string): bool;`
- `export fn extractDispositionParam(disp: string, key: string): string;`
- `export fn nextPart(payload: string, boundary: string, start: usize): MultipartPart;`
- `export fn countParts(payload: string, boundary: string): usize;`
