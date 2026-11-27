# memchr — AI Agent Quick Ref

Pure CP zero-allocation SWAR-accelerated byte and substring search engine (Rust memchr v2 parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    mcBytesEq,
    memchr,
    memchr2,
    memchr3,
    memrchr,
    memrchr2,
    memrchr3,
    memchrCount,
} from "memchr";
```

## API Signatures

```cp
fn mcBytesEq(a: &[]u8, b: &[]u8): bool;
fn memchr(needle: u8, haystack: &[]u8): i64;
fn memchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
fn memchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
fn memrchr(needle: u8, haystack: &[]u8): i64;
fn memrchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
fn memchrCount(needle: u8, haystack: &[]u8): usize;
fn memchrFindAll(needle: u8, haystack: &[]u8, out_offsets: []usize): usize;
fn memmemFind(haystack: &[]u8, needle: &[]u8): i64;
fn memmemRfind(haystack: &[]u8, needle: &[]u8): i64;
fn countByte(needle: u8, haystack: &[]u8): usize;
```

## Minimal Usage

```cp
import { mcBytesEq } from "memchr";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = mcBytesEq(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"memchr"`; install via `cap add memchr`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
