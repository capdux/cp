[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `strip-indent`

> **Source Attribution**: `npm:strip-indent` ([sindresorhus/strip-indent](https://github.com/sindresorhus/strip-indent)) & `npm:min-indent` — License: MIT

Zero-allocation leading whitespace stripper, tab-width-aware indentation analyzer, and line indenter in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`strip-indent`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: strip-indent)
import stripIndent from "strip-indent";
const clean = stripIndent("\n    foo\n      bar\n");
```

```cp
// CP (zero-heap, direct buffer output)
import { stripIndent } from "strip-indent";

let buf: [128]u8 = undefined;
let n = stripIndent("\n    foo\n      bar\n", buf[0..128]);
let clean = buf[0..n] as string; // "\nfoo\n  bar\n"
```

## Installation

Add `strip-indent` to your `cap.json`:

```json
{
  "dependencies": {
    "strip-indent": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `StripIndentOptions` | `class { tab_width: usize, trim_blank_lines: bool, fn defaults(): StripIndentOptions }` | Options for tab expansion width and blank-line trimming |
| `minIndent` | `(s: string): usize` | Find minimum leading whitespace count across non-empty lines |
| `minIndentWithTabWidth` | `(s: string, tab_width: usize): usize` | Find minimum indentation treating `\t` as `tab_width` columns |
| `stripIndent` | `(s: string, out: []u8): usize` | Strip minimum common indentation from every line |
| `stripIndentInto` | `(s: string, out: []u8): usize` | Alias for `stripIndent` |
| `stripIndentBy` | `(s: string, n: usize, out: []u8): usize` | Strip up to `n` leading indent characters from each line |
| `stripIndentWith` | `(s: string, opts: StripIndentOptions, out: []u8): usize` | Strip indentation with configurable tab width & blank-line trimming |
| `indentLines` | `(s: string, count: usize, indent_char: u8, include_empty: bool, out: []u8): usize` | Add `count` copies of `indent_char` to each line |
| `siIsIndentChar` / `siCopyRange` | `(ch: u8): bool` / `(out: []u8, pos: usize, s: string, start: usize, end: usize): usize` | Low-level helpers |

## Practical Examples

### 1. Deduplicating Indentation & Inspecting `minIndent`

```cp
import { minIndent, stripIndent, stripIndentBy } from "strip-indent";

let src = "    fn main() {\n        return;\n    }";
let m = minIndent(src); // 4

let out: [128]u8 = undefined;
let n1 = stripIndent(src, out[0..128]);       // strips 4 spaces from every line
let n2 = stripIndentBy(src, 2, out[0..128]);  // strips only 2 spaces
```

### 2. Tab-Width Aware Stripping & Re-Indenting Lines

```cp
import { StripIndentOptions, minIndentWithTabWidth, stripIndentWith, indentLines } from "strip-indent";

let tabbed = "\tfoo\n\t  bar\n   \n\tbaz";
let cols = minIndentWithTabWidth(tabbed, 4); // 4

let opts = StripIndentOptions.defaults();
opts.tab_width = 4;
opts.trim_blank_lines = true;

let out: [256]u8 = undefined;
let n1 = stripIndentWith(tabbed, opts, out[0..256]);
let stripped = out[0..n1] as string;

// Re-indent non-empty lines by 2 spaces
let out2: [256]u8 = undefined;
let n2 = indentLines(stripped, 2, ' ', false, out2[0..256]);
```

## Notes & Edge Cases

- **Blank lines ignored**: Lines containing only spaces/tabs do not reduce `minIndent` to `0`, matching npm `strip-indent` / `min-indent`.
- **Line endings preserved**: `\n` and `\r\n` line endings are preserved intact.
- **Zero heap allocations**: All string-producing functions write into caller-provided `[]u8` buffers and return the written byte count.
