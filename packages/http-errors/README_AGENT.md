# http-errors — AI Agent Quick Ref

Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 application/problem+json serializer for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HttpError,
    statusTitle,
    errorClassName,
    statusCodeName,
    createHttpError,
    badRequest,
    unauthorized,
    forbidden,
} from "http-errors";
```

## Core Types & Constants

```cp
class HttpError { status: u16, code: string, title: string, detail: string, /* +2 fields */ } // methods: withInstance(...) +2
```

## API Signatures

```cp
fn statusTitle(status: u16): string;
fn errorClassName(status: u16, out: []u8): usize;
fn statusCodeName(status: u16): string;
fn createHttpError(status: u16, detail: string): HttpError;
fn badRequest(detail: string): HttpError;
fn unauthorized(detail: string): HttpError;
fn forbidden(detail: string): HttpError;
fn notFound(detail: string): HttpError;
fn methodNotAllowed(detail: string): HttpError;
fn conflict(detail: string): HttpError;
fn gone(detail: string): HttpError;
fn payloadTooLarge(detail: string): HttpError;
fn unsupportedMediaType(detail: string): HttpError;
fn unprocessableEntity(detail: string): HttpError;
fn tooManyRequests(detail: string): HttpError;
fn internalServerError(detail: string): HttpError;
fn notImplemented(detail: string): HttpError;
fn badGateway(detail: string): HttpError;
fn serviceUnavailable(detail: string): HttpError;
fn gatewayTimeout(detail: string): HttpError;
fn isClientError(err: HttpError): bool;
fn isServerError(err: HttpError): bool;
fn isRetryable(err: HttpError): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn hexDigitLower(v: u8): u8;
fn appendJsonEscaped(out: []u8, start_pos: usize, s: string): usize;
fn appendU16(out: []u8, pos: usize, val: u16): usize;
fn toProblemJson(err: HttpError, include_hidden_detail: bool, out: []u8): string;
```

## Minimal Usage

```cp
import { createHttpError } from "http-errors";

pub fn main() {
    let result = createHttpError(0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http-errors"`; install via `cap add http-errors`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
