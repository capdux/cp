# idna — AI Agent Quick Ref

Zero-allocation UTS #46 Internationalized Domain Names (IDNA domainToAscii / domainToUnicode) reusing icu-normalizer for NFC and punycode for RFC 3492 Punycode (idna crate parity) in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    idnaIsLdhLower,
    idnaIsLdhAnyCase,
    isPunycodeLabel,
    idnaMapDomain,
    idnaValidateAsciiLabelLower,
    idnaAppendBytes,
    domainToAscii,
    domainToUnicode,
} from "idna";
```

## API Signatures

```cp
fn idnaIsLdhLower(ch: u8): bool;
fn idnaIsLdhAnyCase(ch: u8): bool;
fn isPunycodeLabel(label: string): bool;
fn idnaMapDomain(domain: string, out: []u8): i64;
fn idnaValidateAsciiLabelLower(label: &[]u8): i64;
fn idnaAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn domainToAscii(domain: string, out: []u8): i64;
fn domainToUnicode(domain: string, out: []u8): i64;
fn isValidIdnaAscii(domain: string): bool;
```

## Minimal Usage

```cp
import { isPunycodeLabel } from "idna";

pub fn main() {
    let result = isPunycodeLabel("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"idna"`; install via `cap add idna`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
