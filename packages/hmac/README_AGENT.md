# hmac — AI Agent Quick Reference

- **Package**: `hmac` (`v1.0.0`) | **Source**: crates.io `hmac` | **Import**: `import { ... } from "hmac";`
- **Role**: Zero-heap RFC 2104 streaming HMAC-SHA256 with constant-time `verifySlice` and hex output.

## Core Exports (`src/index.d.cp`)

```cp
class HmacSha256 {
    inner: Sha256State,
    o_key_pad: [64]u8,
    fn newFromSlice(key: []u8): HmacSha256;
    fn newFromKey(key: string): HmacSha256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, msg: string);
    fn finalize(_: auto, out32: []u8);
    fn verifySlice(_: auto, expected: []u8): bool;
}

fn encodeHex32(digest: []u8, out64: []u8): usize;
fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;
```

## Minimal Usage

```cp
import { HmacSha256, hmacSha256Hex } from "hmac";

let hex: [64]u8 = undefined;
const n = hmacSha256Hex("secret", "hello", hex[0..64]); // n == 64

let mac = HmacSha256.newFromKey("secret");
mac.updateStr("hello");
let tag: [32]u8 = undefined;
mac.finalize(tag[0..32]);

let v = HmacSha256.newFromKey("secret");
v.updateStr("hello");
const ok = v.verifySlice(tag[0..32]); // constant-time ctEqual
```

## Pitfalls & Constraints

1. **Key length cap in `newFromKey`**: `newFromKey(string)` copies at most `256` bytes into a stack buffer. For keys `> 256` bytes or raw binary keys, ALWAYS use `HmacSha256.newFromSlice(key: []u8)`.
2. **Single-use `finalize` / `verifySlice`**: Both consume/finalize the internal `Sha256State`; do NOT call `update` or `finalize` again on the same instance.
3. **Output buffer sizes**: `finalize` requires `out32.len >= 32`; `encodeHex32` and `hmacSha256Hex` require `out64.len >= 64` (return `0` if smaller).
4. **Always use `verifySlice`**: Never compare MAC tags with `==` on untrusted inputs; `verifySlice` uses constant-time `ctEqual`.
