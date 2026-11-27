[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/icu-locid`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:icu_locid` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))（原库名 `icu_locid` → CP 包名 `@cap/icu-locid`）

Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser, canonicalizer, and likely-subtags maximizer in pure CP (ICU4X `icu_locid` / `icu_locale_core` parity). Reuses `@cap/tinystr` (`TinyAsciiStr4`, `TinyAsciiStr8`) for stack-inline ASCII subtags and case normalization.

## API

- `LanguageIdentifier` — Stack-inline locale representation (`language: TinyAsciiStr8`, `script: TinyAsciiStr4`, `region: TinyAsciiStr4`, `variants: [4]TinyAsciiStr8`)
- `LanguageIdentifier.parse(input: string): LocaleParseResult` — Parses `-` or `_` separated BCP-47 tags with strict UTS #35 validation and automatic canonicalization (lower language, Title script, UPPER region, sorted lower variants)
- `LanguageIdentifier.und(): LanguageIdentifier` — Undetermined locale (`"und"`)
- `loc.toStr(out: []u8): usize` — Formats canonical hyphenated BCP-47 tag into `out`
- `loc.eq(other: LanguageIdentifier): bool` — Exact structural equality
- `loc.matches(other: LanguageIdentifier): bool` — Wildcard/partial subtag compatibility match
- `loc.isUnd(): bool` — Checks if language subtag is `"und"`
- `loc.maximizeLikelySubtags(): LanguageIdentifier` — CLDR Add Likely Subtags (`zh -> zh-Hans-CN`, `en -> en-Latn-US`, `ja -> ja-Jpan-JP`, `ko -> ko-Kore-KR`, `ar -> ar-Arab-EG`, `ru -> ru-Cyrl-RU`, etc.)

## Error Codes

- `LOCALE_OK` (`0`)
- `LOCALE_ERR_EMPTY` (`-1`)
- `LOCALE_ERR_EMPTY_SUBTAG` (`-2`)
- `LOCALE_ERR_INVALID_LANGUAGE` (`-3`)
- `LOCALE_ERR_INVALID_SUBTAG` (`-4`)
- `LOCALE_ERR_TOO_MANY_VARIANTS` (`-5`)
- `LOCALE_ERR_DUPLICATE_VARIANT` (`-6`)
