# esutils — AI Agent Quick Ref

Zero-allocation ECMAScript keyword, reserved word, identifier, whitespace, and line terminator utilities (clean-room MIT implementation of npm esutils, #201 / 215M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isDecimalDigit,
    isHexDigit,
    isOctalDigit,
    isWhiteSpace,
    isLineTerminator,
    isIdentifierStartES6,
    isIdentifierPartES6,
    isKeywordES6,
} from "esutils";
```

## API Signatures

```cp
fn isDecimalDigit(cp: u32): bool;
fn isHexDigit(cp: u32): bool;
fn isOctalDigit(cp: u32): bool;
fn isWhiteSpace(cp: u32): bool;
fn isLineTerminator(cp: u32): bool;
fn isIdentifierStartES6(cp: u32): bool;
fn isIdentifierPartES6(cp: u32): bool;
fn isKeywordES6(id: string, strict: bool): bool;
fn isReservedWordES6(id: string, strict: bool): bool;
fn isRestrictedWord(id: string): bool;
fn isIdentifierNameES6(id: string): bool;
fn isIdentifierES6(id: string, strict: bool): bool;
```

## Minimal Usage

```cp
import { isKeywordES6 } from "esutils";

pub fn main() {
    let result = isKeywordES6("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"esutils"`; install via `cap add esutils`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
