# indent-string — AI Agent Quick Reference

Zero-heap multi-line string indenter matching npm `indent-string`.

## Import

```cp
import {
    indentString,
    indentStringWith,
} from "indent-string";
```

## API Signatures

```cp
fn indentString(input: string, count: usize, out: []u8): usize;
fn indentStringWith(
    input: string,
    count: usize,
    indent_unit: string,
    include_empty_lines: bool,
    out: []u8,
): usize;
```

## Minimal Example

```cp
import { indentString, indentStringWith } from "indent-string";

let buf: [128]u8 = [0; 128];
const n1 = indentString("foo\n\nbar", 2, buf[0..128]);
// "  foo\n\n  bar"

const n2 = indentStringWith("foo\n\nbar", 1, "> ", true, buf[0..128]);
// "> foo\n> \n> bar"
```

## Behavior Details

- `indentString(input, count, out)` is shorthand for `indentStringWith(input, count, " ", false, out)`.
- Lines are delimited by `\n` (byte `10`); lines containing only `' '` (32), `'\t'` (9), or `'\r'` (13) are considered blank.
- Empty `input` (`input.len == 0`) immediately returns `0`.
- When `count == 0` or `indent_unit.len == 0`, `input` is copied unchanged into `out`.

## Gotchas & Borrow Rules

- Output grows by `count * indent_unit.len` bytes per indented line; size `out` generously (e.g. `input.len + line_count * count * indent_unit.len`) because writes stop silently at `out.len`.
- `input` and `indent_unit` are read-only `string` borrows; `out: []u8` is caller-owned mutable output.
- Do not pass overlapping slices for `input` and `out` (in-place indentation is not supported).
