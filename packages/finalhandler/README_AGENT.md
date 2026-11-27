# finalhandler — AI Agent Quick Ref

Zero-heap HTTP/1.1 final 404/error response builder with XSS-safe HTML escaping and default security headers.

## Import
```typescript
import {
    FinalEnv, ResponseHeader, FinalError, FinalHandlerOptions, FinalResponse,
    normalizeErrorStatus, statusCode, getErrorStatusCode, statusReason,
    escapeHtmlText, formatErrorMessage, formatErrorHtml, createHtmlDocument,
    defaultSecurityHeaders, formatCannotMessage,
    buildFinalHttpResponse, buildFinalHttpResponseWithOptions
} from "finalhandler";
```

## API Signatures
```typescript
export enum FinalEnv { Development, Production }
export class ResponseHeader { name: string, value: string }
export class FinalError { status: u16, status_code: u16, expose: bool, message: string, stack: string }
export class FinalHandlerOptions {
    env: FinalEnv, is_head: bool, method: string, url: string,
    fn default(): FinalHandlerOptions;
}
export class FinalResponse { status: u16, body_len: usize, total_len: usize }

export fn normalizeErrorStatus(code: u16): u16;
export fn statusCode(err: FinalError): u16;
export fn getErrorStatusCode(err: FinalError): u16;
export fn statusReason(code: u16): string;
export fn escapeHtmlText(input: string, out: []u8): usize;
export fn formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string;
export fn formatErrorHtml(message: string, out: []u8): usize;
export fn createHtmlDocument(message: string, out: []u8): usize;
export fn defaultSecurityHeaders(out: []ResponseHeader): usize;
export fn formatCannotMessage(method: string, url: string, out: []u8): usize;
export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;
export fn buildFinalHttpResponseWithOptions(err: FinalError?, opts: FinalHandlerOptions, extra_headers: &[]ResponseHeader, out: []u8): FinalResponse;
```

## Minimal Example
```typescript
import { FinalHandlerOptions, ResponseHeader, buildFinalHttpResponseWithOptions } from "finalhandler";

fn respond404(out: []u8): usize {
    let mut opts = FinalHandlerOptions.default();
    opts.method = "GET";
    opts.url = "/nope";
    let empty: [0]ResponseHeader = [];
    let res = buildFinalHttpResponseWithOptions(null, opts, &empty, out);
    return res.total_len;
}
```

## Pitfalls & Rules
- Pass `null` as `err` to `buildFinalHttpResponseWithOptions` for default `404 Cannot <METHOD> <URL>`.
- Non-4xx/5xx error statuses normalize to `500`; `Production` hides non-exposed 5xx messages/stacks.
- `extra_headers` is a read-only slice `&[]ResponseHeader`; `out: []u8` is caller-owned (check `res.total_len` for written bytes).
- `is_head: true` keeps `Content-Length` accurate while omitting the HTTP body bytes.
- `statusCode` / `getErrorStatusCode` and `formatErrorHtml` / `createHtmlDocument` are aliases.
