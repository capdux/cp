[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mailto-link

Zero-allocation RFC 6068 mailto: URI parser, recipient counter, email checker, and percent-encoding builder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mailto-link` ([https://github.com/sindresorhus/mailto-link](https://github.com/sindresorhus/mailto-link)) / `crates.io:mailto` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mailto-link` → CP 包名 `mailto-link`）

| Metadata | Value |
| --- | --- |
| Package | `mailto-link` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mailto-link
```

```cp
import {
    MailtoUri,
    invalidMailto,
    mailtoIsSpace,
    trimSlice,
    parseMailto,
    countRecipients,
    isBasicEmail,
    isUnreserved,
} from "mailto-link";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MailtoUri` | `class` | Fields: `to: string, cc: string, bcc: string, subject: string, body: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidMailto` | `(): MailtoUri` | Invokes `invalidMailto` and returns `MailtoUri`. |
| `mailtoIsSpace` | `(b: u8): bool` | Invokes `mailtoIsSpace` and returns `bool`. |
| `trimSlice` | `(s: string): string` | Invokes `trimSlice` and returns `string`. |
| `parseMailto` | `(uri: string): MailtoUri` | Parses input via `parseMailto` and returns `MailtoUri`. |
| `countRecipients` | `(to_list: string): usize` | Invokes `countRecipients` and returns `usize`. |
| `isBasicEmail` | `(addr: string): bool` | Checks predicate `isBasicEmail` and returns `bool`. |
| `isUnreserved` | `(b: u8): bool` | Checks predicate `isUnreserved` and returns `bool`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushPctEncoded` | `(dst: []u8, pos: usize, src: string, allow_addr_chars: bool): i64` | Mutates or appends state via `pushPctEncoded`. |
| `formatMailto` | `(to: string, subject: string, body: string, cc: string, out: []u8): usize` | Serializes/formats data via `formatMailto` returning `usize`. |

## Minimal Example

```cp
import { parseMailto } from "mailto-link";

pub fn main() {
    let result = parseMailto("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
