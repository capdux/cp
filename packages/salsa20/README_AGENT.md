# salsa20 — AI Agent Quick Ref

Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce) verified against official Bernstein/eSTREAM vectors (`v0.10.2`, `MIT`)

## Import

```cp
import {
    s20Rotl,
    s20LoadLe32,
    s20StoreLe32,
    salsa20Block,
    salsa20Apply,
} from "salsa20";
```

## API Signatures

```cp
fn s20Rotl(v: u32, shift: u5): u32;
fn s20LoadLe32(src: &[]u8, off: usize): u32;
fn s20StoreLe32(v: u32, dst: []u8, off: usize);
fn salsa20Block(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8);
fn salsa20Apply(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { s20Rotl } from "salsa20";

pub fn main() {
    let result = s20Rotl(0, /* shift: u5 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"salsa20"`; install via `cap add salsa20`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add salsa20
cpc check src/main.cp
```
