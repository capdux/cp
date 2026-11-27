# uuencode — AI Agent Quick Ref

Zero-heap POSIX/Unix uuencode and uudecode codec supporting 45-byte lines, backtick/space zero encoding, and begin/end frames. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    uuEncChar,
    uuDecVal,
    encodeLine,
    decodeLine,
    encode,
    decode,
} from "uuencode";
```

## API Signatures

```cp
fn uuEncChar(v: u8): u8;
fn uuDecVal(c: u8): i32;
fn encodeLine(data: &[]u8, out: []u8): usize;
fn decodeLine(line: string, out: []u8): i64;
fn encode(data: &[]u8, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { decodeLine } from "uuencode";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeLine("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"uuencode"`; install via `cap add uuencode`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add uuencode
cpc check src/main.cp
```
