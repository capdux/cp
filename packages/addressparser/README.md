[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# addressparser

Zero-allocation RFC 5322 email address and recipient list parser and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:addressparser` ([https://github.com/nodemailer/addressparser](https://github.com/nodemailer/addressparser))（原库名 `addressparser` → CP 包名 `addressparser`）

| Metadata | Value |
| --- | --- |
| Package | `addressparser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add addressparser
```

```cp
import {
    MailAddress,
    AddressList,
    invalidMail,
    stripEdgeComments,
    isValidLocal,
    isValidDomain,
    makeAddrFromSpec,
    unwrapDisplayName,
} from "addressparser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MailAddress` | `class` | Fields: `valid: bool, display_name: string, address: string, local: string, domain: string` |
| `AddressList` | `class` | Fields: `items: [MAX_ADDRESSES]MailAddress, count: usize` · Methods: `get` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidMail` | `(): MailAddress` | Invokes `invalidMail` and returns `MailAddress`. |
| `stripEdgeComments` | `(raw: string): string` | Invokes `stripEdgeComments` and returns `string`. |
| `isValidLocal` | `(local: string): bool` | Checks predicate `isValidLocal` and returns `bool`. |
| `isValidDomain` | `(domain: string): bool` | Checks predicate `isValidDomain` and returns `bool`. |
| `makeAddrFromSpec` | `(displayName: string, addrSpec: string): MailAddress` | Invokes `makeAddrFromSpec` and returns `MailAddress`. |
| `unwrapDisplayName` | `(rawName: string): string` | Invokes `unwrapDisplayName` and returns `string`. |
| `parseMailbox` | `(raw: string): MailAddress` | Parses input via `parseMailbox` and returns `MailAddress`. |
| `parseAddressList` | `(raw: string): AddressList` | Parses input via `parseAddressList` and returns `AddressList`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `needsQuoting` | `(name: string): bool` | Invokes `needsQuoting` and returns `bool`. |
| `formatMailbox` | `(m: MailAddress, out: []u8): string` | Serializes/formats data via `formatMailbox` returning `string`. |

## Minimal Example

```cp
import { isValidLocal } from "addressparser";

pub fn main() {
    let result = isValidLocal("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
