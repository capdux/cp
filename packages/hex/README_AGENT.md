# hex — AI Agent Quick Ref

Zero-allocation hexadecimal encoder/decoder, fixed-width integer hex formatter, and canonical HexDump utility for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    appendByte,
    encodeHex,
    encodeHexUpper,
    isValidHex,
    decodeHex,
    decodeHexPrefixed,
    encodeU32Hex,
    encodeU64Hex,
} from "hex";
```

## API Signatures

```cp
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn encodeHex(input: string, out: []u8): string;
fn encodeHexUpper(input: string, out: []u8): string;
fn isValidHex(s: string): bool;
fn decodeHex(hex_str: string, out: []u8): string?;
fn decodeHexPrefixed(hex_str: string, out: []u8): string?;
fn encodeU32Hex(v: u32, out: []u8): string;
fn encodeU64Hex(v: u64, out: []u8): string;
fn hexDump(data: string, out: []u8): string;
```

## Minimal Usage

```cp
import { encodeHex } from "hex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeHex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hex"`; install via `cap add hex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
