[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `idna`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:idna` ([https://github.com/servo/rust-url](https://github.com/servo/rust-url))（原库名 `idna` → CP 包名 `idna`）

Zero-allocation UTS #46 Internationalized Domain Names (IDNA) in pure CP (Rust `idna` crate parity). Reuses `icu_normalizer` (`normalizeNfc`) for UAX #15 NFC normalization and `punycode` (`encodeLabel`, `decodeLabel`, `isAsciiOnly`) for RFC 3492 Punycode with zero duplication.

## API

- `domainToAscii(domain: string, out: []u8): i64` — Maps Unicode full stops (`U+3002`/`U+FF0E`/`U+FF61`) to `.`, lowercases ASCII, normalizes non-ASCII labels via `icu_normalizer::normalizeNfc`, encodes A-labels as `xn--<punycode>`, and enforces LDH + 63/253 length limits
- `domainToUnicode(domain: string, out: []u8): i64` — Decodes `xn--` A-labels back to canonical UTF-8 U-labels and lowercases ASCII labels
- `isValidIdnaAscii(domain: string): bool` — Validates ASCII IDNA domain syntax, LDH rules, 63/253 limits, and Punycode decodability
- `isPunycodeLabel(label: string): bool` — Case-insensitive check for RFC 3492 `"xn--"` ACE prefix

## Error Codes

- `IDNA_OK` (`0`)
- `IDNA_ERR_EMPTY` (`-1`)
- `IDNA_ERR_EMPTY_LABEL` (`-2`)
- `IDNA_ERR_LABEL_TOO_LONG` (`-3`)
- `IDNA_ERR_DOMAIN_TOO_LONG` (`-4`)
- `IDNA_ERR_INVALID_HYPHEN` (`-5`)
- `IDNA_ERR_INVALID_CHAR` (`-6`)
- `IDNA_ERR_INVALID_PUNYCODE` (`-7`)
- `IDNA_ERR_BUFFER_TOO_SMALL` (`-8`)
