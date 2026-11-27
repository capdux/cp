# finalhandler@1.0.0 (Agent Quickref)

> Zero-heap HTTP final response & error handler with XSS-safe HTML formatting and security headers (clean-room port of npm finalhandler) | License: MIT
> Import: `import { ... } from "finalhandler";` | Full types: `src/index.d.cp`
> Deps: `escape-html`, `http`, `itoa`

## Types & Constants
- `export enum FinalEnv`

## Functions
- `export fn fhPush(out: []u8, pos: usize, s: string): usize;`
- `export fn fhPushU64(out: []u8, pos: usize, v: u64): usize;`
- `export fn normalizeErrorStatus(code: u16): u16;`
- `export fn statusCode(err: FinalError): u16;`
- `export fn getErrorStatusCode(err: FinalError): u16;`
- `export fn statusReason(code: u16): string;`
- `export fn escapeHtmlText(input: string, out: []u8): usize;`
- `export fn formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string;`
- `export fn formatErrorHtml(message: string, out: []u8): usize;`
- `export fn createHtmlDocument(message: string, out: []u8): usize;`
- `export fn defaultSecurityHeaders(out: []ResponseHeader): usize;`
- `export fn formatCannotMessage(method: string, url: string, out: []u8): usize;`
- `export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;`
- `export fn buildFinalHttpResponseWithOptions(err: FinalError?, opts: FinalHandlerOptions, extra_headers: &[]ResponseHeader, out: []u8): FinalResponse;`
