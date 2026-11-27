[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# regex-syntax

Zero-allocation regular expression syntax validator, metacharacter escaper, and static literal prefix extractor for CP (Rust `regex-syntax` v0.8 parity).

Validates regex patterns on the stack (balanced groups, character classes, escapes, `{m,n}` quantifiers, nesting limits), computes capture counts, anchor/backreference flags, minimum match length, and extracts static literal prefixes for SIMD prefiltering.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `regex-syntax` v0.8 (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/rust-lang/regex` |
| License | MIT (upstream: MIT OR Apache-2.0) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: regex-syntax)
let escaped = regex_syntax::escape("a.b+c?");
let is_meta = regex_syntax::is_meta_character(b'*');
let hir = regex_syntax::parse(pattern)?;
```

```cp
// CP (cap: regex-syntax) — zero-allocation stack validator & escaper
import { escapeRegex, isMetaCharacter, validateRegexSyntax, extractLiteralPrefix } from "regex-syntax";
const n = escapeRegex("a.b+c?", outBuf);
const info = validateRegexSyntax(pattern);
```

## Installation

```bash
cap add regex-syntax
```

## Usage

### Basic Usage: Validation, Metacharacter Escaping & Literal Prefix Extraction

```cp
import {
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    validateRegexSyntax,
    extractLiteralPrefix
} from "regex-syntax";

// 1. Validate a regex pattern and inspect static properties
const info = validateRegexSyntax("^GET\\s+(/api/[a-z]+)");
if (info.isOk()) {
    // info.capture_count == 1
    // info.has_anchors == true
    // info.has_backref == false
    // info.min_len >= 9
}

// 2. Escape user input safely into a stack buffer before embedding in a regex
let escBuf: [64]u8 = undefined;
const escLen = escapeRegex("price=$9.99 (USD)", escBuf[0..64]);
// escBuf[0..escLen] == "price=\\$9\\.99 \\(USD\\)"

// 3. Extract static literal prefix (useful for memchr/memmem prefiltering)
let pfxBuf: [64]u8 = undefined;
const pfxLen = extractLiteralPrefix("https://api\\.example\\.com/[0-9]+", pfxBuf[0..64]);
// pfxBuf[0..pfxLen] == "https://api.example.com/"

const meta = isMetaCharacter('*' as u8); // true
const word = isWordByte('_' as u8);      // true
```

### Security & Edge Cases: Malformed Pattern Diagnostics & Nesting Limit Guard

```cp
import { validateRegexSyntax } from "regex-syntax";

// 1. Detect unclosed group / dangling quantifier with exact byte position `err_pos`
const bad1 = validateRegexSyntax("(abc");
// bad1.isOk() == false, bad1.err_code == 1 (UNCLOSED_GROUP), bad1.err_pos == 4

const bad2 = validateRegexSyntax("*abc");
// bad2.isOk() == false, bad2.err_code == 6 (DANGLING_QUANTIFIER), bad2.err_pos == 0

// 2. Reject invalid character class ranges (e.g. [z-a]) or empty classes ([])
const bad3 = validateRegexSyntax("[z-a]");
// bad3.isOk() == false, bad3.err_code == 4 (INVALID_CLASS_RANGE)

// 3. Built-in nesting limit protection (err_code == 12) prevents stack exhaustion on deeply nested "((((...))))"
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `RegexSyntaxInfo` | `class { valid, err_code, err_pos, capture_count, has_anchors, has_backref, min_len, literal_prefix_len, fn isOk(): bool }` | Full static analysis & diagnostic result. |
| `validateRegexSyntax` | `fn(pattern: string): RegexSyntaxInfo` | Validates regex syntax and computes captures, anchors, backrefs, min length, and prefix length. |
| `isMetaCharacter` | `fn(c: u8): bool` | Returns `true` for the 18 regex metacharacters (`\.+*?()\|[]{}^$#&-~`). |
| `escapeRegex` | `fn(input: string, out: []u8): usize` | Escapes metacharacters with `\` into `out`; returns bytes written. |
| `isWordByte` | `fn(b: u8): bool` | Returns `true` for ASCII word bytes `[A-Za-z0-9_]`. |
| `extractLiteralPrefix` | `fn(pattern: string, out: []u8): usize` | Extracts leading literal bytes (handling escapes) into `out`; returns bytes written. |

## Error Codes (`RegexSyntaxInfo.err_code`)

`0`: OK · `1`: Unclosed group · `2`: Unopened group · `3`: Unclosed class · `4`: Invalid class range · `5`: Empty class · `6`: Dangling quantifier · `7`: Double quantifier · `8`: Invalid quantifier · `9`: Trailing backslash · `10`: Invalid escape · `11`: Invalid group header · `12`: Nesting limit exceeded.

## License

MIT — Clean-room CP implementation with Rust `regex-syntax` v0.8 parity.
