# rot13-cipher — AI Agent Quick Ref

Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    shiftLetter,
    rot13,
    rot13InPlace,
    rot47,
    caesar,
    atbash,
} from "rot13-cipher";
```

## API Signatures

```cp
fn shiftLetter(c: u8, norm_shift: u8): u8;
fn rot13(input: string, out: []u8): usize;
fn rot13InPlace(buf: []u8);
fn rot47(input: string, out: []u8): usize;
fn caesar(input: string, shift: i32, out: []u8): usize;
fn atbash(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { rot13 } from "rot13-cipher";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = rot13("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rot13-cipher"`; install via `cap add rot13-cipher`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add rot13-cipher
cpc check src/main.cp
```
