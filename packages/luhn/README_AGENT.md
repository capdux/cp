# luhn — AI Agent Quick Ref

Zero-heap ISO/IEC 7812 Luhn Mod-10, Damm, and Verhoeff check digit algorithms in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    luhnValidate,
    luhnCheckDigit,
    dammCheckDigit,
    dammValidate,
    verhoeffCheckDigit,
    verhoeffValidate,
} from "luhn";
```

## API Signatures

```cp
fn luhnValidate(input: &[]u8): bool;
fn luhnCheckDigit(payload: &[]u8): i32;
fn dammCheckDigit(payload: &[]u8): i32;
fn dammValidate(input: &[]u8): bool;
fn verhoeffCheckDigit(payload: &[]u8): i32;
fn verhoeffValidate(input: &[]u8): bool;
```

## Minimal Usage

```cp
import { luhnValidate } from "luhn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = luhnValidate(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"luhn"`; install via `cap add luhn`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add luhn
cpc check src/main.cp
```
