# kind-of — AI Agent Quick Ref

Fast zero-heap value & JSON/literal type classifier (clean-room CP implementation of npm kind-of) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    koIsSpace,
    koIsDigit,
    koEq,
    koStartsWith,
    koIsNumber,
    koIsBigInt,
    koIsIsoDate,
    kindOf,
} from "kind-of";
```

## API Signatures

```cp
fn koIsSpace(c: u8): bool;
fn koIsDigit(c: u8): bool;
fn koEq(s: string, start: usize, end: usize, lit: string): bool;
fn koStartsWith(s: string, start: usize, end: usize, prefix: string): bool;
fn koIsNumber(s: string, start: usize, end: usize): bool;
fn koIsBigInt(s: string, start: usize, end: usize): bool;
fn koIsIsoDate(s: string, start: usize, end: usize): bool;
fn kindOf(raw: string): u8;
fn kindName(kind: u8): string;
fn kindOfName(raw: string): string;
fn isPrimitive(raw: string): bool;
fn isPlainObject(raw: string): bool;
```

## Minimal Usage

```cp
import { isPrimitive } from "kind-of";

pub fn main() {
    let result = isPrimitive("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kind-of"`; install via `cap add kind-of`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
