# icu_locid@1.0.0 (Agent Quickref)

> Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer reusing tinystr (ICU4X icu_locid parity) in pure CP. | License: MIT
> Import: `import { ... } from "icu_locid";` | Full types: `src/index.d.cp`
> Deps: `tinystr`

## Functions
- `export fn locIsSep(ch: u8): bool;`
- `export fn locIsDigitByte(ch: u8): bool;`
- `export fn locEmptyVariants(): [4]TinyAsciiStr8;`
- `export fn locLangEqStr(lang: TinyAsciiStr8, lit: string): bool;`
- `export fn locRegionEqStr(reg: TinyAsciiStr4, lit: string): bool;`
- `export fn locValidLanguageSubtag(sub: TinyAsciiStr8): bool;`
- `export fn locValidVariantSubtag(sub: TinyAsciiStr8): bool;`
