# http-errors@1.0.0 (Agent Quickref)

> Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 application/problem+json serializer for CP | License: MIT
> Import: `import { ... } from "http-errors";` | Full types: `src/index.d.cp`
> Deps: `toidentifier`

## Functions
- `export fn statusTitle(status: u16): string;`
- `export fn errorClassName(status: u16, out: []u8): usize;`
- `export fn statusCodeName(status: u16): string;`
- `export fn createHttpError(status: u16, detail: string): HttpError;`
- `export fn badRequest(detail: string): HttpError;`
- `export fn unauthorized(detail: string): HttpError;`
- `export fn forbidden(detail: string): HttpError;`
- `export fn notFound(detail: string): HttpError;`
- `export fn methodNotAllowed(detail: string): HttpError;`
- `export fn conflict(detail: string): HttpError;`
- `export fn gone(detail: string): HttpError;`
- `export fn payloadTooLarge(detail: string): HttpError;`
- `export fn unsupportedMediaType(detail: string): HttpError;`
- `export fn unprocessableEntity(detail: string): HttpError;`
- `export fn tooManyRequests(detail: string): HttpError;`
- `export fn internalServerError(detail: string): HttpError;`
- `export fn notImplemented(detail: string): HttpError;`
- `export fn badGateway(detail: string): HttpError;`
- `export fn serviceUnavailable(detail: string): HttpError;`
- `export fn gatewayTimeout(detail: string): HttpError;`
- `export fn isClientError(err: HttpError): bool;`
- `export fn isServerError(err: HttpError): bool;`
- `export fn isRetryable(err: HttpError): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- *(+5 more in `src/index.d.cp`)*
