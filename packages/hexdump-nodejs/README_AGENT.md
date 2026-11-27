# hexdump-nodejs — AI Agent Quick Ref

Zero-allocation canonical hexdump -C 16-byte-per-line binary debugger, spaced hex encoder, and first-diff locator in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    buildPrintableLut,
    formatLine,
    formatDump,
    encodeHexSpaced,
    firstDiffOffset,
} from "hexdump-nodejs";
```

## API Signatures

```cp
fn buildPrintableLut(): [256]u8;
fn formatLine(data: &[]u8, base_offset: usize, out: []u8): usize;
fn formatDump(data: &[]u8, out: []u8): usize;
fn encodeHexSpaced(data: &[]u8, uppercase: bool, out: []u8): usize;
fn firstDiffOffset(a: &[]u8, b: &[]u8): i64;
```

## Minimal Usage

```cp
import { formatLine } from "hexdump-nodejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatLine(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hexdump-nodejs"`; install via `cap add hexdump-nodejs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add hexdump-nodejs
cpc check src/main.cp
```
