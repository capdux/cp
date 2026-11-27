[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jwt-decode

Zero-allocation RFC 7519 JWT segment splitter, Base64URL decoder, exp/nbf leeway validator, and alg:none guard in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jwt-decode` ([https://github.com/auth0/jwt-decode](https://github.com/auth0/jwt-decode)) / `crates.io:jsonwebtoken` ([https://github.com/Keats/jsonwebtoken](https://github.com/Keats/jsonwebtoken))（原库名 `jwt-decode` → CP 包名 `jwt-decode`）

| Metadata | Value |
| --- | --- |
| Package | `jwt-decode` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jwt-decode
```

```cp
import {
    JwtParts,
    invalidParts,
    b64UrlVal,
    splitToken,
    decodeBase64Url,
    decodeHeader,
    decodePayload,
    isExpired,
} from "jwt-decode";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JwtParts` | `class` | Fields: `header_b64: string, payload_b64: string, signature_b64: string, signing_input: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidParts` | `(): JwtParts` | Invokes `invalidParts` and returns `JwtParts`. |
| `b64UrlVal` | `(b: u8): i32` | Invokes `b64UrlVal` and returns `i32`. |
| `splitToken` | `(token: string): JwtParts` | Invokes `splitToken` and returns `JwtParts`. |
| `decodeBase64Url` | `(input: string, out: []u8): i64` | Parses input via `decodeBase64Url` and returns `i64`. |
| `decodeHeader` | `(token: string, out: []u8): i64` | Parses input via `decodeHeader` and returns `i64`. |
| `decodePayload` | `(token: string, out: []u8): i64` | Parses input via `decodePayload` and returns `i64`. |
| `isExpired` | `(exp_unix: i64, now_unix: i64, leeway_secs: i64): bool` | Checks predicate `isExpired` and returns `bool`. |
| `isNotYetValid` | `(nbf_unix: i64, now_unix: i64, leeway_secs: i64): bool` | Checks predicate `isNotYetValid` and returns `bool`. |
| `isJsonSpace` | `(b: u8): bool` | Checks predicate `isJsonSpace` and returns `bool`. |
| `isAlgNone` | `(header_json: string): bool` | Checks predicate `isAlgNone` and returns `bool`. |

## Minimal Example

```cp
import { decodeBase64Url } from "jwt-decode";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeBase64Url("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
