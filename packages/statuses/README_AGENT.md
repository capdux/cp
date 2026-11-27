# statuses — AI Agent Quick Ref

Zero-heap RFC 9110 / IANA HTTP status code lookup, reason phrases, reverse lookup, and category predicates (npm statuses parity). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    message,
    codeFromMessage,
    isValid,
    isInformational,
    isSuccess,
    isRedirect,
    isEmpty,
    isRetry,
} from "statuses";
```

## API Signatures

```cp
fn message(code: u16): string;
fn codeFromMessage(msg: string): u16;
fn isValid(code: u16): bool;
fn isInformational(code: u16): bool;
fn isSuccess(code: u16): bool;
fn isRedirect(code: u16): bool;
fn isEmpty(code: u16): bool;
fn isRetry(code: u16): bool;
fn isClientError(code: u16): bool;
fn isServerError(code: u16): bool;
```

## Minimal Usage

```cp
import { codeFromMessage } from "statuses";

pub fn main() {
    let result = codeFromMessage("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"statuses"`; install via `cap add statuses`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
