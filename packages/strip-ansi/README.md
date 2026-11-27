[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strip-ansi

Zero-heap ECMA-48 / ANSI terminal escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter for CP, powered by the `anstyle-parse` VT100/ANSI state machine.

## Source Alignment

- **Upstream**: npm [`strip-ansi`](https://github.com/chalk/strip-ansi) (MIT), crates.io [`strip-ansi-escapes`](https://github.com/luser/strip-ansi-escapes) (MIT), npm [`string-length`](https://github.com/sindresorhus/string-length) (MIT)
- **CP Package**: `strip-ansi` v1.0.0 (clean-room MIT in pure CP, reusing `anstyle-parse`)
- **Design**: Zero heap allocations, real VT100/ANSI state machine (no regex backtracking), supports CSI (`\x1b[...m`), OSC (`\x1b]...\x07` / ST), and 2-byte ESC sequences.

## Installation & Import

```bash
cap add strip-ansi
```

```cp
import {
    hasAnsi,
    strippedLen,
    stripAnsi,
    visibleAsciiCount,
} from "strip-ansi";
```

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `hasAnsi` | `fn hasAnsi(input: string): bool` | Fast check returning `true` if `input` contains any ANSI escape sequence (re-exported from `anstyle-parse`) |
| `strippedLen` | `fn strippedLen(input: string): usize` | Exact visible byte length of `input` after stripping all ANSI escape sequences |
| `stripAnsi` | `fn stripAnsi(input: string, out: []u8): usize` | Strips all ANSI sequences into `out` and returns bytes written; returns `0` if `input` is empty or `out.len < strippedLen(input)` |
| `visibleAsciiCount` | `fn visibleAsciiCount(input: string): usize` | Counts visible UTF-8 scalar values (ASCII bytes + UTF-8 leading bytes) excluding ANSI escape sequences |

## Examples

### 1. Fast-Path Check & Stripping ANSI Codes

```cp
import { hasAnsi, strippedLen, stripAnsi } from "strip-ansi";
import { print } from "cap:io";

pub fn main() {
    const colored = "\x1b[31mError:\x1b[0m build failed";
    if (!hasAnsi(colored)) {
        print(colored);
        return;
    }
    let buf: [128]u8 = [0; 128];
    const n = stripAnsi(colored, buf[0..128]);
    print(buf[0..n] as string); // "Error: build failed"
}
```

### 2. Computing Visible Character Count for Alignment

```cp
import { visibleAsciiCount, strippedLen } from "strip-ansi";

pub fn measureColumn(styled: string): usize {
    const bytes = strippedLen(styled);       // visible UTF-8 byte count
    const chars = visibleAsciiCount(styled); // visible UTF-8 scalar count
    return chars;
}
```

### 3. Safe Buffer Sizing Before Stripping

```cp
import { strippedLen, stripAnsi } from "strip-ansi";

pub fn cleanLogLine(line: string, out: []u8): usize {
    if (strippedLen(line) > out.len) {
        return 0;
    }
    return stripAnsi(line, out);
}
```

## Differences from npm `strip-ansi` & Safety Highlights

- **State Machine Instead of Regex**: Reuses `anstyle-parse` VT100/ANSI state machine for O(n) streaming tokenization with zero ReDoS risk.
- **No Silent Truncation**: `stripAnsi` checks `strippedLen(input) <= out.len` up front and returns `0` if `out` is too small, preventing partial writes.
- **Zero-Heap Execution**: Uses a fixed stack token buffer (`[64]AnsiToken`) internally; no dynamic allocation anywhere.
