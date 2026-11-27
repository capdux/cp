# quoted-printable — AI Agent Quick Ref

Zero-allocation RFC 2045 Section 6.7 MIME Quoted-Printable encoder and decoder with 76-col soft line breaks in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { isDirectPrintable, encode, decode } from "quoted-printable";
```

## API Signatures

```cp
fn isDirectPrintable(b: u8): bool;
fn encode(data: &[]u8, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { decode } from "quoted-printable";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"quoted-printable"`; install via `cap add quoted-printable`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add quoted-printable
cpc check src/main.cp
```
