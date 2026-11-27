# `strip-indent` — AI Agent Quick Reference

- **Package**: `strip-indent` (`1.0.0`, MIT)
- **Origin**: `npm:strip-indent` + `npm:min-indent`
- **Import**: `import { minIndent, stripIndent, stripIndentBy, stripIndentWith, minIndentWithTabWidth, indentLines, StripIndentOptions } from "strip-indent";`

## Types

```cp
class StripIndentOptions {
    tab_width: usize,
    trim_blank_lines: bool,
    fn defaults(): StripIndentOptions,
}
```

## Core Signatures

```cp
fn minIndent(s: string): usize
fn minIndentWithTabWidth(s: string, tab_width: usize): usize
fn stripIndent(s: string, out: []u8): usize
fn stripIndentInto(s: string, out: []u8): usize
fn stripIndentBy(s: string, n: usize, out: []u8): usize
fn stripIndentWith(s: string, opts: StripIndentOptions, out: []u8): usize
fn indentLines(s: string, count: usize, indent_char: u8, include_empty: bool, out: []u8): usize
fn siIsIndentChar(ch: u8): bool
fn siCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize
```

## Minimal Pattern

```cp
let out: [256]u8 = undefined;
let n = stripIndent("    line1\n      line2", out[0..256]);
let res = out[0..n] as string;
```

## Pitfalls

1. **Buffer sizing**: For `stripIndent*`, `out.len >= s.len` is always sufficient. For `indentLines`, allocate `s.len + line_count * count`.
2. **Whitespace-only lines**: Ignored when computing `minIndent`; set `opts.trim_blank_lines = true` in `StripIndentOptions` to clear trailing spaces on blank lines.
3. **Tab expansion**: `minIndent` counts raw indent characters (`' '` and `'\t'`); use `minIndentWithTabWidth` / `stripIndentWith` when tabs represent multiple columns.
4. **No in-place aliasing**: Pass a separate `out` buffer rather than aliasing `s`.
