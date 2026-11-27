[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/i18n

Official Accept-Language / query / cookie locale negotiation & zero-heap translation plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/i18n` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/i18n
```

```cp
import {
    I18nConfig,
    I18nEntry,
    i18nAsciiLower,
    i18nEqlIgnoreCase,
    i18nStrEql,
    i18nMatchLocale,
    i18nNegotiateHeader,
    i18nResetCatalog,
} from "@web/i18n";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `I18nConfig` | `class` | Fields: `default_locale: string, supported_locales: string, query_param: string, cookie_name: string` |
| `I18nEntry` | `class` | Fields: `locale: string, key: string, val: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `i18nAsciiLower` | `(c: u8): u8` | Invokes `i18nAsciiLower` and returns `u8`. |
| `i18nEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `i18nEqlIgnoreCase` and returns `bool`. |
| `i18nStrEql` | `(a: string, b: string): bool` | Invokes `i18nStrEql` and returns `bool`. |
| `i18nMatchLocale` | `(supported: string, candidate: string): string?` | Invokes `i18nMatchLocale` and returns `string?`. |
| `i18nNegotiateHeader` | `(supported: string, accept_lang: string, fallback: string): string` | Invokes `i18nNegotiateHeader` and returns `string`. |
| `i18nResetCatalog` | `()` | Executes `i18nResetCatalog` in-place. |
| `i18nAddTranslation` | `(locale: string, key: string, value: string): bool` | Invokes `i18nAddTranslation` and returns `bool`. |
| `i18nTranslate` | `(locale: string, key: string): string` | Invokes `i18nTranslate` and returns `string`. |
| `i18nDeriveHook` | `(ctx: &mut Context): bool` | Invokes `i18nDeriveHook` and returns `bool`. |
| `createI18nPlugin` | `(cfg: I18nConfig): Plugin` | Constructs/initializes an instance via `createI18nPlugin`. |
| `registerI18n` | `(app: &mut App, cfg: I18nConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { i18nEqlIgnoreCase } from "@web/i18n";

pub fn main() {
    let result = i18nEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
