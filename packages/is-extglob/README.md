[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-extglob

Zero-allocation Bash / Micromatch extended glob (`@(...)`, `!(...)`, `*(...)`, `+(...)`, `?(...)`) detector and span locator for CP, matching npm [`is-extglob`](https://github.com/jonschlinkert/is-extglob).

## Source Alignment

- **Upstream**: npm `is-extglob` (`jonschlinkert/is-extglob`, MIT)
- **CP Package**: `is-extglob` v1.0.0 (clean-room MIT in pure CP)
- **Design**: Zero heap allocations, single-pass balanced-parenthesis scanner respecting `\` escapes; exposes both boolean detection and structured `ExtglobSpan` / `ExtglobKind` inspection.

## Installation & Import

```bash
cap add is-extglob
```

```cp
import {
    ExtglobKind,
    ExtglobSpan,
    isExtglobPrefix,
    extglobKindFromByte,
    extglobPrefixByte,
    matchExtglobEnd,
    findExtglobFrom,
    findExtglob,
    findExtglobStart,
    isExtglob,
    hasNestedExtglob,
    isNegatedExtglob,
    countExtglobs,
} from "is-extglob";
```

## Core API

### Types

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `ExtglobKind` | `enum { Optional, ZeroOrMore, OneOrMore, ExactlyOne, Negated }` | Extglob operator kind (`?`, `*`, `+`, `@`, `!`) |
| `ExtglobSpan` | `struct { kind: ExtglobKind, start: usize, end: usize }` | Byte span `str[start..end]` (exclusive `end`) and operator kind of a matched extglob |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isExtglob` | `fn isExtglob(str: string): bool` | Returns `true` if `str` contains a valid unescaped extglob expression |
| `findExtglob` | `fn findExtglob(str: string): ExtglobSpan?` | Returns the first complete extglob span in `str`, or `null` |
| `findExtglobFrom` | `fn findExtglobFrom(str: string, start_pos: usize): ExtglobSpan?` | Finds the next complete extglob starting at or after `start_pos`, or `null` |
| `findExtglobStart` | `fn findExtglobStart(str: string): i64` | Returns byte index of the first extglob, or `-1` if none |
| `matchExtglobEnd` | `fn matchExtglobEnd(str: string, start: usize): usize` | Given extglob prefix at `start`, returns exclusive end index after matching `)`, or `0` if unmatched |
| `hasNestedExtglob` | `fn hasNestedExtglob(str: string): bool` | Returns `true` if `str` contains a nested extglob (e.g. `+(a\|!(b\|c))`) |
| `isNegatedExtglob` | `fn isNegatedExtglob(str: string): bool` | Returns `true` if `str` contains at least one negated extglob `!(...)` (including nested) |
| `countExtglobs` | `fn countExtglobs(str: string): usize` | Counts top-level complete extglob expressions in `str` |
| `isExtglobPrefix` | `fn isExtglobPrefix(ch: u8): bool` | Returns `true` if `ch` is one of `?`, `*`, `+`, `@`, `!` |
| `extglobKindFromByte` | `fn extglobKindFromByte(ch: u8): ExtglobKind?` | Maps operator byte (`?`/`*`/`+`/`@`/`!`) to `ExtglobKind`, or `null` |
| `extglobPrefixByte` | `fn extglobPrefixByte(kind: ExtglobKind): u8` | Maps `ExtglobKind` back to its ASCII operator byte |

## Examples

### 1. Detecting Extglob Patterns

```cp
import { isExtglob, isNegatedExtglob, hasNestedExtglob } from "is-extglob";

pub fn main() {
    const a = isExtglob("src/@(foo|bar).cp");        // true
    const b = isExtglob("src/\\@(foo|bar).cp");      // false (escaped)
    const c = isNegatedExtglob("!(node_modules)");   // true
    const d = hasNestedExtglob("+(a|!(b|c))");       // true
}
```

### 2. Iterating All Top-Level Extglob Spans

```cp
import { findExtglobFrom, ExtglobSpan } from "is-extglob";
import { print } from "cap:io";

pub fn listExtglobs(pattern: string) {
    let pos: usize = 0;
    while (pos < pattern.len) {
        if (const span = findExtglobFrom(pattern, pos)) {
            print(pattern[span.start..span.end]);
            pos = span.end;
        } else {
            break;
        }
    }
}
```

### 3. Inspecting Operator Kind

```cp
import { findExtglob, extglobPrefixByte } from "is-extglob";

pub fn firstOperator(pattern: string): u8 {
    if (const span = findExtglob(pattern)) {
        return extglobPrefixByte(span.kind);
    }
    return 0;
}
```

## Differences from npm `is-extglob` & Safety Highlights

- **No Regex Engine / Zero ReDoS**: Uses a linear, non-backtracking parenthesis-balancing scanner instead of regular expressions.
- **Rich Span Metadata**: Goes beyond boolean `isExtglob` by returning exact byte spans (`ExtglobSpan`), operator kinds (`ExtglobKind`), nesting checks, and counts.
- **Zero Heap Allocation**: All queries borrow `str: string` read-only and return stack values.
