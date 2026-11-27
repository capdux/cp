# indoc — AI Agent Quick Ref

Zero-allocation multiline string dedent, indent, stripMargin, and newline normalization engine for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isHws,
    isLineBlank,
    appendByte,
    stripOuterSingleBlankLines,
    trimBlankLines,
    minCommonIndent,
    dedent,
    indent,
} from "indoc";
```

## API Signatures

```cp
fn isHws(c: u8): bool;
fn isLineBlank(s: string, start: usize, end: usize): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn stripOuterSingleBlankLines(input: string): string;
fn trimBlankLines(input: string): string;
fn minCommonIndent(input: string): usize;
fn dedent(input: string, out: []u8): string;
fn indent(input: string, prefix: string, skip_empty: bool, out: []u8): string;
fn stripMargin(input: string, margin_char: u8, out: []u8): string;
fn normalizeNewlines(input: string, out: []u8): string;
```

## Minimal Usage

```cp
import { isLineBlank } from "indoc";

pub fn main() {
    let result = isLineBlank("example", 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"indoc"`; install via `cap add indoc`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
