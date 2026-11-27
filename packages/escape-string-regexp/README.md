[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# escape-string-regexp

Zero-heap, single-pass RegExp metacharacter escaper and unescaper for CP, matching npm [`escape-string-regexp`](https://github.com/sindresorhus/escape-string-regexp) v5 (350M+/wk).

## Source Alignment

- **Upstream**: npm `escape-string-regexp` v5 (`sindresorhus/escape-string-regexp`, MIT)
- **CP Package**: `escape-string-regexp` v1.0.0 (clean-room MIT in pure CP)
- **Rules**: Escapes `| \ { } ( ) [ ] ^ $ + * ? .` with `\` and escapes `-` as `\x2d` so the output is also safe inside `[...]` character classes.

## Installation & Import

```bash
cap add escape-string-regexp
```

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

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `escapeStringRegexp` | `fn escapeStringRegexp(input: string, out: []u8): usize` | Escapes RegExp metacharacters into `out` (`-` -> `\x2d`); returns bytes written |
| `escapeStringRegexpChecked` | `fn escapeStringRegexpChecked(input: string, out: []u8): usize?` | Checked variant returning `null` if `out.len < escapedRegexpLen(input)` instead of truncating |
| `escapedRegexpLen` | `fn escapedRegexpLen(input: string): usize` | Computes exact byte length produced by `escapeStringRegexp` |
| `escapedLen` | `fn escapedLen(input: string): usize` | Alias for `escapedRegexpLen` |
| `escapeCharClass` | `fn escapeCharClass(input: string, out: []u8): usize` | Escapes character-class specials (`\`, `]`, `^`, and `-` as `\x2d`) |
| `unescapeStringRegexp` | `fn unescapeStringRegexp(escaped: string, out: []u8): usize` | Reverses `escapeStringRegexp` escaping (`\x2d` -> `-`, `\<ch>` -> `<ch>`) |
| `unescapeRegexp` | `fn unescapeRegexp(escaped: string, out: []u8): usize` | Alias for `unescapeStringRegexp` |
| `unescapedLen` | `fn unescapedLen(escaped: string): usize` | Computes exact byte length produced by `unescapeStringRegexp` |
| `hasRegexpMeta` | `fn hasRegexpMeta(input: string): bool` | Returns `true` if `input` contains any RegExp metacharacter or `-` |
| `needsRegexEscape` | `fn needsRegexEscape(input: string): bool` | Alias for `hasRegexpMeta` |
| `isRegexpMeta` | `fn isRegexpMeta(c: u8): bool` | Returns `true` if byte `c` is a RegExp metacharacter or `-` |
| `esrIsSpecial` | `fn esrIsSpecial(c: u8): bool` | Returns `true` if byte `c` is one of `| \ { } ( ) [ ] ^ $ + * ? .` (excluding `-`) |

## Examples

### 1. Checked Escaping with Fast-Path Skip

```cp
import { hasRegexpMeta, escapeStringRegexpChecked } from "escape-string-regexp";
import { print } from "cap:io";

pub fn main() {
    const raw = "How much $ for a foo-bar?";
    if (!hasRegexpMeta(raw)) {
        print(raw);
        return;
    }
    let buf: [128]u8 = [0; 128];
    if (const n = escapeStringRegexpChecked(raw, buf[0..128])) {
        print(buf[0..n] as string);
        // "How much \$ for a foo\x2dbar\?"
    }
}
```

### 2. Escaping Inside Character Classes `[...]`

```cp
import { escapeCharClass } from "escape-string-regexp";

pub fn buildCharClass(chars: string, out: []u8): usize {
    return escapeCharClass(chars, out); // "a-z^]" -> "a\x2dz\^\]"
}
```

### 3. Round-Trip Unescaping

```cp
import { escapeStringRegexp, unescapeStringRegexp } from "escape-string-regexp";

pub fn roundTrip(input: string): usize {
    let esc: [256]u8 = [0; 256];
    let back: [256]u8 = [0; 256];
    const n1 = escapeStringRegexp(input, esc[0..256]);
    return unescapeStringRegexp(esc[0..n1] as string, back[0..256]);
}
```

## Differences from npm `escape-string-regexp` & Safety Highlights

- **Zero-Heap Buffer Model**: Writes directly into caller-owned `[]u8` buffers with O(n) single-pass scanning.
- **Overflow-Safe Checked API**: `escapeStringRegexpChecked` returns `null` (`usize?`) if the destination buffer is too small, preventing silent partial escapes.
- **Round-Trip & Char-Class Extras**: Adds `escapeCharClass`, `unescapeStringRegexp`, `hasRegexpMeta`, and exact pre-sizing helpers (`escapedRegexpLen`, `unescapedLen`).
