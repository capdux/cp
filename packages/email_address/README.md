[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# email_address

Zero-heap RFC 5321/5322 email address parser and validator

| Metadata | Value |
| --- | --- |
| Package | `email_address` |
| Version | `0.2.9` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add email_address
```

```cp
import {
    EmailAddress,
    emIsAtext,
    emIsLabelChar,
    emTrim,
    emValidLocal,
    emValidDomain,
    parseEmail,
    isValidEmail,
} from "email_address";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EmailAddress` | `class` | Fields: `local_part: string, domain: string, display_name: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emIsAtext` | `(b: u8): bool` | Invokes `emIsAtext` and returns `bool`. |
| `emIsLabelChar` | `(b: u8): bool` | Invokes `emIsLabelChar` and returns `bool`. |
| `emTrim` | `(s: string): string` | Invokes `emTrim` and returns `string`. |
| `emValidLocal` | `(local: string): bool` | Invokes `emValidLocal` and returns `bool`. |
| `emValidDomain` | `(domain: string): bool` | Invokes `emValidDomain` and returns `bool`. |
| `parseEmail` | `(input: string): EmailAddress` | Parses input via `parseEmail` and returns `EmailAddress`. |
| `isValidEmail` | `(input: string): bool` | Checks predicate `isValidEmail` and returns `bool`. |

## Minimal Example

```cp
import { parseEmail } from "email_address";

pub fn main() {
    let result = parseEmail("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
