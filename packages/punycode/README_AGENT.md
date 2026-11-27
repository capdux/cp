# punycode — AI Agent Quick Ref

Zero-allocation RFC 3492 Punycode / IDNA (xn--...) encoder and decoder for internationalized domain names in CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    pcToLower,
    pcIsXnPrefix,
    isAsciiOnly,
    pcAdapt,
    pcEncodeDigit,
    pcDecodeDigit,
    pcThreshold,
    pcDecodeUtf8,
} from "punycode";
```

## API Signatures

```cp
fn pcToLower(c: u8): u8;
fn pcIsXnPrefix(s: &[]u8): bool;
fn isAsciiOnly(s: string): bool;
fn pcAdapt(delta_in: u32, numpoints: u32, firsttime: bool): u32;
fn pcEncodeDigit(d: u32): u8;
fn pcDecodeDigit(c: u8): u32;
fn pcThreshold(k: u32, bias: u32): u32;
fn pcDecodeUtf8(s: &[]u8, cps: []u32): i64;
fn pcEncodeUtf8(cps: &[]u32, out: []u8): i64;
fn encodeLabel(utf8_label: string, out: []u8): i64;
fn decodeLabel(ascii_label: string, out_utf8: []u8): i64;
fn pcWriteSlice(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn toAsciiDomain(utf8_domain: string, out: []u8): i64;
fn toUnicodeDomain(ascii_domain: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { isAsciiOnly } from "punycode";

pub fn main() {
    let result = isAsciiOnly("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"punycode"`; install via `cap add punycode`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
