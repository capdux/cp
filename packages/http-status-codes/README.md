[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http-status-codes

Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional phrase lookup, and semantic predicates for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`http-status-codes`](https://github.com/prettymuchbryce/http-status-codes)（`MIT`）（原库名 `http-status-codes`，CP 包名 `http-status-codes`）。

| Metadata | Value |
| --- | --- |
| Package | `http-status-codes` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http-status-codes
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `StatusClass` | `Invalid, Informational, Success, Redirection, ClientError, ServerError` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `reasonPhrase` | `(code: u16): string` | Invokes `reasonPhrase` and returns `string`. |
| `isSep` | `(c: u8): bool` | Checks predicate `isSep` and returns `bool`. |
| `eqNormalizedPhrase` | `(input: string, target: string): bool` | Invokes `eqNormalizedPhrase` and returns `bool`. |
| `codeFromPhrase` | `(phrase: string): u16?` | Invokes `codeFromPhrase` and returns `u16?`. |
| `statusClass` | `(code: u16): StatusClass` | Invokes `statusClass` and returns `StatusClass`. |
| `isValidStatus` | `(code: u16): bool` | Checks predicate `isValidStatus` and returns `bool`. |
| `isKnownStatus` | `(code: u16): bool` | Checks predicate `isKnownStatus` and returns `bool`. |
| `isInformational` | `(code: u16): bool` | Checks predicate `isInformational` and returns `bool`. |
| `isSuccess` | `(code: u16): bool` | Checks predicate `isSuccess` and returns `bool`. |
| `isRedirect` | `(code: u16): bool` | Checks predicate `isRedirect` and returns `bool`. |
| `isClientError` | `(code: u16): bool` | Checks predicate `isClientError` and returns `bool`. |
| `isServerError` | `(code: u16): bool` | Checks predicate `isServerError` and returns `bool`. |
| `isEmptyBody` | `(code: u16): bool` | Checks predicate `isEmptyBody` and returns `bool`. |
| `isRetryable` | `(code: u16): bool` | Checks predicate `isRetryable` and returns `bool`. |
| `isCacheableByDefault` | `(code: u16): bool` | Checks predicate `isCacheableByDefault` and returns `bool`. |

## Minimal Example

```cp
import { eqNormalizedPhrase } from "http-status-codes";

pub fn main() {
    let result = eqNormalizedPhrase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
