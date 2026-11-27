# `regex-syntax` — AI Agent Quick Ref

Zero-allocation regex syntax validator, metacharacter escaper & literal prefix extractor (Rust `regex-syntax` v0.8 parity).

## Import

```cp
import {
    RSX_OK, RSX_ERR_UNCLOSED_GROUP, RSX_ERR_UNOPENED_GROUP,
    RSX_ERR_UNCLOSED_CLASS, RSX_ERR_INVALID_CLASS_RANGE, RSX_ERR_EMPTY_CLASS,
    RSX_ERR_DANGLING_QUANTIFIER, RSX_ERR_DOUBLE_QUANTIFIER, RSX_ERR_INVALID_QUANTIFIER,
    RSX_ERR_TRAILING_BACKSLASH, RSX_ERR_INVALID_ESCAPE,
    RSX_ERR_INVALID_GROUP_HEADER, RSX_ERR_NESTING_LIMIT,
    RegexSyntaxInfo,
    isMetaCharacter, escapeRegex, isWordByte, extractLiteralPrefix, validateRegexSyntax,
} from "regex-syntax";
```

## Core API Signatures

```cp
// Error codes (i32): RSX_OK = 0; RSX_ERR_UNCLOSED_GROUP = 1 .. RSX_ERR_NESTING_LIMIT = 12

struct RegexSyntaxInfo {
    valid: bool,
    err_code: i32,
    err_pos: usize,
    capture_count: usize,
    has_anchors: bool,
    has_backref: bool,
    min_len: usize,
    literal_prefix_len: usize,
    fn isOk(): bool;
}

fn isMetaCharacter(c: u8): bool;
fn escapeRegex(input: string, out: []u8): usize;         // worst-case output = 2 * input.len
fn isWordByte(b: u8): bool;                              // [A-Za-z0-9_]
fn extractLiteralPrefix(pattern: string, out: []u8): usize;
fn validateRegexSyntax(pattern: string): RegexSyntaxInfo;
```

## Minimal Example

```cp
import { validateRegexSyntax, escapeRegex, extractLiteralPrefix } from "regex-syntax";

const info = validateRegexSyntax("^api/v([0-9]+)/items");
if (info.isOk()) {
    let pre: [64]u8 = undefined;
    const n = extractLiteralPrefix("^api/v([0-9]+)/items", pre[0..64]); // "api/v"
}
let esc: [64]u8 = undefined;
const en = escapeRegex("a.b+c", esc[0..64]); // "a\\.b\\+c"
```

## Gotchas & Borrow Rules

- All functions are pure and zero-allocation; `out: []u8` must be a mutable slice (`let buf: [N]u8 = undefined`).
- Size `escapeRegex` output buffer to at least `2 * input.len` to avoid truncation when every byte is a metacharacter.
- Top-level alternation (e.g., `foo|bar`) yields `extractLiteralPrefix` length `0`.
- Use `info.isOk()` (checks `valid && err_code == 0`) rather than assuming `err_pos == 0` means valid.
