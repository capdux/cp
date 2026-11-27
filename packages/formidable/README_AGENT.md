# formidable@1.0.0 (Agent Quickref)

> Zero-heap pure CP multipart/form-data parser for fields and file uploads (clean-room MIT implementation of npm formidable v3) | License: MIT
> Import: `import { ... } from "formidable";` | Full types: `src/index.d.cp`
> Deps: `content-disposition`, `mediatype`

## Functions
- `export fn frmToLower(b: u8): u8;`
- `export fn frmIsOws(b: u8): bool;`
- `export fn frmIsTokenChar(b: u8): bool;`
- `export fn frmStrEq(a: string, b: string): bool;`
- `export fn frmEqIgnoreCase(a: string, b: string): bool;`
- `export fn frmTrimOws(s: string): string;`
- `export fn frmSkipOws(s: string, start: usize): usize;`
- `export fn frmMatchesAt(hay: string, pos: usize, needle: string): bool;`
- `export fn frmIsBoundaryTerminator(body: string, after_pos: usize): bool;`
- `export fn frmFindFirstBoundary(body: string, boundary: string): usize;`
- `export fn frmFindNextBoundary(body: string, start: usize, boundary: string): usize;`
- `export fn frmExtractDispositionName(header: string): string;`
- `export fn frmHasFilenameParam(header: string): bool;`
- `export fn parseMultipartForm(content_type: string, body: string, opts: FormidableOptions): ParsedForm;`
