# normalize-line-endings — AI Agent Quick Reference

Zero-heap line ending detector, counter, and normalizer (`CRLF`/`CR` <-> `LF`, Apache-2.0).

## Import

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

## API Signatures

```cp
enum LineEnding {
    None,
    Lf,
    Crlf,
    Cr,
    Mixed,
}

fn lineEndingStr(le: LineEnding): string;
fn detectLineEnding(input: string): LineEnding;
fn countLineBreaks(input: string): usize;
fn countLines(input: string): usize;
fn needsNormalization(input: string, target: LineEnding): bool;
fn normalizedLen(input: string, target: LineEnding): usize;
fn normalizeToLf(input: string, out: []u8): usize;
fn normalizeToCrlf(input: string, out: []u8): usize;
fn normalizeToCr(input: string, out: []u8): usize;
fn normalizeLineEndings(input: string, target: LineEnding, out: []u8): usize;
```

## Minimal Example

```cp
import { LineEnding, normalizedLen, normalizeToLf, normalizeLineEndings } from "normalize-line-endings";

let buf: [64]u8 = [0; 64];
const n = normalizeToLf("a\r\nb\rc\n", buf[0..64]); // "a\nb\nc\n"

const need = normalizedLen("a\nb", .Crlf); // 4
const m = normalizeLineEndings("a\nb", .Crlf, buf[0..64]); // "a\r\nb"
```

## Gotchas & Borrow Rules

- `normalizeToCrlf` can expand output up to `2 * input.len` (when every byte is `\n` or `\r`); check `normalizedLen(input, .Crlf) <= out.len` to prevent silent truncation.
- `normalizeLineEndings(input, .None, out)` strips all line endings (`\r\n`, `\n`, `\r`).
- `needsNormalization(input, target)` returns `false` when `input` has no line breaks (`.None`) or already matches `target` — ideal for zero-copy fast paths.
- `countLines("")` returns `0`; trailing `\n` / `\r\n` does not add an extra line.
