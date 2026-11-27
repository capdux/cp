# regex-syntax — AI Agent Quick Reference

- **Package**: `regex-syntax` (`v1.0.0`) | **Source**: crates.io `regex-syntax` v0.8 | **Import**: `import { ... } from "regex-syntax";`
- **Role**: Zero-heap regex syntax validator, metacharacter escaper, and static literal prefix extractor.

## Core Exports (`src/index.d.cp`)

```cp
class RegexSyntaxInfo {
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
fn escapeRegex(input: string, out: []u8): usize;
fn isWordByte(b: u8): bool;
fn extractLiteralPrefix(pattern: string, out: []u8): usize;
fn validateRegexSyntax(pattern: string): RegexSyntaxInfo;
```

## Minimal Usage

```cp
import { validateRegexSyntax, escapeRegex, extractLiteralPrefix, isMetaCharacter } from "regex-syntax";

const info = validateRegexSyntax("^foo([0-9]+)$");
if (info.isOk()) { /* capture_count=1, has_anchors=true */ }

let buf: [64]u8 = undefined;
const n = escapeRegex("a.b+c?", buf[0..64]);            // "a\\.b\\+c\\?"
const p = extractLiteralPrefix("foo\\.bar[0-9]+", buf[0..64]); // "foo.bar"
```

## Pitfalls & Constraints

1. **Error codes (`err_code`)**: `0`=OK, `1`=UNCLOSED_GROUP, `2`=UNOPENED_GROUP, `3`=UNCLOSED_CLASS, `4`=INVALID_CLASS_RANGE, `5`=EMPTY_CLASS, `6`=DANGLING_QUANTIFIER, `7`=DOUBLE_QUANTIFIER, `8`=INVALID_QUANTIFIER, `9`=TRAILING_BACKSLASH, `10`=INVALID_ESCAPE, `11`=INVALID_GROUP_HEADER, `12`=NESTING_LIMIT.
2. **Buffer truncation**: `escapeRegex` needs up to `2 * input.len` bytes in `out`; it stops cleanly without partial `\` escapes if `out` is full.
3. **Top-level alternation**: `extractLiteralPrefix` returns `0` when the pattern has top-level `|` alternation (no single guaranteed prefix).
4. **Private helpers**: Functions prefixed with `rsx*` in `index.d.cp` are internal parser helpers — prefer the 5 public APIs above.
5. **Validation only**: This package parses/validates syntax and extracts metadata; it does not execute regex matching.
