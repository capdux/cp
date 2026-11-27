# string-similarity — AI Agent Quick Ref

Zero-allocation Sorensen-Dice bigram string similarity coefficient and best-match finder for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    dcToLower,
    dcIsSpace,
    dcNormalize,
    dcEval,
    diceCoefficient,
    diceCaseSensitive,
    bestMatchIndex,
} from "string-similarity";
```

## API Signatures

```cp
fn dcToLower(c: u8): u8;
fn dcIsSpace(c: u8): bool;
fn dcNormalize(src: &[]u8, strip_ws_lower: bool, dst: []u8): usize;
fn dcEval(a: string, b: string, strip_ws_lower: bool): f64;
fn diceCoefficient(a: string, b: string): f64;
fn diceCaseSensitive(a: string, b: string): f64;
fn bestMatchIndex(target: string, candidates: &[]string, min_rating: f64): i64;
```

## Minimal Usage

```cp
import { dcEval } from "string-similarity";

pub fn main() {
    let result = dcEval("example", "example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"string-similarity"`; install via `cap add string-similarity`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
