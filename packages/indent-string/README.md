[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# indent-string

Zero-heap multi-line string indenter for CP, matching npm [`indent-string`](https://github.com/sindresorhus/indent-string) (180M+/wk).

## Source Alignment

- **Upstream**: npm `indent-string` (`sindresorhus/indent-string`, MIT)
- **CP Package**: `indent-string` v1.0.0 (clean-room MIT in pure CP)
- **Design**: Zero heap allocations, single-pass line-by-line indenter writing into caller-owned `[]u8` buffers, supporting custom indent units (`" "`, `"\t"`, `"> "`, etc.) and optional blank-line indentation.

## Installation & Import

```bash
cap add indent-string
```

```cp
import {
    indentString,
    indentStringWith,
} from "indent-string";
```

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `indentString` | `fn indentString(input: string, count: usize, out: []u8): usize` | Indents every non-blank line in `input` by `count` spaces (`" "`) into `out`; returns bytes written |
| `indentStringWith` | `fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize` | Indents each line by `count` repetitions of `indent_unit`; when `include_empty_lines` is `false`, blank / whitespace-only (`' '`, `'\t'`, `'\r'`) lines are left unindented |

## Examples

### 1. Default Space Indentation

```cp
import { indentString } from "indent-string";
import { print } from "cap:io";

pub fn main() {
    let buf: [128]u8 = [0; 128];
    const n = indentString("Unicorns\nRainbows", 4, buf[0..128]);
    print(buf[0..n] as string);
    // "    Unicorns\n    Rainbows"
}
```

### 2. Custom Prefix & Including Empty Lines

```cp
import { indentStringWith } from "indent-string";

pub fn quoteBlock(text: string, out: []u8): usize {
    return indentStringWith(text, 1, "> ", true, out);
    // "line1\n\nline2" -> "> line1\n> \n> line2"
}
```

### 3. Skipping Whitespace-Only Lines

```cp
import { indentStringWith } from "indent-string";

pub fn indentCode(code: string, out: []u8): usize {
    // Tabs with include_empty_lines = false leaves blank lines untouched
    return indentStringWith(code, 1, "\t", false, out);
}
```

## Differences from npm `indent-string` & Safety Highlights

- **Zero-Heap Buffer Output**: Writes directly into caller-owned `[]u8` slices in O(n) time without splitting lines into temporary arrays or running regular expressions.
- **Bounded Writes**: Every write checks `p < out.len` so undersized buffers never cause out-of-bounds access.
- **Line Delimiter**: Splits lines on `\n` (ASCII `10`) and treats lines containing only `' '`, `'\t'`, and `'\r'` as blank.
