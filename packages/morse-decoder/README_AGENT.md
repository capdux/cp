# morse-decoder — AI Agent Quick Ref

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder supporting A-Z, 0-9, punctuation, and '/' word separators. (`v1.0.0`, `MIT`)

## Import

```cp
import { encodeChar, decodeSymbol, encodeText, decodeText } from "morse-decoder";
```

## API Signatures

```cp
fn encodeChar(c: u8): string;
fn decodeSymbol(sym: string): u8;
fn encodeText(input: string, out: []u8): usize;
fn decodeText(morse: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { decodeSymbol } from "morse-decoder";

pub fn main() {
    let result = decodeSymbol("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"morse-decoder"`; install via `cap add morse-decoder`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add morse-decoder
cpc check src/main.cp
```
