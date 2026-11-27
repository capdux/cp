# huffman-compress — AI Agent Quick Ref

Zero-allocation RFC 1951 Canonical Huffman prefix encoder & decoder with 256-symbol alphabet, 16-bit length limiting, and MSB-first bitstream framing (Rust huffman-compress parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { HuffmanCodec } from "huffman-compress";
```

## Core Types & Constants

```cp
class HuffmanCodec { code_len: [MAX_SYMBOLS]u8, codes: [MAX_SYMBOLS]u16, symbol_count: usize } // methods: empty(...) +6
```

## Minimal Usage

```cp
import { HuffmanCodec } from "huffman-compress";

pub fn main() {
    // Refer to src/index.d.cp for HuffmanCodec fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"huffman-compress"`; install via `cap add huffman-compress`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add huffman-compress
cpc check src/main.cp
```
