# finalhandler@1.0.0 (Agent Quickref)

> Zero-heap HTTP final response & error handler with XSS-safe HTML formatting and security headers (clean-room port of npm finalhandler) | License: MIT
> Import: `import { ... } from "finalhandler";` | Full types: `src/index.d.cp`
> Deps: `escape-html`, `http`, `itoa`

## Functions
- `export fn fhPush(out: []u8, pos: usize, s: string): usize;`
- `export fn fhPushU64(out: []u8, pos: usize, v: u64): usize;`
- `export fn normalizeErrorStatus(code: u16): u16;`
- `export fn statusReason(code: u16): string;`
- `export fn formatErrorHtml(message: string, out: []u8): usize;`
- `export fn formatCannotMessage(method: string, url: string, out: []u8): usize;`
- `export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;`
