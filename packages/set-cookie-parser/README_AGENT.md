# set-cookie-parser — AI Agent Quick Ref

Zero-allocation RFC 6265 Set-Cookie HTTP header parser and splitter (`v2.7.1`, `MIT`)

## Import

```cp
import {
    ParsedCookie,
    scpStripQuotes,
    parseSetCookie,
    splitCookiesCount,
    splitCookiesGet,
} from "set-cookie-parser";
```

## Core Types & Constants

```cp
class ParsedCookie { name: string, value: string, path: string, domain: string, /* +7 fields */ }
```

## API Signatures

```cp
fn scpStripQuotes(s: string): string;
fn parseSetCookie(header: string): ParsedCookie;
fn splitCookiesCount(combined: string): usize;
fn splitCookiesGet(combined: string, target_idx: usize): string?;
```

## Minimal Usage

```cp
import { parseSetCookie } from "set-cookie-parser";

pub fn main() {
    let result = parseSetCookie("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"set-cookie-parser"`; install via `cap add set-cookie-parser`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
