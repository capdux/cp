# adler2 — AI Agent Quick Reference

- **Package**: `adler2` (`v1.0.0`) | **Source**: crates.io `adler2` | **Import**: `import { ... } from "adler2";`
- **Role**: Zero-heap 16-lane SIMD RFC 1950 Adler-32 checksum with streaming, O(1) rolling window, and O(1) combine.

## Core Exports (`src/index.d.cp`)

```cp
class Adler32 {
    s1: u32,
    s2: u32,
    fn init(): Adler32;
    fn fromValue(checksum: u32): Adler32;
    fn update(_: auto, data: &[]u8);
    fn updateByte(_: auto, b: u8);
    fn roll(_: auto, window_len: usize, out_byte: u8, in_byte: u8);
    fn finalize(): u32;
    fn reset(_: auto);
}

fn adler32(data: &[]u8): u32;
fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;
```

## Minimal Usage

```cp
import { Adler32, adler32, combineAdler32 } from "adler2";

const data: []const u8 = "Wikipedia";
const s = adler32(&data); // 0x11E60398

let h = Adler32.init();
h.update(&"abcd");
h.roll(4, 'a' as u8, 'e' as u8); // sliding window -> "bcde"
const rolled = h.finalize();

const merged = combineAdler32(adler32(&"foo"), adler32(&"bar"), 3);
```

## Pitfalls & Constraints

1. **Empty input**: `adler32(&"")` returns `1` (`0x00000001`), NOT `0` (per RFC 1950).
2. **`roll` window length**: `window_len` MUST match the exact byte length currently hashed in `Adler32` (and `out_byte` must be the oldest byte leaving the window).
3. **`combineAdler32` length**: Pass `len2` as `u64` — it is the byte length of the **second** buffer (`adler2`), not the first.
4. **Input type**: Takes `&[]u8` (byte slice). For `string`-based CRC-32/CRC-32C/Adler-32, see `simd-adler32`.
