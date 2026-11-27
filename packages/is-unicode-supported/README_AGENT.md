# is-unicode-supported — AI Agent Quick Ref

Zero-allocation pure-logic terminal Unicode support detector (npm is-unicode-supported v2 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EnvInfo,
    iusStrEq,
    isUnicodeSupportedTerm,
    isUnicodeSupportedTermProgram,
    isUnicodeSupported,
    isUnicodeSupportedPlatform,
    selectFallback,
} from "is-unicode-supported";
```

## Core Types & Constants

```cp
class EnvInfo { is_windows: bool, ci: bool, wt_session: bool, terminus_sublime: bool, /* +4 fields */ } // methods: posix(...) +5
```

## API Signatures

```cp
fn iusStrEq(a: string, b: string): bool;
fn isUnicodeSupportedTerm(term: string): bool;
fn isUnicodeSupportedTermProgram(term_program: string): bool;
fn isUnicodeSupported(env: EnvInfo): bool;
fn isUnicodeSupportedPlatform(is_windows: bool, term: string): bool;
fn selectFallback(env: EnvInfo, unicode_val: string, ascii_val: string): string;
```

## Minimal Usage

```cp
import { isUnicodeSupportedTerm } from "is-unicode-supported";

pub fn main() {
    let result = isUnicodeSupportedTerm("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"is-unicode-supported"`; install via `cap add is-unicode-supported`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
