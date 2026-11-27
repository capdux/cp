[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fast-redact

Zero-allocation PII & secret redaction/masking engine (fast-redact / maskdata / secrecy parity: emails, Luhn credit cards, JWT/API tokens, phones, IPv4, and sensitive JSON fields) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-redact` ([https://github.com/davidmarkclements/fast-redact](https://github.com/davidmarkclements/fast-redact))（原库名 `fast-redact` → CP 包名 `fast-redact`）

| Metadata | Value |
| --- | --- |
| Package | `fast-redact` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fast-redact
```

```cp
import {
    RedactOptions,
    isTokenChar,
    isEmailLocalChar,
    isDomainChar,
    appendStr,
    appendByte,
    startsWithAt,
    maskEmail,
} from "fast-redact";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RedactOptions` | `class` | Fields: `redact_emails: bool, redact_tokens: bool, redact_cards: bool, redact_phones: bool, redact_ipv4: bool` · Methods: `defaultAll, strict` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isTokenChar` | `(c: u8): bool` | Checks predicate `isTokenChar` and returns `bool`. |
| `isEmailLocalChar` | `(c: u8): bool` | Checks predicate `isEmailLocalChar` and returns `bool`. |
| `isDomainChar` | `(c: u8): bool` | Checks predicate `isDomainChar` and returns `bool`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `startsWithAt` | `(s: string, idx: usize, prefix: string): bool` | Invokes `startsWithAt` and returns `bool`. |
| `maskEmail` | `(email: string, out: []u8): string` | Invokes `maskEmail` and returns `string`. |
| `maskPhone` | `(phone: string, out: []u8): string` | Invokes `maskPhone` and returns `string`. |
| `maskCard` | `(card: string, out: []u8): string` | Invokes `maskCard` and returns `string`. |
| `maskSecret` | `(secret: string, out: []u8): string` | Invokes `maskSecret` and returns `string`. |
| `maskIp` | `(ip: string, out: []u8): string` | Invokes `maskIp` and returns `string`. |
| `checkLuhnRange` | `(s: string, start: usize, end: usize): bool` | Validates input with `checkLuhnRange` returning `bool`. |
| `matchTokenEnd` | `(s: string, idx: usize): usize` | Invokes `matchTokenEnd` and returns `usize`. |
| `matchEmailEnd` | `(s: string, idx: usize): usize` | Invokes `matchEmailEnd` and returns `usize`. |
| `matchCardEnd` | `(s: string, idx: usize): usize` | Invokes `matchCardEnd` and returns `usize`. |
| `matchPhoneEnd` | `(s: string, idx: usize): usize` | Invokes `matchPhoneEnd` and returns `usize`. |
| `matchIpv4End` | `(s: string, idx: usize): usize` | Invokes `matchIpv4End` and returns `usize`. |
| `redactText` | `(input: string, opts: RedactOptions, out: []u8): string` | Invokes `redactText` and returns `string`. |
| `containsSensitive` | `(input: string): bool` | Invokes `containsSensitive` and returns `bool`. |
| `isSensitiveKey` | `(key: string): bool` | Checks predicate `isSensitiveKey` and returns `bool`. |
| `findJsonStringEnd` | `(s: string, open_quote: usize): usize` | Queries/retrieves data via `findJsonStringEnd` returning `usize`. |
| `redactJsonFields` | `(json_str: string, out: []u8): string` | Invokes `redactJsonFields` and returns `string`. |

## Minimal Example

```cp
import { isSensitiveKey } from "fast-redact";

pub fn main() {
    let result = isSensitiveKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
