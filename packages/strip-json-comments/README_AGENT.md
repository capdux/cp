# strip-json-comments — AI Agent Quick Ref

Zero-allocation JSONC comment and trailing-comma stripper with line/column-preserving whitespace mode (npm strip-json-comments parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StripOptions,
    isTrailingComma,
    stripComments,
    stripJsonc,
    stripCompact,
} from "strip-json-comments";
```

## Core Types & Constants

```cp
class StripOptions { preserve_whitespace: bool, strip_trailing_commas: bool }
```

## API Signatures

```cp
fn isTrailingComma(src: &[]u8, commaIdx: usize): bool;
fn stripComments(src: &[]u8, dst: []u8, opts: StripOptions): i64;
fn stripJsonc(src: &[]u8, dst: []u8): i64;
fn stripCompact(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isTrailingComma } from "strip-json-comments";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isTrailingComma(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strip-json-comments"`; install via `cap add strip-json-comments`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
