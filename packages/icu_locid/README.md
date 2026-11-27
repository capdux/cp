[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# icu_locid

Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer reusing tinystr (ICU4X icu_locid parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:icu_locid` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))（原库名 `icu_locid` → CP 包名 `icu_locid`）

| Metadata | Value |
| --- | --- |
| Package | `icu_locid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add icu_locid
```

```cp
import {
    LanguageIdentifier,
    LocaleParseResult,
    locIsSep,
    locIsDigitByte,
    locEmptyVariants,
    locLangEqStr,
    locRegionEqStr,
    locValidLanguageSubtag,
} from "icu_locid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LanguageIdentifier` | `class` | Fields: `language: TinyAsciiStr8, has_script: bool, script: TinyAsciiStr4, has_region: bool, region: TinyAsciiStr4, variant_count: usize, +1 more` · Methods: `und, parse, toStr, isUnd, eq, matches, +1 more` |
| `LocaleParseResult` | `class` | Fields: `ok: bool, locale: LanguageIdentifier, err: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `locIsSep` | `(ch: u8): bool` | Invokes `locIsSep` and returns `bool`. |
| `locIsDigitByte` | `(ch: u8): bool` | Invokes `locIsDigitByte` and returns `bool`. |
| `locEmptyVariants` | `(): [4]TinyAsciiStr8` | Invokes `locEmptyVariants` and returns `[4]TinyAsciiStr8`. |
| `locLangEqStr` | `(lang: TinyAsciiStr8, lit: string): bool` | Invokes `locLangEqStr` and returns `bool`. |
| `locRegionEqStr` | `(reg: TinyAsciiStr4, lit: string): bool` | Invokes `locRegionEqStr` and returns `bool`. |
| `locValidLanguageSubtag` | `(sub: TinyAsciiStr8): bool` | Invokes `locValidLanguageSubtag` and returns `bool`. |
| `locValidVariantSubtag` | `(sub: TinyAsciiStr8): bool` | Invokes `locValidVariantSubtag` and returns `bool`. |

## Minimal Example

```cp
import { locLangEqStr } from "icu_locid";

pub fn main() {
    let result = locLangEqStr(/* lang: TinyAsciiStr8 */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
