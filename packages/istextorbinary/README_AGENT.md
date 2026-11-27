# istextorbinary — AI Agent Quick Ref

Zero-allocation WHATWG MIME Sniffing Standard binary/text detector and HTML/XML/JSON/SVG content sniffer for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    msToLower,
    msIsSpace,
    msIsTagTerminator,
    msStartsWithIgnoreCase,
    msMatchTag,
    isBinary,
    isText,
    sniffContentType,
} from "istextorbinary";
```

## API Signatures

```cp
fn msToLower(c: u8): u8;
fn msIsSpace(c: u8): bool;
fn msIsTagTerminator(c: u8): bool;
fn msStartsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
fn msMatchTag(s: &[]u8, tagPrefix: &[]u8): bool;
fn isBinary(data: &[]u8): bool;
fn isText(data: &[]u8): bool;
fn sniffContentType(data: &[]u8): u8;
fn contentTypeToMime(ct: u8): string;
fn sniffMime(data: &[]u8): string;
```

## Minimal Usage

```cp
import { isBinary } from "istextorbinary";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBinary(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"istextorbinary"`; install via `cap add istextorbinary`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
