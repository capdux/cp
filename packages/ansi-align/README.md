[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ansi-align

Zero-allocation ANSI-aware multiline text alignment (left, center, right) with full Unicode / CJK visual width calculation, composing `string-width` and `widest-line`.

Aligned with npm [`ansi-align`](https://github.com/nexdrew/ansi-align) (ISC).

## Installation & Import

```bash
cap add ansi-align
```

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

## Core API

### Types & Enums

| Symbol | Definition | Description |
| --- | --- | --- |
| `AlignMode` | `enum { Left, Center, Right }` | Alignment strategy applied to each line. |
| `AlignOptions` | `{ mode: AlignMode, split: string, pad: string }` | Alignment configuration with `AlignOptions.default()` (`Center`, `"\n"`, `" "`). |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ansiVisibleWidth` | `(s: string): usize` | Computes visible terminal column width of `s`, ignoring ANSI escape sequences and handling CJK/emoji widths. |
| `ansiMaxLineWidth` | `(text: string, split: string): usize` | Computes the maximum visible column width across lines separated by `split`. |
| `alignAnsi` | `(text: string, mode: AlignMode, out: []u8): usize` | Aligns multiline `text` (split by `"\n"`, padded with `" "`) into `out`; returns bytes written. |
| `alignAnsiWithOptions` | `(text: string, opts: AlignOptions, out: []u8): usize` | Aligns `text` using custom `AlignOptions` (`mode`, `split`, `pad`); returns bytes written. |
| `alignAnsiLines` | `(lines: []const string, mode: AlignMode, pad: string, split: string, out: []u8): usize` | Aligns a slice of pre-split lines and joins them with `split` into `out`; returns bytes written. |
| `alignCenter` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Center, out)`. |
| `alignRight` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Right, out)`. |
| `alignLeft` | `(text: string, out: []u8): usize` | Shorthand for `alignAnsi(text, AlignMode.Left, out)` (no leading pad added). |

## Examples

### 1. Centering ANSI-Colored Text

```cp
import { print } from "cap:io";
import { alignCenter } from "ansi-align";

pub fn main() {
    let text = "\x1b[32mOK\x1b[0m\nLonger Status Line";
    let out: [256]u8 = undefined;
    let n = alignCenter(text, out[0..256]);
    print("{}\n", out[0..n]);
}
```

### 2. Custom Padding & Line Separator

```cp
import { print } from "cap:io";
import { AlignMode, AlignOptions, alignAnsiWithOptions } from "ansi-align";

pub fn main() {
    let opts = AlignOptions {
        mode: AlignMode.Right,
        split: "\r\n",
        pad: ".",
    };
    let out: [256]u8 = undefined;
    let n = alignAnsiWithOptions("hi\r\nhello world", opts, out[0..256]);
    print("{}\n", out[0..n]);
}
```

### 3. Aligning Pre-Split Line Slices

```cp
import { print } from "cap:io";
import { AlignMode, alignAnsiLines } from "ansi-align";

pub fn main() {
    let lines: [2]string = ["CP", "Zero-Alloc CLI"];
    let out: [128]u8 = undefined;
    let n = alignAnsiLines(lines[0..2], AlignMode.Center, " ", "\n", out[0..128]);
    print("{}\n", out[0..n]);
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: Writes aligned UTF-8 output directly into a caller-supplied `out: []u8` buffer and returns the written byte count `usize`, with no intermediate string arrays.
- **Unicode & CJK accurate**: Delegates visual width calculation to `string-width` and `widest-line`, accurately aligning ANSI SGR codes, East Asian full-width characters, and combining marks.
- **Immutable input borrows**: Source strings and `[]const string` slices are borrowed read-only without copying.
