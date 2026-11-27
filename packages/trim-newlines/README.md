[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# trim-newlines

Zero-allocation, zero-copy trimming of leading and trailing `\r` / `\n` newlines from strings for CP, matching npm [`trim-newlines`](https://github.com/sindresorhus/trim-newlines) v5.

## Source Alignment

- **Upstream**: npm `trim-newlines` v5 (`sindresorhus/trim-newlines`, MIT)
- **CP Package**: `trim-newlines` v1.0.0 (clean-room MIT in pure CP)
- **Design**: 100% zero-copy sub-slicing — trims only CR (`\r`, `0x0D`) and LF (`\n`, `0x0A`) bytes while preserving spaces, tabs, and inner newlines.

## Installation & Import

```bash
cap add trim-newlines
```

```cp
import {
    trimNewlines,
    trimNewlinesStart,
    trimNewlinesEnd,
    countLeadingNewlines,
    countTrailingNewlines,
} from "trim-newlines";
```

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `trimNewlines` | `fn trimNewlines(input: string): string` | Returns a zero-copy sub-slice with both leading and trailing `\r` and `\n` bytes removed |
| `trimNewlinesStart` | `fn trimNewlinesStart(input: string): string` | Returns a zero-copy sub-slice with all leading `\r` and `\n` bytes removed |
| `trimNewlinesEnd` | `fn trimNewlinesEnd(input: string): string` | Returns a zero-copy sub-slice with all trailing `\r` and `\n` bytes removed |
| `countLeadingNewlines` | `fn countLeadingNewlines(input: string): usize` | Counts leading `\r` / `\n` bytes |
| `countTrailingNewlines` | `fn countTrailingNewlines(input: string): usize` | Counts trailing `\r` / `\n` bytes |

## Examples

### 1. Trimming Leading, Trailing, or Both Ends

```cp
import { trimNewlines, trimNewlinesStart, trimNewlinesEnd } from "trim-newlines";
import { print } from "cap:io";

pub fn main() {
    print(trimNewlines("\r\n  hello\nworld  \r\n")); // "  hello\nworld  "
    print(trimNewlinesStart("\n\nunicorn\n"));       // "unicorn\n"
    print(trimNewlinesEnd("\nunicorn\r\n\n"));       // "\nunicorn"
}
```

### 2. Measuring Leading & Trailing Newline Bytes

```cp
import { countLeadingNewlines, countTrailingNewlines } from "trim-newlines";

pub fn inspectPadding(text: string): usize {
    const lead = countLeadingNewlines(text);   // "\r\nfoo\n" -> 2
    const trail = countTrailingNewlines(text); // "\r\nfoo\n" -> 1
    return lead + trail;
}
```

### 3. Safe Handling of All-Newline Strings

```cp
import { trimNewlines } from "trim-newlines";

pub fn isOnlyNewlines(text: string): bool {
    return trimNewlines(text).len == 0;
}
```

## Differences from npm `trim-newlines` & Safety Highlights

- **True Zero-Copy**: Returns borrowed sub-slices of `input` in O(k) time (proportional only to the number of leading/trailing newline bytes), with zero heap allocation.
- **Preserves Horizontal Whitespace**: Unlike generic `trim()`, spaces (`' '`) and tabs (`'\t'`) at either end are preserved untouched.
- **Extra Counters**: Adds `countLeadingNewlines` and `countTrailingNewlines` for line-offset tracking.
