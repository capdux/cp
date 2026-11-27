[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `regex-syntax`

Zero-allocation regular expression syntax validator, metacharacter escaper, and literal prefix extractor in pure CP — a clean-room MIT counterpart to Rust [`regex-syntax`](https://github.com/rust-lang/regex) (v0.8 parity).

## Installation & Import

```bash
cap add regex-syntax
```

```cp
import {
    RSX_OK,
    RSX_ERR_UNCLOSED_GROUP,
    RSX_ERR_UNOPENED_GROUP,
    RSX_ERR_UNCLOSED_CLASS,
    RSX_ERR_INVALID_CLASS_RANGE,
    RSX_ERR_EMPTY_CLASS,
    RSX_ERR_DANGLING_QUANTIFIER,
    RSX_ERR_DOUBLE_QUANTIFIER,
    RSX_ERR_INVALID_QUANTIFIER,
    RSX_ERR_TRAILING_BACKSLASH,
    RSX_ERR_INVALID_ESCAPE,
    RSX_ERR_INVALID_GROUP_HEADER,
    RSX_ERR_NESTING_LIMIT,
    RegexSyntaxInfo,
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    extractLiteralPrefix,
    validateRegexSyntax,
} from "regex-syntax";
```

## Core API

### Error Code Constants (`i32`)

| Constant | Value | Meaning |
| :--- | :--- | :--- |
| `RSX_OK` | `0` | Valid regular expression syntax |
| `RSX_ERR_UNCLOSED_GROUP` / `RSX_ERR_UNOPENED_GROUP` | `1` / `2` | Unmatched `(` or `)` |
| `RSX_ERR_UNCLOSED_CLASS` / `RSX_ERR_INVALID_CLASS_RANGE` / `RSX_ERR_EMPTY_CLASS` | `3` / `4` / `5` | Unclosed `[...]`, inverted range (`z-a`), or empty `[]` |
| `RSX_ERR_DANGLING_QUANTIFIER` / `RSX_ERR_DOUBLE_QUANTIFIER` / `RSX_ERR_INVALID_QUANTIFIER` | `6` / `7` / `8` | Nothing to repeat, stacked quantifiers (`a**`), or malformed `{m,n}` |
| `RSX_ERR_TRAILING_BACKSLASH` / `RSX_ERR_INVALID_ESCAPE` | `9` / `10` | Dangling `\` at EOF or invalid escape sequence |
| `RSX_ERR_INVALID_GROUP_HEADER` / `RSX_ERR_NESTING_LIMIT` | `11` / `12` | Unsupported `(?...)` syntax or exceeded group nesting depth |

### Types & Functions

| Symbol | Signature / Fields | Description |
| :--- | :--- | :--- |
| `RegexSyntaxInfo` | `valid: bool`, `err_code: i32`, `err_pos: usize`, `capture_count: usize`, `has_anchors: bool`, `has_backref: bool`, `min_len: usize`, `literal_prefix_len: usize`; `isOk(): bool` | Complete syntax validation & static pattern analysis summary |
| `validateRegexSyntax` | `(pattern: string): RegexSyntaxInfo` | Validate pattern syntax, count capture groups, compute `min_len` & literal prefix length |
| `isMetaCharacter` | `(c: u8): bool` | Exact parity with `regex_syntax::is_meta_character` (`\ . + * ? ( ) \| [ ] { } ^ $ # & - ~`) |
| `escapeRegex` | `(input: string, out: []u8): usize` | Escape all regex metacharacters into `out`; returns bytes written |
| `isWordByte` | `(b: u8): bool` | Return `true` for ASCII word characters (`[A-Za-z0-9_]`) |
| `extractLiteralPrefix` | `(pattern: string, out: []u8): usize` | Extract mandatory leading literal bytes (for fast prefilter search) into `out` |

## Examples

### 1. Validating User-Supplied Patterns & Inspecting Metadata

```cp
import { print } from "cap:io";
import { validateRegexSyntax, RSX_ERR_UNCLOSED_GROUP } from "regex-syntax";

fn main() {
    const info = validateRegexSyntax("^user_([0-9]{2,4})@example\\.com$");
    if (info.isOk()) {
        print("captures:", info.capture_count, "anchors:", info.has_anchors);
        print("min_len:", info.min_len, "prefix_len:", info.literal_prefix_len);
    }

    const bad = validateRegexSyntax("(abc[0-9]+");
    if (!bad.isOk() && bad.err_code == RSX_ERR_UNCLOSED_GROUP) {
        print("unclosed group at byte:", bad.err_pos);
    }
}
```

### 2. Escaping Literals & Extracting Prefilter Prefixes

```cp
import { print } from "cap:io";
import { escapeRegex, extractLiteralPrefix } from "regex-syntax";

fn main() {
    let esc: [128]u8 = undefined;
    const n1 = escapeRegex("v1.2+beta (test)", esc[0..128]);
    print("escaped:", esc[0..n1]); // "v1\\.2\\+beta \\(test\\)"

    let pre: [64]u8 = undefined;
    const n2 = extractLiteralPrefix("https://[a-z]+\\.org", pre[0..64]);
    print("literal prefix:", pre[0..n2]); // "https://"
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: Unlike Rust `regex-syntax` (which constructs heap-allocated `Ast`/`Hir` trees), CP `regex-syntax` validates and analyzes patterns in a single stack-only pass with bounded nesting depth (`RSX_ERR_NESTING_LIMIT`).
- **Prefilter-Ready**: `extractLiteralPrefix` unescapes leading literals directly into caller buffers so search engines can run SIMD `memmem` prefilters before invoking regex matching.
- **Structured Error Diagnostics**: Returns exact byte offset `err_pos` and typed `RSX_ERR_*` code without exceptions.
