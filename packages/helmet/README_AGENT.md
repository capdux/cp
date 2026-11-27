# helmet — AI Agent Quick Ref

Zero-heap HTTP security response headers policy generator (CSP, HSTS, COOP, CORP, X-Frame-Options, Referrer-Policy) with CRLF-injection defense in pure CP (clean-room MIT counterpart to npm helmet v8). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HelmetOptions,
    hlmWriteByte,
    hlmWriteStr,
    hlmWriteU64,
    hlmWriteHeaderLine,
    hlmAppendCspDirective,
    isSafeHeaderValue,
    formatHstsHeader,
} from "helmet";
```

## Core Types & Constants

```cp
class HelmetOptions { csp_enabled: bool, csp_value: string, hsts_enabled: bool, hsts_max_age: u64, /* +10 fields */ } // methods: defaultStrict(...) +1
```

## API Signatures

```cp
fn hlmWriteByte(out: []u8, pos: &mut usize, b: u8);
fn hlmWriteStr(out: []u8, pos: &mut usize, s: string);
fn hlmWriteU64(out: []u8, pos: &mut usize, val: u64);
fn hlmWriteHeaderLine(out: []u8, pos: &mut usize, name: string, val: string);
fn hlmAppendCspDirective(out: []u8, pos: &mut usize, has_any: &mut bool, dir_name: string, val: string);
fn isSafeHeaderValue(val: string): bool;
fn formatHstsHeader(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize;
fn formatCspHeader(default_src: string, script_src: string, style_src: string, img_src: string, connect_src: string, out: []u8): usize;
fn formatHelmetHeaders(opts: HelmetOptions, out: []u8): usize;
```

## Minimal Usage

```cp
import { isSafeHeaderValue } from "helmet";

pub fn main() {
    let result = isSafeHeaderValue("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"helmet"`; install via `cap add helmet`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
