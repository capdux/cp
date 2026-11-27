[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# statuses

Zero-heap RFC 9110 / IANA HTTP status code lookup, reason phrases, reverse lookup, and category predicates (npm statuses parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:statuses` ([https://github.com/jshttp/statuses](https://github.com/jshttp/statuses))

| Metadata | Value |
| --- | --- |
| Package | `statuses` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add statuses
```

```cp
import {
    message,
    codeFromMessage,
    isValid,
    isInformational,
    isSuccess,
    isRedirect,
    isEmpty,
    isRetry,
} from "statuses";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `message` | `(code: u16): string` | Invokes `message` and returns `string`. |
| `codeFromMessage` | `(msg: string): u16` | Invokes `codeFromMessage` and returns `u16`. |
| `isValid` | `(code: u16): bool` | Checks predicate `isValid` and returns `bool`. |
| `isInformational` | `(code: u16): bool` | Checks predicate `isInformational` and returns `bool`. |
| `isSuccess` | `(code: u16): bool` | Checks predicate `isSuccess` and returns `bool`. |
| `isRedirect` | `(code: u16): bool` | Checks predicate `isRedirect` and returns `bool`. |
| `isEmpty` | `(code: u16): bool` | Checks predicate `isEmpty` and returns `bool`. |
| `isRetry` | `(code: u16): bool` | Checks predicate `isRetry` and returns `bool`. |
| `isClientError` | `(code: u16): bool` | Checks predicate `isClientError` and returns `bool`. |
| `isServerError` | `(code: u16): bool` | Checks predicate `isServerError` and returns `bool`. |

## Minimal Example

```cp
import { codeFromMessage } from "statuses";

pub fn main() {
    let result = codeFromMessage("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
