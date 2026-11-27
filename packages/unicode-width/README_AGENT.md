# unicode-width — AI Agent Quick Ref

Zero-allocation Unicode 15.1 East Asian Width & terminal display column width calculator with UTF-8 safe truncation for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    uwNextCodepoint,
    uwIsZeroWidth,
    uwIsWide,
    uwIsAmbiguous,
    charWidth,
    charWidthCjk,
    strWidth,
    strWidthCjk,
} from "unicode-width";
```

## API Signatures

```cp
fn uwNextCodepoint(s: &[]u8, pos: usize, next_pos: &mut usize): u32;
fn uwIsZeroWidth(cp: u32): bool;
fn uwIsWide(cp: u32): bool;
fn uwIsAmbiguous(cp: u32): bool;
fn charWidth(cp: u32): usize;
fn charWidthCjk(cp: u32): usize;
fn strWidth(s: string): usize;
fn strWidthCjk(s: string): usize;
fn truncateToWidth(s: string, max_cols: usize): usize;
```

## Minimal Usage

```cp
import { strWidth } from "unicode-width";

pub fn main() {
    let result = strWidth("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-width"`; install via `cap add unicode-width`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
