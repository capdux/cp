# @cap/http-types

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types for CP — clean-room MIT parity with crates.io `http` v1 and RFC 9110.

Composes [`@cap/unicase`](../unicase) (`eqAscii`) for ASCII case-insensitive header name and HTTP version comparison with zero code duplication.

## Installation

```json
{
  "dependencies": {
    "@cap/http-types": "1.0.0"
  }
}
```

## Quick Start

```cp
import {
    METHOD_GET,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    StatusCode,
    parseHttpVersion,
    httpVersionAsStr,
    isValidHeaderName,
    headerNameEq,
} from "@cap/http-types";

const m = parseMethod("GET"); // METHOD_GET
const ok = StatusCode.fromU16(200);
// ok.canonicalReason() == "OK", ok.isSuccess() == true
const same = headerNameEq("Content-Type", "content-type"); // true
```

## API Reference

### HTTP Method
- Constants: `METHOD_UNKNOWN`, `METHOD_GET`, `METHOD_HEAD`, `METHOD_POST`, `METHOD_PUT`, `METHOD_DELETE`, `METHOD_CONNECT`, `METHOD_OPTIONS`, `METHOD_TRACE`, `METHOD_PATCH`
- `parseMethod(s: string): u8`
- `methodAsStr(m: u8): string`
- `isSafeMethod(m: u8): bool` (`GET`, `HEAD`, `OPTIONS`, `TRACE`)
- `isIdempotentMethod(m: u8): bool` (`GET`, `HEAD`, `OPTIONS`, `TRACE`, `PUT`, `DELETE`)

### HTTP StatusCode
- `StatusCode { code: u16 }`
- `StatusCode.fromU16(code: u16): StatusCode?` — validates `100..=599`
- `asU16(): u16`
- `canonicalReason(): string` — covers 60+ IANA/RFC 9110 standard status codes (1xx–5xx)
- `isInformational()`, `isSuccess()`, `isRedirection()`, `isClientError()`, `isServerError()`

### HTTP Version
- Constants: `HTTP_UNKNOWN`, `HTTP_09`, `HTTP_10`, `HTTP_11`, `HTTP_2`, `HTTP_3`
- `parseHttpVersion(s: string): u8` — ASCII case-insensitive via `@cap/unicase`
- `httpVersionAsStr(v: u8): string`

### HeaderName
- `isValidHeaderName(name: string): bool` — validates RFC 9110 `1*tchar` token rules
- `headerNameEq(a: string, b: string): bool` — ASCII case-insensitive equality via `@cap/unicase`

## License

MIT
