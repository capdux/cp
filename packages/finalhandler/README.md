[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# finalhandler

Zero-heap HTTP final response & error handler with XSS-safe HTML escaping, default security headers (`CSP`, `X-Content-Type-Options`), and `HEAD`/environment-aware response building in pure CP.

- **Upstream Alignment**: Clean-room port of npm [`finalhandler`](https://github.com/pillarjs/finalhandler) (`MIT`).
- **Package Version**: `1.0.0` (depends on `escape-html`, `http`, `itoa`).

## Installation & Import

```bash
cap add finalhandler
```

In `cap.json`:

```json
{
  "dependencies": {
    "finalhandler": "^1.0.0"
  }
}
```

```typescript
import {
    FinalEnv, ResponseHeader, FinalError, FinalHandlerOptions, FinalResponse,
    normalizeErrorStatus, statusCode, getErrorStatusCode, statusReason,
    escapeHtmlText, formatErrorMessage, formatErrorHtml, createHtmlDocument,
    defaultSecurityHeaders, formatCannotMessage,
    buildFinalHttpResponse, buildFinalHttpResponseWithOptions
} from "finalhandler";
```

## Core API Reference

### Enum & Data Structures

| Symbol | Fields / Variants | Description |
| :--- | :--- | :--- |
| `FinalEnv` | `Development`, `Production` | Controls whether error details/stacks are exposed in HTML bodies |
| `ResponseHeader` | `name: string`, `value: string` | HTTP response header pair |
| `FinalError` | `status: u16`, `status_code: u16`, `expose: bool`, `message: string`, `stack: string` | HTTP error descriptor |
| `FinalHandlerOptions` | `env: FinalEnv`, `is_head: bool`, `method: string`, `url: string`; `FinalHandlerOptions.default()` | Options for `buildFinalHttpResponseWithOptions` |
| `FinalResponse` | `status: u16`, `body_len: usize`, `total_len: usize` | Metadata for the serialized HTTP/1.1 response written into `out` |

### Core Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `normalizeErrorStatus` | `normalizeErrorStatus(code: u16): u16` | Clamp non-4xx/5xx codes to `500` |
| `statusCode` / `getErrorStatusCode` | `statusCode(err: FinalError): u16` | Resolve normalized status from `err.status` or `err.status_code` |
| `statusReason` | `statusReason(code: u16): string` | Standard HTTP reason phrase (e.g. `404` -> `"Not Found"`) |
| `escapeHtmlText` | `escapeHtmlText(input: string, out: []u8): usize` | XSS-safe HTML entity escaping into `out` |
| `formatErrorMessage` | `formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string` | Choose message/stack in `Development` or safe status reason in `Production` |
| `formatErrorHtml` / `createHtmlDocument` | `createHtmlDocument(message: string, out: []u8): usize` | Render escaped HTML5 error page (`<!DOCTYPE html>...<pre>...</pre>...`) |
| `defaultSecurityHeaders` | `defaultSecurityHeaders(out: []ResponseHeader): usize` | Populate default security headers (`Content-Security-Policy`, `X-Content-Type-Options`, etc.) |
| `formatCannotMessage` | `formatCannotMessage(method: string, url: string, out: []u8): usize` | Format `"Cannot <METHOD> <URL>"` for default 404 responses |
| `buildFinalHttpResponse` | `buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse` | Build complete HTTP/1.1 response with security headers (omits body when `is_head == true`) |
| `buildFinalHttpResponseWithOptions` | `buildFinalHttpResponseWithOptions(err: FinalError?, opts: FinalHandlerOptions, extra_headers: &[]ResponseHeader, out: []u8): FinalResponse` | Full handler: 404 when `err == null`, otherwise normalized error response with `extra_headers` |

## Examples

### 1. Building a 404 or Error HTTP/1.1 Response

```typescript
import {
    FinalEnv, FinalError, FinalHandlerOptions, ResponseHeader,
    buildFinalHttpResponseWithOptions
} from "finalhandler";

fn renderError(out: []u8): usize {
    let mut opts = FinalHandlerOptions.default();
    opts.env = FinalEnv.Production;
    opts.method = "GET";
    opts.url = "/secret";

    let err = FinalError {
        status: 403,
        status_code: 0,
        expose: true,
        message: "Access <denied>",
        stack: "internal stack trace"
    };
    let extra = [ResponseHeader { name: "X-Request-Id", value: "req-42" }];
    let res = buildFinalHttpResponseWithOptions(err, opts, &extra, out);
    return res.total_len;
}
```

### 2. Default 404 Response (`err == null`) for `HEAD` Requests

```typescript
import { FinalHandlerOptions, ResponseHeader, buildFinalHttpResponseWithOptions } from "finalhandler";

fn handleNotFoundHead(out: []u8): u16 {
    let mut opts = FinalHandlerOptions.default();
    opts.is_head = true;
    opts.method = "HEAD";
    opts.url = "/missing";

    let empty: [0]ResponseHeader = [];
    let res = buildFinalHttpResponseWithOptions(null, opts, &empty, out);
    // res.status == 404, Content-Length preserved, body omitted
    return res.status;
}
```

## Differences from Upstream & CP Highlights

- **Zero-Heap HTTP Serialization**: Writes the full HTTP/1.1 status line, security headers, `Content-Length`, and escaped HTML document directly into caller-owned `out: []u8`.
- **XSS-Safe by Construction**: Delegates HTML escaping to `escape-html` and injects `Content-Security-Policy: default-src 'none'` and `X-Content-Type-Options: nosniff` automatically.
- **Explicit Optional Error**: Passes `err: FinalError?` (`null` triggers `404 Cannot <METHOD> <URL>`) instead of untyped JS callbacks.
