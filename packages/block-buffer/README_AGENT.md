# block-buffer — AI Agent Quick Ref

Zero-allocation fixed-size block streaming buffers (64/128-byte) and cryptographic padding primitives (Merkle-Damgard BE/LE, PKCS#7, ISO 7816, Zero) for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { BlockBuffer64, BlockBuffer128, bbWriteBe64, bbWriteLe64 } from "block-buffer";
```

## Core Types & Constants

```cp
class BlockBuffer64 { buf: [64]u8, pos: usize, total_bytes: u64 } // methods: new(...) +9
class BlockBuffer128 { buf: [128]u8, pos: usize, total_bytes: u64 } // methods: new(...) +9
```

## API Signatures

```cp
fn bbWriteBe64(out: []u8, offset: usize, val: u64);
fn bbWriteLe64(out: []u8, offset: usize, val: u64);
```

## Minimal Usage

```cp
import { bbWriteBe64 } from "block-buffer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    bbWriteBe64(out_buf[0..256], 0, 0);
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"block-buffer"`; install via `cap add block-buffer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
