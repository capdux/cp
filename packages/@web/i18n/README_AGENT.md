# @web/i18n — AI Agent Quick Ref

Official Accept-Language / query / cookie locale negotiation & zero-heap translation plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class I18nConfig { default_locale: string, supported_locales: string, query_param: string, cookie_name: string }
class I18nEntry { locale: string, key: string, val: string }
```

## API Signatures

```cp
fn i18nAsciiLower(c: u8): u8;
fn i18nEqlIgnoreCase(a: string, b: string): bool;
fn i18nStrEql(a: string, b: string): bool;
fn i18nMatchLocale(supported: string, candidate: string): string?;
fn i18nNegotiateHeader(supported: string, accept_lang: string, fallback: string): string;
fn i18nResetCatalog();
fn i18nAddTranslation(locale: string, key: string, value: string): bool;
fn i18nTranslate(locale: string, key: string): string;
fn i18nDeriveHook(ctx: &mut Context): bool;
fn createI18nPlugin(cfg: I18nConfig): Plugin;
fn registerI18n(app: &mut App, cfg: I18nConfig): &mut App;
```

## Minimal Usage

```cp
import { i18nEqlIgnoreCase } from "@web/i18n";

pub fn main() {
    let result = i18nEqlIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/i18n"`; install via `cap add @web/i18n`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
