# strip-ansi — AI Agent Quick Reference

Zero-heap ECMA-48 / ANSI escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter (npm `strip-ansi` parity).

## Import

```cp
import {
    hasAnsi,
    strippedLen,
    stripAnsi,
    visibleAsciiCount,
} from "strip-ansi";
```

## API Signatures

```cp
fn hasAnsi(input: string): bool;
fn strippedLen(input: string): usize;
fn stripAnsi(input: string, out: []u8): usize;
fn visibleAsciiCount(input: string): usize;
```

## Minimal Example

```cp
import { hasAnsi, strippedLen, stripAnsi, visibleAsciiCount } from "strip-ansi";

const s = "\x1b[1;32mOK\x1b[0m done";
if (hasAnsi(s)) {
    const need = strippedLen(s);       // 7
    let buf: [64]u8 = [0; 64];
    const n = stripAnsi(s, buf[0..64]); // 7 -> "OK done"
    const chars = visibleAsciiCount(s); // 7
}
```

## Behavior Details

- Built on `anstyle-parse` VT100/ANSI state machine: strips CSI (`\x1b[...`), OSC (`\x1b]...`), and 2-byte ESC sequences in O(n) time.
- `strippedLen(input)` returns the exact visible UTF-8 byte count (always `<= input.len`).
- `visibleAsciiCount(input)` counts visible UTF-8 code points (all non-continuation bytes `(b & 0xC0) != 0x80` outside escape tokens).

## Gotchas & Borrow Rules

- `stripAnsi` returns `0` (writes nothing) if `out.len < strippedLen(input)` or `input.len == 0` — always check `out.len >= strippedLen(input)` (or `out.len >= input.len`).
- Use `hasAnsi(input)` first as a zero-copy fast path to skip buffering when no ANSI codes exist.
- `input` is a read-only `string` borrow; `out: []u8` is caller-owned mutable output.
- `visibleAsciiCount` counts UTF-8 scalar values, not East-Asian double-width terminal columns (use a display-width package for CJK/emoji column width).
