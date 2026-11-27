[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# http

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http` ([https://github.com/hyperium/http](https://github.com/hyperium/http))（原库名 `http` → CP 包名 `http`）

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types for CP — clean-room MIT parity with crates.io `http` v1 and RFC 9110.

Composes [`unicase`](../unicase) (`eqAscii`) for ASCII case-insensitive header name and HTTP version comparison with zero code duplication.

## Installation

```json
{
  "dependencies": {
    "http": "1.0.0"
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
} from "http";

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
- `parseHttpVersion(s: string): u8` — ASCII case-insensitive via `unicase`
- `httpVersionAsStr(v: u8): string`

### HeaderName
- `isValidHeaderName(name: string): bool` — validates RFC 9110 `1*tchar` token rules
- `headerNameEq(a: string, b: string): bool` — ASCII case-insensitive equality via `unicase`

## License

MIT
