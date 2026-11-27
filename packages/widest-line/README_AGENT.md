# widest-line — AI Agent Quick Ref

Zero-allocation widest visual terminal line width calculator (npm widest-line parity) composing string-width for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    wlTrimCr,
    wlLineWidthWithTab,
    countLines,
    widestLine,
    widestLineWithOptions,
    lineWidths,
    lineWidthsWithOptions,
    widestLineWithTab,
} from "widest-line";
```

## API Signatures

```cp
fn wlTrimCr(line: string): string;
fn wlLineWidthWithTab(line: string, tab_width: usize): usize;
fn countLines(s: string): usize;
fn widestLine(s: string): usize;
fn widestLineWithOptions(s: string, opts: StringWidthOptions): usize;
fn lineWidths(s: string, out: []usize): usize;
fn lineWidthsWithOptions(s: string, opts: StringWidthOptions, out: []usize): usize;
fn widestLineWithTab(s: string, tab_width: usize): usize;
```

## Minimal Usage

```cp
import { wlTrimCr } from "widest-line";

pub fn main() {
    let result = wlTrimCr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"widest-line"`; install via `cap add widest-line`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
