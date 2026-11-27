# iban_validate — AI Agent Quick Ref

Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    countryLength,
    electronicFormat,
    friendlyFormat,
    mod97,
    isValid,
    computeCheckDigits,
} from "iban_validate";
```

## API Signatures

```cp
fn countryLength(c0: u8, c1: u8): usize;
fn electronicFormat(input: string, out: []u8): usize;
fn friendlyFormat(input: string, out: []u8): usize;
fn mod97(electronic: string): u32;
fn isValid(input: string): bool;
fn computeCheckDigits(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool;
```

## Minimal Usage

```cp
import { isValid } from "iban_validate";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"iban_validate"`; install via `cap add iban_validate`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
