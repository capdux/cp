# base64ct — AI Agent Quick Reference

- **Package**: `base64ct` (`v1.0.0`) | **Source**: crates.io `base64ct` | **Import**: `import { ... } from "base64ct";`
- **Role**: Zero-heap constant-time, table-free, branchless Base64 encoder & strict RFC 4648 canonical decoder.

## Core Exports (`src/index.d.cp`)

```cp
class DecodeResult {
    len: usize,
    ok: bool,
}

fn encodedLen(n: usize, padded: bool): usize;
fn encodeStd(src: []u8, out: []u8): usize;
fn encodeStdUnpadded(src: []u8, out: []u8): usize;
fn encodeUrl(src: []u8, out: []u8): usize;
fn encodeUrlUnpadded(src: []u8, out: []u8): usize;
fn decodeStd(src: string, out: []u8): DecodeResult;
fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult;
```

## Minimal Usage

```cp
import { encodedLen, encodeStd, encodeUrlUnpadded, decodeStd, decodeUrlUnpadded } from "base64ct";

let raw: [5]u8 = ['h', 'e', 'l', 'l', 'o'];
let enc: [16]u8 = undefined;
const n = encodeStd(raw[0..5], enc[0..16]);      // n == 8 ("aGVsbG8=")

let dec: [16]u8 = undefined;
const r = decodeStd("aGVsbG8=", dec[0..16]);     // r.ok == true, r.len == 5
const u = decodeUrlUnpadded("aGVsbG8", dec[0..16]); // r.ok == true, r.len == 5
```

## Pitfalls & Constraints

1. **Buffer sizing**: Always check `out.len >= encodedLen(src.len, padded)` before encoding; `encode*` returns `0` if `out` is too small (or `src.len == 0`).
2. **Strict canonical decoding**: Non-zero unused trailing bits, embedded whitespace, or missing `=` padding in `decodeStd` fail with `{ len: 0, ok: false }` AND zero out `out`.
3. **Decode input type**: `decodeStd` and `decodeUrlUnpadded` accept `src: string` and write decoded bytes into `out: []u8`.
4. **Supported decoders**: `decodeStd` (padded standard) and `decodeUrlUnpadded` (unpadded URL-safe, e.g. JWT).
5. **Security**: Zero lookup tables and zero secret-dependent branches; safe for encoding/decoding cryptographic secrets.
