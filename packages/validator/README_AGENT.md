# `validator` — AI Agent Quick Ref

Zero-allocation format validation predicates & stack-allocated multi-error collector (npm `validator` v13 / Rust `validator` parity).

## Import

```cp
import {
    Validator, ValidationError,
    isAscii, startsWith, endsWith, isAlphanumeric, isNumeric, isHex, isBase64, isSlug,
    hasMinLen, hasMaxLen, hasLenBetween, containsStr, isOneOf,
    isEmail, isIpv4, isIpv6, isCidr, isUrl, isUuid, isUlid, isSemver,
    isIsoDate, isIsoDateTime, isLuhn, isE164Phone, isPort, inRangeI64, inRangeF64, isMultipleOf,
} from "validator";
```

## Core API Signatures

```cp
// Types
struct ValidationError { field: string, code: string, message: string }
struct Validator { errors: [32]ValidationError, count: usize }

// Standalone predicates (all pure, zero-alloc, read-only borrows)
fn isAlphanumeric(s: string) -> bool;
fn isNumeric(s: string) -> bool;
fn isHex(s: string) -> bool;
fn isBase64(s: string) -> bool;
fn isSlug(s: string) -> bool;
fn hasMinLen(s: string, min_len: usize) -> bool;
fn hasMaxLen(s: string, max_len: usize) -> bool;
fn hasLenBetween(s: string, min_len: usize, max_len: usize) -> bool;
fn containsStr(s: string, needle: string) -> bool;
fn isOneOf(s: string, allowed: []string) -> bool;
fn isEmail(s: string) -> bool;
fn isIpv4(s: string) -> bool;
fn isIpv6(s: string) -> bool;
fn isCidr(s: string) -> bool;
fn isUrl(s: string) -> bool;
fn isUuid(s: string) -> bool;
fn isUlid(s: string) -> bool;
fn isSemver(s: string) -> bool;
fn isIsoDate(s: string) -> bool;
fn isIsoDateTime(s: string) -> bool;
fn isLuhn(s: string) -> bool;
fn isE164Phone(s: string) -> bool;
fn isPort(p: i64) -> bool;
fn inRangeI64(v: i64, min_v: i64, max_v: i64) -> bool;
fn inRangeF64(v: f64, min_v: f64, max_v: f64) -> bool;
fn isMultipleOf(v: i64, step: i64) -> bool;

// Validator methods (`*_` mutates self; must declare `let v = Validator.init()`)
Validator.init() -> Validator;
v.isValid() -> bool;  v.errorCount() -> usize;  v.errorAt(idx: usize) -> ValidationError;
v.addError(*_, field: string, code: string, message: string);
v.required/email/url/uuid/ulid/ipv4/ipv6/cidr/semver/isoDate/isoDateTime/luhn/phone/slug(*_, field, val) -> bool;
v.minLen/maxLen(*_, field, val, n: usize) -> bool;
v.oneOf(*_, field, val, allowed: []string) -> bool;
v.rangeI64(*_, field, val: i64, min_v: i64, max_v: i64) -> bool;
v.rangeF64(*_, field, val: f64, min_v: f64, max_v: f64) -> bool;
v.port(*_, field, val: i64) -> bool;
```

## Minimal Example

```cp
import { Validator, isEmail } from "validator";

let v = Validator.init();
v.email("email", "bad-email");
v.port("port", 8080);
if (!v.isValid()) { const e = v.errorAt(0); /* e.field == "email" */ }
```

## Gotchas & Borrow Rules

- `Validator` mutating methods use `*_`; always bind with `let v = Validator.init()`, never `const v`.
- Capacity is fixed at 32 errors (`MAX_VALIDATION_ERRORS = 32`); excess errors are silently dropped.
- `errorAt(idx)` returns an empty `ValidationError` when `idx >= count`; guard with `idx < v.errorCount()`.
- `isPort` checks `1..=65535` (port `0` returns `false`).
