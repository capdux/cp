# progress — AI Agent Quick Ref

Zero-heap terminal progress bar renderer supporting :bar, :percent, :current, :total, :eta, and :rate tokens (`v2.0.3`, `MIT`)

## Import

```cp
import { ProgressBar, newProgressBar, prgWriteBar, renderProgress } from "progress";
```

## Core Types & Constants

```cp
class ProgressBar { total: u64, current: u64, width: usize, complete_char: u8, /* +3 fields */ } // methods: ratio(...) +4
```

## API Signatures

```cp
fn newProgressBar(total: u64, width: usize): ProgressBar;
fn prgWriteBar(bar: ProgressBar, out: []u8, pos: usize): usize;
fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { renderProgress } from "progress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderProgress(/* bar: ProgressBar */, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"progress"`; install via `cap add progress`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
