# icu_locid — AI Agent Quick Ref

Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer reusing tinystr (ICU4X icu_locid parity) in pure CP. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class LanguageIdentifier { language: TinyAsciiStr8, has_script: bool, script: TinyAsciiStr4, has_region: bool, /* +3 fields */ } // methods: und(...) +6
class LocaleParseResult { ok: bool, locale: LanguageIdentifier, err: i64 }
```

## API Signatures

```cp
fn locIsSep(ch: u8): bool;
fn locIsDigitByte(ch: u8): bool;
fn locEmptyVariants(): [4]TinyAsciiStr8;
fn locLangEqStr(lang: TinyAsciiStr8, lit: string): bool;
fn locRegionEqStr(reg: TinyAsciiStr4, lit: string): bool;
fn locValidLanguageSubtag(sub: TinyAsciiStr8): bool;
fn locValidVariantSubtag(sub: TinyAsciiStr8): bool;
```

## Minimal Usage

```cp
import { locLangEqStr } from "icu_locid";

pub fn main() {
    let result = locLangEqStr(/* lang: TinyAsciiStr8 */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"icu_locid"`; install via `cap add icu_locid`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
