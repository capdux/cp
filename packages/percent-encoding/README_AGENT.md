# percent-encoding@1.0.0 — AI Agent Quick Ref

- **Source**: `crates.io:percent-encoding` (`servo/rust-url`, MIT) | **Deps**: none
- **Import**: `import { AsciiSet, controlsSet, fragmentSet, querySet, pathSet, userinfoSet, componentSet, nonAlphanumericSet, percentEncodeLen, percentEncode, percentDecode, percentDecodeStrict, percentDecodePlus } from "percent-encoding";`
- **Full Types**: `packages/percent-encoding/src/index.d.cp`

## Core Types & Functions
```cp
class AsciiSet {
    bits: [4]u32,
    fn empty(): AsciiSet
    fn add(_: auto, byte: u8)
    fn remove(_: auto, byte: u8)
    fn unionWith(other: AsciiSet): AsciiSet
    fn shouldPercentEncode(byte: u8): bool
}

// Predefined WHATWG sets (comptime-built):
fn controlsSet(): AsciiSet
fn fragmentSet(): AsciiSet
fn querySet(): AsciiSet
fn pathSet(): AsciiSet
fn userinfoSet(): AsciiSet
fn componentSet(): AsciiSet
fn nonAlphanumericSet(): AsciiSet

// Encode / Decode:
fn percentEncodeLen(input: &[]u8, set: AsciiSet): usize
fn percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize
fn percentDecode(input: string, out: []u8): usize
fn percentDecodeStrict(input: string, out: []u8): i64
fn percentDecodePlus(input: string, out: []u8): usize
```

## Minimal Pattern
```cp
import { componentSet, percentEncode, percentDecodeStrict } from "percent-encoding";

let enc: [128]u8 = undefined;
const n = percentEncode("a b/c?", componentSet(), enc[0..]); // "a%20b%2Fc%3F"
let dec: [128]u8 = undefined;
const m = percentDecodeStrict(enc[0..n], dec[0..]); // m >= 0 on success, -1 on error
```

## Pitfalls & Notes
1. **Non-ASCII Bytes (`>= 0x80`)**: Always percent-encoded by `shouldPercentEncode` regardless of `AsciiSet` bits (per WHATWG spec).
2. **Strict vs Lenient Decode**: `percentDecode` / `percentDecodePlus` copy invalid `%XX` literally; use `percentDecodeStrict` (`i64`, `-1` on error) when validating untrusted URLs.
3. **Buffer Sizing**: Encoded output can be up to `3 * input.len`; call `percentEncodeLen(input, set)` first if exact sizing is needed.
4. **Form Data**: Use `percentDecodePlus` for `application/x-www-form-urlencoded` where `+` represents space (`0x20`).
5. **Internal Helpers**: `peBitMask`, `peHexUpper`, `peHexVal` are not re-exported by `export { ... }` — do not import them.
