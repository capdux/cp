# @cap/textwrap-crate

Zero-allocation smart word wrapping, indentation, dedentation, and paragraph unfilling engine for CP — clean-room MIT parity with crates.io `textwrap` v0.16.

Composes [`@cap/unicode-width`](../unicode-width) for accurate Unicode 15.1 / CJK / Emoji terminal display column width calculation with zero code duplication.

## Installation

```json
{
  "dependencies": {
    "@cap/textwrap-crate": "1.0.0"
  }
}
```

## Quick Start

```cp
import {
    fill,
    fillWithIndent,
    indent,
    dedent,
    unfill,
    lineDisplayWidth,
} from "@cap/textwrap-crate";

let buf: [512]u8 = undefined;
const n = fill("textwrap: a small library for wrapping text.", 18, buf[0..]);
const wrapped = buf[0..n];
// "textwrap: a small\nlibrary for\nwrapping text."
```

## API Reference

All text-producing functions are zero-heap-allocation: they write UTF-8 bytes into a caller-provided buffer `out: []u8` and return the number of bytes written (`usize`).

### `fill(text: string, width: usize, out: []u8): usize`

Wraps `text` at word boundaries so lines do not exceed `width` terminal display columns (measured via `@cap/unicode-width`). Preserves explicit line breaks and empty paragraph lines.

### `fillWithIndent(text: string, width: usize, initial_indent: string, subsequent_indent: string, out: []u8): usize`

Wraps `text` to at most `width` display columns, prepending `initial_indent` to the first line and `subsequent_indent` to all subsequent lines (accounting for indent display width).

### `indent(text: string, prefix: string, out: []u8): usize`

Prepends `prefix` to each non-blank line in `text`, leaving blank lines untouched while preserving line endings.

### `dedent(text: string, out: []u8): usize`

Strips the longest common leading whitespace prefix shared by all non-blank lines (matching Python `textwrap.dedent` and Rust `textwrap::dedent`). Whitespace-only lines are normalized to empty lines.

### `unfill(text: string, out: []u8): usize`

Reflows soft-wrapped lines within each paragraph back into a single space-separated line while preserving double-newline (`\n\n`) paragraph boundaries.

### `lineDisplayWidth(line: string): usize`

Returns the terminal display column width of `line` via `@cap/unicode-width` (`strWidth`), handling ASCII, CJK wide characters, Emoji, and zero-width combining marks.

## License

MIT
