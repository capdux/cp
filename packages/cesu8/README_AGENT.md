# cesu8 — AI Agent Quick Ref

Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) to/from standard UTF-8 transcoder (`v1.1.0`, `MIT`)

## Import

```cp
import { toCesu8, fromCesu8, isValidCesu8AsciiOrBmp } from "cesu8";
```

## API Signatures

```cp
fn toCesu8(utf8: &[]u8, modified_nul: bool, out: []u8): i64;
fn fromCesu8(cesu: &[]u8, modified_nul: bool, out: []u8): i64;
fn isValidCesu8AsciiOrBmp(utf8: &[]u8, modified_nul: bool): bool;
```

## Minimal Usage

```cp
import { isValidCesu8AsciiOrBmp } from "cesu8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidCesu8AsciiOrBmp(out_buf[0..256], false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cesu8"`; install via `cap add cesu8`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add cesu8
cpc check src/main.cp
```
