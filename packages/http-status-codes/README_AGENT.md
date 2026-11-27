# http-status-codes — AI Agent Quick Ref

Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional phrase lookup, and semantic predicates for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StatusClass,
    reasonPhrase,
    isSep,
    eqNormalizedPhrase,
    codeFromPhrase,
    statusClass,
    isValidStatus,
    isKnownStatus,
} from "http-status-codes";
```

## Core Types & Constants

```cp
enum StatusClass { Invalid, Informational, Success, Redirection, ClientError, ServerError }
```

## API Signatures

```cp
fn reasonPhrase(code: u16): string;
fn isSep(c: u8): bool;
fn eqNormalizedPhrase(input: string, target: string): bool;
fn codeFromPhrase(phrase: string): u16?;
fn statusClass(code: u16): StatusClass;
fn isValidStatus(code: u16): bool;
fn isKnownStatus(code: u16): bool;
fn isInformational(code: u16): bool;
fn isSuccess(code: u16): bool;
fn isRedirect(code: u16): bool;
fn isClientError(code: u16): bool;
fn isServerError(code: u16): bool;
fn isEmptyBody(code: u16): bool;
fn isRetryable(code: u16): bool;
fn isCacheableByDefault(code: u16): bool;
```

## Minimal Usage

```cp
import { eqNormalizedPhrase } from "http-status-codes";

pub fn main() {
    let result = eqNormalizedPhrase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http-status-codes"`; install via `cap add http-status-codes`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
