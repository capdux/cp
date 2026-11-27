[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http-errors

Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 application/problem+json serializer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:http-errors` ([https://github.com/jshttp/http-errors](https://github.com/jshttp/http-errors))

| Metadata | Value |
| --- | --- |
| Package | `http-errors` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http-errors
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HttpError` | `class` | Fields: `status: u16, code: string, title: string, detail: string, instance: string, expose: bool` · Methods: `withInstance, withExpose, withCode` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `statusTitle` | `(status: u16): string` | Invokes `statusTitle` and returns `string`. |
| `errorClassName` | `(status: u16, out: []u8): usize` | Invokes `errorClassName` and returns `usize`. |
| `statusCodeName` | `(status: u16): string` | Invokes `statusCodeName` and returns `string`. |
| `createHttpError` | `(status: u16, detail: string): HttpError` | Constructs/initializes an instance via `createHttpError`. |
| `badRequest` | `(detail: string): HttpError` | Invokes `badRequest` and returns `HttpError`. |
| `unauthorized` | `(detail: string): HttpError` | Invokes `unauthorized` and returns `HttpError`. |
| `forbidden` | `(detail: string): HttpError` | Invokes `forbidden` and returns `HttpError`. |
| `notFound` | `(detail: string): HttpError` | Invokes `notFound` and returns `HttpError`. |
| `methodNotAllowed` | `(detail: string): HttpError` | Invokes `methodNotAllowed` and returns `HttpError`. |
| `conflict` | `(detail: string): HttpError` | Invokes `conflict` and returns `HttpError`. |
| `gone` | `(detail: string): HttpError` | Invokes `gone` and returns `HttpError`. |
| `payloadTooLarge` | `(detail: string): HttpError` | Invokes `payloadTooLarge` and returns `HttpError`. |
| `unsupportedMediaType` | `(detail: string): HttpError` | Invokes `unsupportedMediaType` and returns `HttpError`. |
| `unprocessableEntity` | `(detail: string): HttpError` | Invokes `unprocessableEntity` and returns `HttpError`. |
| `tooManyRequests` | `(detail: string): HttpError` | Invokes `tooManyRequests` and returns `HttpError`. |
| `internalServerError` | `(detail: string): HttpError` | Invokes `internalServerError` and returns `HttpError`. |
| `notImplemented` | `(detail: string): HttpError` | Invokes `notImplemented` and returns `HttpError`. |
| `badGateway` | `(detail: string): HttpError` | Invokes `badGateway` and returns `HttpError`. |
| `serviceUnavailable` | `(detail: string): HttpError` | Invokes `serviceUnavailable` and returns `HttpError`. |
| `gatewayTimeout` | `(detail: string): HttpError` | Invokes `gatewayTimeout` and returns `HttpError`. |
| `isClientError` | `(err: HttpError): bool` | Checks predicate `isClientError` and returns `bool`. |
| `isServerError` | `(err: HttpError): bool` | Checks predicate `isServerError` and returns `bool`. |
| `isRetryable` | `(err: HttpError): bool` | Checks predicate `isRetryable` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `hexDigitLower` | `(v: u8): u8` | Invokes `hexDigitLower` and returns `u8`. |
| `appendJsonEscaped` | `(out: []u8, start_pos: usize, s: string): usize` | Invokes `appendJsonEscaped` and returns `usize`. |
| `appendU16` | `(out: []u8, pos: usize, val: u16): usize` | Invokes `appendU16` and returns `usize`. |
| `toProblemJson` | `(err: HttpError, include_hidden_detail: bool, out: []u8): string` | Invokes `toProblemJson` and returns `string`. |

## Minimal Example

```cp
import { createHttpError } from "http-errors";

pub fn main() {
    let result = createHttpError(0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
