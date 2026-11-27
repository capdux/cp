# escape-string-regexp — AI Agent Quick Reference

Zero-heap RegExp special character escaper and unescaper matching npm `escape-string-regexp` v5.

## Import

```cp
import {
    esrIsSpecial,
    isRegexpMeta,
    hasRegexpMeta,
    needsRegexEscape,
    escapedRegexpLen,
    escapedLen,
    escapeStringRegexp,
    escapeStringRegexpChecked,
    escapeCharClass,
    unescapedLen,
    unescapeStringRegexp,
    unescapeRegexp,
} from "escape-string-regexp";
```

## API Signatures

```cp
fn esrIsSpecial(c: u8): bool;
fn isRegexpMeta(c: u8): bool;
fn hasRegexpMeta(input: string): bool;
fn needsRegexEscape(input: string): bool;
fn escapedRegexpLen(input: string): usize;
fn escapedLen(input: string): usize;
fn escapeStringRegexp(input: string, out: []u8): usize;
fn escapeStringRegexpChecked(input: string, out: []u8): usize?;
fn escapeCharClass(input: string, out: []u8): usize;
fn unescapedLen(escaped: string): usize;
fn unescapeStringRegexp(escaped: string, out: []u8): usize;
fn unescapeRegexp(escaped: string, out: []u8): usize;
```

## Minimal Example

```cp
import { hasRegexpMeta, escapeStringRegexpChecked } from "escape-string-regexp";

const src = "a+b-c?";
let buf: [64]u8 = [0; 64];
if (hasRegexpMeta(src)) {
    if (const n = escapeStringRegexpChecked(src, buf[0..64])) {
        const escaped = buf[0..n] as string; // "a\+b\x2dc\?"
    }
}
```

## Gotchas & Borrow Rules

- `-` expands from 1 byte to 4 bytes (`\x2d`); worst-case output size is `4 * input.len` — use `escapedRegexpLen(input)` or `escapeStringRegexpChecked(input, out)` to avoid silent truncation.
- `escapeStringRegexp` writes into caller-owned `out: []u8` and returns written byte count; `escapeStringRegexpChecked` returns `null` if `out.len` is insufficient.
- Use `hasRegexpMeta(input)` as a zero-copy fast path to skip escaping when `false`.
- `esrIsSpecial(c)` checks `| \ { } ( ) [ ] ^ $ + * ? .` only (excludes `-`), whereas `isRegexpMeta(c)` includes `-`.
