# trim-newlines — AI Agent Quick Reference

Zero-allocation, zero-copy trimmer for leading and trailing `\r` / `\n` bytes (npm `trim-newlines` v5 parity).

## Import

```cp
import {
    trimNewlines,
    trimNewlinesStart,
    trimNewlinesEnd,
    countLeadingNewlines,
    countTrailingNewlines,
} from "trim-newlines";
```

## API Signatures

```cp
fn trimNewlines(input: string): string;
fn trimNewlinesStart(input: string): string;
fn trimNewlinesEnd(input: string): string;
fn countLeadingNewlines(input: string): usize;
fn countTrailingNewlines(input: string): usize;
```

## Minimal Example

```cp
import { trimNewlines, trimNewlinesStart, trimNewlinesEnd } from "trim-newlines";

const a = trimNewlines("\r\n  hello\nworld  \r\n"); // "  hello\nworld  "
const b = trimNewlinesStart("\n\nfoo\n");           // "foo\n"
const c = trimNewlinesEnd("\nfoo\r\n");             // "\nfoo"
```

## Behavior Details

- Only strips ASCII `\n` (`0x0A`) and `\r` (`0x0D`); spaces (`' '`), tabs (`'\t'`), and interior newlines are preserved.
- `countLeadingNewlines` and `countTrailingNewlines` count raw `\r` and `\n` bytes (so a single `\r\n` counts as `2` bytes).
- Empty or all-newline inputs safely return `""` (`len == 0`).

## Gotchas & Borrow Rules

- All three `trimNewlines*` functions return a borrowed sub-slice of `input` — no buffer is needed, and `input` must outlive the returned slice.
- Because `count*Newlines` counts bytes rather than CRLF pairs, `"\r\n"` returns `2`; use `normalize-line-endings` if you need logical line-break counts.
- All parameters are read-only `string` borrows; zero heap allocation.
