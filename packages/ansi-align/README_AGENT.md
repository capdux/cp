# ansi-align — AI Agent Quick Ref

Zero-allocation ANSI & CJK-aware multiline text alignment in pure CP (npm `ansi-align` parity, composing `string-width` and `widest-line`).

## Import

```cp
import {
    AlignMode,
    AlignOptions,
    ansiVisibleWidth,
    ansiMaxLineWidth,
    alignAnsi,
    alignAnsiWithOptions,
    alignAnsiLines,
    alignCenter,
    alignRight,
    alignLeft,
} from "ansi-align";
```

## API Signatures

```cp
enum AlignMode { Left, Center, Right }
class AlignOptions {
    mode: AlignMode,
    split: string,
    pad: string,
    static fn default(): AlignOptions;
}

fn ansiVisibleWidth(s: string): usize;
fn ansiMaxLineWidth(text: string, split: string): usize;
fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize;
fn alignAnsiWithOptions(text: string, opts: AlignOptions, out: []u8): usize;
fn alignAnsiLines(lines: []const string, mode: AlignMode, pad: string, split: string, out: []u8): usize;
fn alignCenter(text: string, out: []u8): usize;
fn alignRight(text: string, out: []u8): usize;
fn alignLeft(text: string, out: []u8): usize;
```

## Minimal Example

```cp
import { alignCenter, AlignMode, alignAnsiLines } from "ansi-align";

let out: [256]u8 = undefined;
let n = alignCenter("\x1b[32mhi\x1b[0m\nhello world", out[0..256]);
// out[0..n] contains the centered ANSI-preserving text

let lines: [2]string = ["a", "abcd"];
let m = alignAnsiLines(lines[0..2], AlignMode.Right, " ", "\n", out[0..256]);
```

## Pitfalls & Borrowing Rules

- **Caller output buffer**: Allocate `out: []u8` large enough for `text.len + total_padding`; output truncates safely at `out.len` without panicking.
- **No trailing padding**: Only leading pad is prepended per line (matching npm `ansi-align`); `AlignMode.Left` copies lines unchanged.
- **Visual width vs byte length**: Padding counts are computed from terminal column width (`ansiVisibleWidth`), not byte length, so ANSI escape sequences and wide CJK characters align properly.
- **Immutability**: Input strings and `lines: []const string` are read-only borrows; only `out: []u8` is written.
