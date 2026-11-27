# convert_case — AI Agent Quick Ref

Zero-allocation 14-style string case converter (crates.io convert_case v0.6 parity) composing heck-case for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ccvReplaceUnderscoreWithSpace,
    ccvStripUnderscores,
    toFlatCase,
    toUpperFlatCase,
    toLowerWords,
    toUpperWords,
    toAlternatingCase,
    convertCase,
} from "convert_case";
```

## API Signatures

```cp
fn ccvReplaceUnderscoreWithSpace(out: []u8, len: usize);
fn ccvStripUnderscores(out: []u8, len: usize): usize;
fn toFlatCase(src: string, out: []u8): usize;
fn toUpperFlatCase(src: string, out: []u8): usize;
fn toLowerWords(src: string, out: []u8): usize;
fn toUpperWords(src: string, out: []u8): usize;
fn toAlternatingCase(src: string, out: []u8): usize;
fn convertCase(src: string, style: u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { toFlatCase } from "convert_case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = toFlatCase("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"convert_case"`; install via `cap add convert_case`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
