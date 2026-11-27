[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `validator`

Zero-allocation schema & format validation engine for CP — a clean-room MIT counterpart to npm [`validator`](https://github.com/validatorjs/validator.js) (v13 parity) and Rust `validator`, featuring 30+ standalone format predicates and a stack-allocated `Validator` multi-error collector.

## Installation & Import

```bash
cap add validator
```

```cp
import {
    Validator,
    ValidationError,
    isEmail,
    isUrl,
    isUuid,
    isUlid,
    isIpv4,
    isIpv6,
    isCidr,
    isSemver,
    isIsoDate,
    isIsoDateTime,
    isLuhn,
    isE164Phone,
    isPort,
    inRangeI64,
} from "validator";
```

## Core API

### Types

| Type | Fields / Methods | Description |
| :--- | :--- | :--- |
| `ValidationError` | `field: string`, `code: string`, `message: string` | Structured validation failure entry |
| `Validator` | `errors: [32]ValidationError`, `count: usize` | Stack-allocated multi-error collector (up to 32 errors, zero heap) |

### Standalone Format & Range Predicates

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isAscii` / `startsWith` / `endsWith` | `(s: string, ...) -> bool` | Re-exported ASCII/prefix/suffix helpers from `cap:ascii` |
| `isAlphanumeric` / `isNumeric` / `isHex` | `(s: string) -> bool` | Character-class checks (`[A-Za-z0-9]`, numeric, hex) |
| `isBase64` / `isSlug` | `(s: string) -> bool` | Standard Base64 and lowercase URL slug validation |
| `hasMinLen` / `hasMaxLen` / `hasLenBetween` | `(s: string, min/max: usize) -> bool` | Byte-length bounds checks |
| `containsStr` / `isOneOf` | `(s: string, needle: string / allowed: []string) -> bool` | Substring and allow-list membership checks |
| `isEmail` / `isUrl` | `(s: string) -> bool` | RFC-style email and `http(s)://` URL validation |
| `isIpv4` / `isIpv6` / `isCidr` | `(s: string) -> bool` | IPv4, IPv6, and IPv4/IPv6 CIDR notation checks |
| `isIpv4Range` / `isIpv6Range` | `(s: string, start: usize, end: usize) -> bool` | Zero-copy slice-range IP validation |
| `isUuid` / `isUlid` / `isSemver` | `(s: string) -> bool` | UUID (8-4-4-4-12), Crockford Base32 ULID (26 chars), SemVer 2.0 |
| `isIsoDate` / `isIsoDateTime` | `(s: string) -> bool` | `YYYY-MM-DD` (leap-year aware) and ISO-8601 datetime |
| `isLuhn` / `isE164Phone` | `(s: string) -> bool` | Luhn mod-10 checksum (cards/IMEI) and E.164 phone (`+...`) |
| `isPort` / `inRangeI64` / `inRangeF64` / `isMultipleOf` | `(v, ...) -> bool` | TCP/UDP port (`1..=65535`), inclusive numeric ranges, divisibility |

### `Validator` Collector Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Validator.init` | `() -> Validator` | Construct an empty stack-allocated validator |
| `isValid` / `errorCount` / `errorAt` | `() -> bool` / `() -> usize` / `(idx: usize) -> ValidationError` | Inspect collected errors (read-only receiver) |
| `addError` | `(*_, field: string, code: string, message: string)` | Append a custom error (silently caps at 32 entries) |
| `required` / `minLen` / `maxLen` | `(*_, field, value, ...) -> bool` | Record error if empty or outside length bounds |
| `email` / `url` / `uuid` / `ulid` / `slug` | `(*_, field, value) -> bool` | Validate format and record error on failure |
| `ipv4` / `ipv6` / `cidr` / `semver` | `(*_, field, value) -> bool` | Validate network/version strings and record error |
| `isoDate` / `isoDateTime` / `luhn` / `phone` | `(*_, field, value) -> bool` | Validate date/card/E.164 phone and record error |
| `oneOf` / `rangeI64` / `rangeF64` / `port` | `(*_, field, value, ...) -> bool` | Validate allow-list or numeric range and record error |

## Examples

### 1. Standalone Zero-Copy Format Checks

```cp
import { print } from "cap:io";
import { isEmail, isUrl, isUuid, isCidr, isSemver, isLuhn } from "validator";

fn main() {
    print("email:", isEmail("dev@cp-lang.org"));         // true
    print("url:", isUrl("https://cp-lang.org/docs"));    // true
    print("uuid:", isUuid("550e8400-e29b-41d4-a716-446655440000")); // true
    print("cidr:", isCidr("10.0.0.0/8"));                // true
    print("semver:", isSemver("2.1.0-rc.1+build.42"));   // true
    print("luhn:", isLuhn("79927398713"));               // true
}
```

### 2. Multi-Error Form Validation with `Validator`

```cp
import { print } from "cap:io";
import { Validator } from "validator";

fn validateSignup(email: string, handle: string, port: i64, role: string) {
    // Must be `let` because mutating methods take `*` receiver
    let v = Validator.init();
    const roles: [3]string = ["admin", "editor", "viewer"];

    v.required("email", email);
    v.email("email", email);
    v.minLen("handle", handle, 3);
    v.slug("handle", handle);
    v.port("port", port);
    v.oneOf("role", role, roles[0..3]);

    if (!v.isValid()) {
        let i: usize = 0;
        while (i < v.errorCount()) {
            const err = v.errorAt(i);
            print(err.field, err.code, err.message);
            i += 1;
        }
    }
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocations**: Unlike npm `validator` (which relies on RegExp compilation and JS string allocations), every predicate scans raw UTF-8/ASCII bytes in a single pass with `O(n)` time and `O(1)` stack space.
- **Built-in Multi-Error Collector**: `Validator` stores up to `32` `ValidationError` structs inline on the stack (`[32]ValidationError`), eliminating `Array` allocations in hot HTTP/API request validation paths.
- **No Implicit Type Coercion**: Strictly typed CP parameters (`string`, `i64`, `f64`, `usize`) prevent JavaScript string-coercion pitfalls; all errors are returned as booleans or structured entries without throwing exceptions.
