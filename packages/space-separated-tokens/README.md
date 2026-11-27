[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# space-separated-tokens

Zero-allocation HTML/SVG space-separated and comma-separated token list parser, inspector, normalizer, and serializer.

Aligned with npm [`space-separated-tokens`](https://github.com/wooorm/space-separated-tokens) `2.0.2` (MIT).

## Installation & Import

```bash
cap add space-separated-tokens
```

```cp
import {
    parseSpaceTokens,
    parse,
    countTokens,
    tokenAt,
    hasSpaceToken,
    containsToken,
    stringifySpaceTokens,
    stringify,
    normalizeSpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
} from "space-separated-tokens";
```

## Core API

| Function | Signature | Description |
| --- | --- | --- |
| `parseSpaceTokens` / `parse` | `(input: string, out: []string): usize` | Splits `input` by ASCII whitespace into zero-copy token slices in `out`; returns token count written. |
| `countTokens` | `(input: string): usize` | Counts space-separated tokens in `input` without allocating an array. |
| `tokenAt` | `(input: string, index: usize): string?` | Returns the `index`-th token as a zero-copy sub-slice, or `null` if out of bounds. |
| `hasSpaceToken` / `containsToken` | `(input: string, target: string): bool` | Checks whether `target` exists as an exact token in `input` (like HTML `classList.contains`). |
| `stringifySpaceTokens` / `stringify` | `(tokens: []const string, out: []u8): usize` | Joins non-empty tokens with a single space `" "` into `out`; returns bytes written. |
| `normalizeSpaceTokens` | `(input: string, out: []u8): usize` | Trims and collapses repeated whitespace into single spaces in `out`; returns bytes written. |
| `parseCommaTokens` | `(input: string, out: []string): usize` | Parses comma-separated tokens (trimming surrounding whitespace) into `out`; returns token count. |
| `stringifyCommaTokens` | `(tokens: []const string, out: []u8): usize` | Joins tokens with `", "` into `out`; returns bytes written. |

## Examples

### 1. Parsing & Inspecting HTML `class` Attributes

```cp
import { print } from "cap:io";
import { parse, hasSpaceToken, tokenAt } from "space-separated-tokens";

pub fn main() {
    let classes = "  btn   btn-primary\tactive  ";
    let tokens: [8]string = undefined;
    let n = parse(classes, tokens[0..8]);
    print("count={}, has_active={}\n", n, hasSpaceToken(classes, "active"));

    let second = tokenAt(classes, 1);
    if (second != null) {
        print("token[1]={}\n", second!); // "btn-primary"
    }
}
```

### 2. Normalizing & Stringifying Token Lists

```cp
import { print } from "cap:io";
import { normalizeSpaceTokens, stringify } from "space-separated-tokens";

pub fn main() {
    let out: [128]u8 = undefined;
    let n = normalizeSpaceTokens("  foo\n\tbar   baz  ", out[0..128]);
    print("normalized: '{}'\n", out[0..n]); // "foo bar baz"

    let list: [3]string = ["alpha", "beta", "gamma"];
    let m = stringify(list[0..3], out[0..128]);
    print("stringified: '{}'\n", out[0..m]); // "alpha beta gamma"
}
```

### 3. Comma-Separated Tokens

```cp
import { print } from "cap:io";
import { parseCommaTokens, stringifyCommaTokens } from "space-separated-tokens";

pub fn main() {
    let items: [8]string = undefined;
    let n = parseCommaTokens("a, b , c", items[0..8]);
    let buf: [64]u8 = undefined;
    let m = stringifyCommaTokens(items[0..n], buf[0..64]);
    print("csv: '{}'\n", buf[0..m]); // "a, b, c"
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero-copy slicing**: `parse`, `parseCommaTokens`, and `tokenAt` return slices pointing directly into `input`—no heap allocation or string duplication.
- **No-array inspection**: `countTokens`, `tokenAt`, and `hasSpaceToken` scan `input` directly in O(n) time and O(1) memory.
- **Bonus comma-token helpers**: Includes `parseCommaTokens` and `stringifyCommaTokens` alongside the full npm `space-separated-tokens` API.
