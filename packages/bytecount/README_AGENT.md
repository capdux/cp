# bytecount — AI Agent Quick Reference

- **Package**: `bytecount` (`v1.0.0`) | **Source**: crates.io `bytecount` | **Import**: `import { ... } from "bytecount";`
- **Role**: Zero-heap 16-lane SIMD (`@Vector(16, u8)`) byte frequency, UTF-8 char, line, and byte-range counters.

## Core Exports (`src/index.d.cp`)

```cp
fn count(haystack: &[]u8, needle: u8): usize;
fn countFast(haystack: &[]u8, needle: u8): usize;
fn numChars(utf8_bytes: &[]u8): usize;
fn countLines(buf: &[]u8): usize;
fn countAsciiWhitespace(buf: &[]u8): usize;
fn countInRange(buf: &[]u8, lo: u8, hi: u8): usize;
```

## Minimal Usage

```cp
import { count, countFast, numChars, countLines, countInRange, countAsciiWhitespace } from "bytecount";

const data: []const u8 = "a,b,c\n1,2,3\n";
const commas = countFast(&data, ',' as u8);          // 4
const lines  = countLines(&data);                    // 2
const digits = countInRange(&data, '0' as u8, '9' as u8); // 3
const chars  = numChars(&data);                      // 12
const ws     = countAsciiWhitespace(&data);          // 2
```

## Pitfalls & Constraints

1. **Argument order vs `memchr`**: `count(haystack, needle)` and `countFast(haystack, needle)` take `haystack` FIRST (opposite of `memchr.memchrCount(needle, haystack)`).
2. **`numChars` assumes valid UTF-8**: It counts non-continuation bytes `(b & 0xC0) != 0x80` without validating UTF-8. For untrusted input, validate with `simdutf8.countCodepoints` instead.
3. **`countLines` trailing newline**: Empty buffer returns `0`; `"a\nb\n"` returns `2`; `"a\nb"` also returns `2`.
4. **`countInRange` bounds**: Inclusive `[lo, hi]`; returns `0` cleanly if `lo > hi`.
5. **`countAsciiWhitespace` byte set**: Matches space (`0x20`), tab (`0x09`), LF (`0x0A`), CR (`0x0D`), and form-feed (`0x0C`) — does NOT include vertical tab (`0x0B`).
6. **SIMD engine & dependency**: `count` delegates to `memchr.countByte`; `countFast`, `numChars`, and `countInRange` run a 255-step `@Vector(16, u8)` lane accumulator (zero heap, 4080 bytes per horizontal sum).

## Source Provenance

- **Original crate**: `crates.io/crates/bytecount` (`https://github.com/llogiq/bytecount`)
- **Migration**: `bytecount::count(buf, b)` → `countFast(&buf, b)` | `bytecount::num_chars(buf)` → `numChars(&buf)`
