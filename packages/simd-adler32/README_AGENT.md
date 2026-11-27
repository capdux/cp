# simd-adler32 — AI Agent Quick Reference

- **Package**: `simd-adler32` (`v1.0.0`) | **Source**: crates.io `simd-adler32` / `crc-catalog` | **Import**: `import { ... } from "simd-adler32";`
- **Role**: Zero-heap 16-lane SIMD Adler-32 + IEEE CRC-32 + Castagnoli CRC-32C + O(log N) GF(2) CRC-32 combine.

## Core Exports (`src/index.d.cp`)

```cp
class Crc32Hasher {
    state: u32,
    table: [256]u32,
    fn init(): Crc32Hasher;
    fn update(_: auto, data: string);
    fn finalize(): u32;
    fn reset(_: auto);
}

class Crc32cHasher {
    state: u32,
    table: [256]u32,
    fn init(): Crc32cHasher;
    fn update(_: auto, data: string);
    fn finalize(): u32;
    fn reset(_: auto);
}

fn crc32Ieee(data: string): u32;
fn crc32c(data: string): u32;
fn adler32(data: string): u32;
fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize): u32;
```

## Minimal Usage

```cp
import { adler32, crc32Ieee, crc32c, Crc32Hasher, combineCrc32Ieee } from "simd-adler32";

const a = adler32("Wikipedia");      // 0x11E60398 (16-lane SIMD)
const c = crc32Ieee("123456789");    // 0xCBF43926
const cc = crc32c("123456789");      // 0xE3069283
const m = combineCrc32Ieee(crc32Ieee("1234"), crc32Ieee("56789"), 5); // 0xCBF43926
```

## Pitfalls & Constraints

1. **String input**: All functions take `string` (not `&[]u8`). For `&[]u8` Adler-32 with rolling window & streaming `Adler32` class, use `adler2`.
2. **Reuse hashers**: `Crc32Hasher.init()` builds a 256-entry `[256]u32` LUT on the stack; call `h.reset()` to reuse it across messages instead of re-initializing in a hot loop.
3. **`combineCrc32Ieee` scope**: Works for **IEEE CRC-32** only (not CRC-32C); `len2` is the byte length of the second chunk.
4. **Known check vectors**: `"123456789"` -> IEEE `0xCBF43926`, Castagnoli `0xE3069283`; `"Wikipedia"` -> Adler-32 `0x11E60398`.
