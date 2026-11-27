[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# express-bearer-token

Zero-allocation RFC 6750 HTTP Bearer token extractor, JWT structure precheck, constant-time verifier, and WWW-Authenticate challenge formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:express-bearer-token` ([https://github.com/tkellen/js-express-bearer-token](https://github.com/tkellen/js-express-bearer-token))（原库名 `express-bearer-token` → CP 包名 `express-bearer-token`）

| Metadata | Value |
| --- | --- |
| Package | `express-bearer-token` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add express-bearer-token
```

```cp
import {
    isAuthSpace,
    isB64TokenCore,
    isValidTokenChars,
    extractBearer,
    formatBearer,
    isBase64UrlChar,
    isJwtLike,
    constantTimeEq,
} from "express-bearer-token";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isAuthSpace` | `(b: u8): bool` | Checks predicate `isAuthSpace` and returns `bool`. |
| `isB64TokenCore` | `(b: u8): bool` | Checks predicate `isB64TokenCore` and returns `bool`. |
| `isValidTokenChars` | `(token: string): bool` | Checks predicate `isValidTokenChars` and returns `bool`. |
| `extractBearer` | `(header: string): string` | Invokes `extractBearer` and returns `string`. |
| `formatBearer` | `(token: string, out: []u8): usize` | Serializes/formats data via `formatBearer` returning `usize`. |
| `isBase64UrlChar` | `(b: u8): bool` | Checks predicate `isBase64UrlChar` and returns `bool`. |
| `isJwtLike` | `(token: string): bool` | Checks predicate `isJwtLike` and returns `bool`. |
| `constantTimeEq` | `(a: string, b: string): bool` | Invokes `constantTimeEq` and returns `bool`. |
| `verifyBearer` | `(header: string, expected_token: string): bool` | Validates input with `verifyBearer` returning `bool`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushQuotedParam` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | Mutates or appends state via `pushQuotedParam`. |
| `formatChallenge` | `(realm: string, err_code: string, err_desc: string, out: []u8): usize` | Serializes/formats data via `formatChallenge` returning `usize`. |

## Minimal Example

```cp
import { isValidTokenChars } from "express-bearer-token";

pub fn main() {
    let result = isValidTokenChars("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
