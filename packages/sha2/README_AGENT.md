# sha2 — AI Agent Quick Reference

- **Package**: `sha2` (`v1.0.0`) | **Source**: crates.io `sha2` | **Import**: `import { ... } from "sha2";`
- **Role**: Zero-heap FIPS 180-4 SHA-256 & SHA-224 streaming hashers with lowercase hex output.

## Core Exports (`src/index.d.cp`)

```cp
class Sha256 {
    st: Sha256State,
    fn new(): Sha256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out32: []u8);
    fn finalizeHex(_: auto, out64: []u8): usize;
    fn reset(_: auto);
}

class Sha224 {
    st: Sha256State,
    fn new(): Sha224;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out28: []u8);
    fn finalizeHex(_: auto, out56: []u8): usize;
}

fn sha256Hex(s: string, out64: []u8): usize;
fn sha224Hex(s: string, out56: []u8): usize;
```

## Minimal Usage

```cp
import { Sha256, Sha224, sha256Hex, sha224Hex } from "sha2";

let hex256: [64]u8 = undefined;
const n1 = sha256Hex("abc", hex256[0..64]); // 64

let h = Sha256.new();
h.updateStr("hello ");
h.updateStr("world");
let dig: [32]u8 = undefined;
h.finalize(dig[0..32]);
h.reset(); // safe to reuse after reset()
```

## Pitfalls & Constraints

1. **Buffer sizes**: `Sha256` needs `out32.len >= 32` / `out64.len >= 64`; `Sha224` needs `out28.len >= 28` / `out56.len >= 56`. `finalizeHex` / `sha*Hex` return `0` if output buffer is too small.
2. **Post-finalize state**: `finalize` / `finalizeHex` mutate internal padding state. Call `h.reset()` on `Sha256` (or `Sha224.new()` for `Sha224`, which has no `reset()` method) before hashing another message.
3. **Scope**: Implements SHA-256 and SHA-224 (not SHA-384/SHA-512). For keyed MAC, use `hmac`; for key derivation, use `hkdf`.
