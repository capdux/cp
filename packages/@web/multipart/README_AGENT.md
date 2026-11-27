# @web/multipart@1.0.0 (Agent Quickref)

> Official multipart/form-data parser and payload guard plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/multipart";` | Full types: `src/index.d.cp`

## Functions
- `export fn mpAsciiLower(c: u8): u8;`
- `export fn mpStartsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn mpEqlIgnoreCase(a: string, b: string): bool;`
- `export fn mpTrimSpaces(s: string): string;`
- `export fn mpParseUsize(s: string): usize?;`
- `export fn mpMatchAt(haystack: string, pos: usize, needle: string): bool;`
- `export fn mpFindBoundary(body: string, start_pos: usize, boundary: string): usize?;`
- `export fn mpFindHeaderEnd(part: string, body_start_out: &mut usize): usize?;`
- `export fn mpExtractParam(header_val: string, key: string): string?;`
- `export fn parseMultipartBody(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32;`
- `export fn multipartGuardHook(ctx: &mut Context): bool;`
- `export fn createMultipartPlugin(cfg: MultipartConfig): Plugin;`
- `export fn registerMultipart(app: &mut App, cfg: MultipartConfig): &mut App;`
