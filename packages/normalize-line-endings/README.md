[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# normalize-line-endings

Zero-heap line ending detector, counter, and normalizer (`CRLF` / `CR` <-> `LF`) for CP, inspired by crates.io [`normalize-line-endings`](https://github.com/derekdreery/normalize-line-endings) v0.3.0.

## Source Alignment

- **Upstream**: crates.io `normalize-line-endings` v0.3.0 (`derekdreery/normalize-line-endings`, Apache-2.0)
- **CP Package**: `normalize-line-endings` v0.3.0 (clean-room Apache-2.0 in pure CP)
- **Design**: Zero heap allocations; supports detection (`None`, `Lf`, `Crlf`, `Cr`, `Mixed`), exact output pre-sizing, and conversion to `LF`, `CRLF`, `CR`, or stripping (`None`).

## Installation & Import

```bash
cap add normalize-line-endings
```

```cp
import {
    LineEnding,
    lineEndingStr,
    detectLineEnding,
    countLineBreaks,
    countLines,
    needsNormalization,
    normalizedLen,
    normalizeToLf,
    normalizeToCrlf,
    normalizeToCr,
    normalizeLineEndings,
} from "normalize-line-endings";
```

## Core API

### Enum

| Symbol | Variants | Description |
| :--- | :--- | :--- |
| `LineEnding` | `None \| Lf \| Crlf \| Cr \| Mixed` | Detected or target line ending style |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `lineEndingStr` | `fn lineEndingStr(le: LineEnding): string` | Returns canonical sequence (`"\n"`, `"\r\n"`, `"\r"`, or `""` for `None`/`Mixed`) |
| `detectLineEnding` | `fn detectLineEnding(input: string): LineEnding` | Scans `input` and returns `.None`, `.Lf`, `.Crlf`, `.Cr`, or `.Mixed` |
| `countLineBreaks` | `fn countLineBreaks(input: string): usize` | Counts logical line breaks (`\r\n` counts as 1; lone `\r` or `\n` counts as 1) |
| `countLines` | `fn countLines(input: string): usize` | Counts logical lines (Rust `str::lines().count()` semantics: empty -> `0`, trailing break does not add an extra line) |
| `needsNormalization` | `fn needsNormalization(input: string, target: LineEnding): bool` | Returns `true` if `input` contains any line break differing from `target` |
| `normalizedLen` | `fn normalizedLen(input: string, target: LineEnding): usize` | Exact byte length needed when normalizing `input` to `target` |
| `normalizeToLf` | `fn normalizeToLf(input: string, out: []u8): usize` | Converts all `\r\n` and lone `\r` to `\n` into `out`; returns bytes written |
| `normalizeToCrlf` | `fn normalizeToCrlf(input: string, out: []u8): usize` | Converts all `\n`, `\r\n`, and lone `\r` to `\r\n` into `out`; returns bytes written |
| `normalizeToCr` | `fn normalizeToCr(input: string, out: []u8): usize` | Converts all `\r\n` and `\n` to `\r` into `out`; returns bytes written |
| `normalizeLineEndings` | `fn normalizeLineEndings(input: string, target: LineEnding, out: []u8): usize` | Dispatches normalization to `.Lf`, `.Crlf`, `.Cr`, or `.None` (strips all line breaks) |

## Examples

### 1. Fast-Path Check & Normalize to LF

```cp
import { LineEnding, needsNormalization, normalizeToLf } from "normalize-line-endings";
import { print } from "cap:io";

pub fn main() {
    const raw = "line1\r\nline2\rline3\n";
    if (!needsNormalization(raw, .Lf)) {
        print(raw);
        return;
    }
    let buf: [128]u8 = [0; 128];
    const n = normalizeToLf(raw, buf[0..128]);
    print(buf[0..n] as string); // "line1\nline2\nline3\n"
}
```

### 2. Pre-Sizing Before CRLF Expansion

```cp
import { LineEnding, normalizedLen, normalizeToCrlf } from "normalize-line-endings";

pub fn toWindowsText(input: string, out: []u8): usize {
    const need = normalizedLen(input, .Crlf);
    if (need > out.len) {
        return 0;
    }
    return normalizeToCrlf(input, out);
}
```

### 3. Detecting & Counting Line Endings

```cp
import { detectLineEnding, countLineBreaks, countLines } from "normalize-line-endings";

pub fn inspect(text: string): usize {
    const style = detectLineEnding(text);   // .None / .Lf / .Crlf / .Cr / .Mixed
    const breaks = countLineBreaks(text);
    const lines = countLines(text);
    return lines;
}
```

## Differences from crates.io `normalize-line-endings` & Safety Highlights

- **Bidirectional Normalization**: In addition to normalizing to `LF`, CP supports `CRLF`, `CR`, and stripping (`.None`), plus `detectLineEnding`, `countLineBreaks`, and `countLines`.
- **Zero-Heap Buffer Output**: Writes directly into caller-owned `[]u8` buffers without iterator-allocation overhead.
- **Exact Expansion Sizing**: Use `normalizedLen(input, .Crlf)` before converting `LF` to `CRLF` (where output can grow up to `2 * input.len`).
