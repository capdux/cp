# serve-static@1.0.0 (Agent Quickref)

> Zero-heap HTTP static file request resolver, dotfile policy evaluator, Cache-Control builder, and MIME/ETag helper (clean-room port of npm serve-static) | License: MIT
> Import: `import { ... } from "serve-static";` | Full types: `src/index.d.cp`
> Deps: `etag`, `itoa`, `mime-types`, `parseurl`

## Functions
- `export fn sstPush(out: []u8, pos: usize, s: string): usize;`
- `export fn sstIsGetOrHead(method: string): bool;`
- `export fn hasDotfileSegment(pathname: string): bool;`
- `export fn resolveStaticRequest(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution;`
- `export fn formatCacheControl(max_age_secs: u64, immutable: bool, out: []u8): usize;`
- `export fn formatStaticEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize;`
- `export fn formatStaticContentType(file_path: string, out: []u8): usize;`
