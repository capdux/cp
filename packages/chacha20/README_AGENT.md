# chacha20@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8439 ChaCha20 stream cipher (Rust chacha20 crate parity) for CP | License: MIT
> Import: `import { ... } from "chacha20";` | Full types: `src/index.d.cp`

## Functions
- `export fn cc20Add32(a: u32, b: u32): u32;`
- `export fn cc20Rotl16(x: u32): u32;`
- `export fn cc20Rotl12(x: u32): u32;`
- `export fn cc20Rotl8(x: u32): u32;`
- `export fn cc20Rotl7(x: u32): u32;`
- `export fn cc20ReadLe32(b: []u8, off: usize): u32;`
- `export fn cc20WriteLe32(out: []u8, off: usize, v: u32);`
- `export fn cc20QuarterRound(st: []u32, ai: usize, bi: usize, ci: usize, di: usize);`
- `export fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool;`
