# bitstream-io — AI Agent Quick Ref

Zero-allocation MSB-first & LSB-first BitReader and BitWriter supporting 0..64-bit reads/writes, peek, byte alignment, and H.264/HEVC Exp-Golomb coding (Rust bitstream-io parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { BitReadResult, BitReader, BitWriter } from "bitstream-io";
```

## Core Types & Constants

```cp
class BitReadResult { value: u64, ok: bool }
class BitReader { buf: &[]u8, bit_pos: usize, msb_first: bool } // methods: initMsb(...) +10
class BitWriter { buf: []u8, bit_pos: usize, msb_first: bool } // methods: initMsb(...) +7
```

## Minimal Usage

```cp
import { BitReadResult } from "bitstream-io";

pub fn main() {
    // Refer to src/index.d.cp for BitReadResult fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bitstream-io"`; install via `cap add bitstream-io`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add bitstream-io
cpc check src/main.cp
```
