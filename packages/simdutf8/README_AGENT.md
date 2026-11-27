# simdutf8 — AI Agent Quick Reference

- **Package**: `simdutf8` (`v1.0.0`) | **Source**: crates.io `simdutf8` | **Import**: `import { ... } from "simdutf8";`
- **Role**: Zero-heap strict RFC 3629 UTF-8 validator with 16-lane SIMD (`@Vector(16, u8)`) ASCII fast-path.

## Core Exports (`src/index.d.cp`)

```cp
class Utf8Error {
    valid_up_to: usize,
    has_error_len: bool,
    error_len: usize,
}

class Utf8CompatResult {
    valid: bool,
    valid_up_to: usize,
    has_error_len: bool,
    error_len: usize,
}

fn isAscii(buf: []u8): bool;
fn validateUtf8Compat(buf: []u8): Utf8CompatResult;
fn basicIsValidUtf8(buf: []u8): bool;
fn countCodepoints(buf: []u8): usize?;
```

## Minimal Usage

```cp
import { isAscii, basicIsValidUtf8, validateUtf8Compat, countCodepoints } from "simdutf8";

let buf: [6]u8 = [0xE4, 0xBD, 0xA0, 0xE5, 0xA5, 0xBD]; // "你好"
const ascii = isAscii(buf[0..6]);            // false
const ok = basicIsValidUtf8(buf[0..6]);      // true
const res = validateUtf8Compat(buf[0..6]);   // res.valid == true, res.valid_up_to == 6
const chars = countCodepoints(buf[0..6]);    // 2 (returns null on invalid UTF-8)
```

## Pitfalls & Constraints

1. **Streaming truncation vs hard error**: When `res.valid == false`, check `res.has_error_len`:
   - `false` → unexpected EOF (truncated multi-byte sequence at `buf[res.valid_up_to..]`; carry over to next stream chunk).
   - `true` → invalid byte sequence of length `res.error_len` (1..3) starting at `res.valid_up_to`.
2. **Strict RFC 3629**: Rejects overlong sequences (`C0..C1`, `E0 80..9F`, `F0 80..8F`), UTF-16 surrogates (`U+D800..U+DFFF`), and `> U+10FFFF`.
3. **Slice type**: Functions accept `[]u8` (mutable/sliced byte buffer); for valid UTF-8 codepoint counting without re-validating, consider `bytecount.numChars`.
