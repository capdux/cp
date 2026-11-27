[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# soundex-code

Zero-heap American Soundex phonetic algorithm encoder, variable-length/padded encoder, phonetic equality matcher, and 0–4 similarity scorer in pure CP.

- **Package**: `soundex-code` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port compatible with both `npm/soundex-code` (`MIT`, `words/soundex-code`) and `crates.io/rphonetic` (`MIT`).

## Installation & Import

```bash
cap add soundex-code
```

```typescript
import {
    DEFAULT_SOUNDEX_LEN,
    soundexCharCode, soundex, soundexCode, soundexPadded,
    soundsLike, soundsLikeWithLength, soundexDifference,
} from "soundex-code";
```

## Core API

| Symbol | Signature / Value | Description |
| --- | --- | --- |
| `DEFAULT_SOUNDEX_LEN` | `4` (`usize`) | Standard American Soundex code length. |
| `soundexCharCode` | `(ch: u8): u8` | Maps an ASCII letter to its Soundex digit (`'1'`–`'6'`), or `0` for vowels/H/W/non-alpha. |
| `soundex` | `(word: string, out: []u8): usize` | Computes standard 4-character padded American Soundex code into `out` (returns `4`, or `0` if no ASCII letters / `out` too small). |
| `soundexCode` | `(word: string, max_length: usize, out: []u8): usize` | Computes Soundex code up to `max_length` (clamped to `min(max_length, out.len)`), padded with `'0'` up to `min(4, max_length)`. |
| `soundexPadded` | `(word: string, length: usize, out: []u8): usize` | Computes Soundex code padded with trailing `'0'` to exact `length`. |
| `soundsLike` | `(a: string, b: string): bool` | Returns `true` if `a` and `b` share the same 4-character Soundex code. |
| `soundsLikeWithLength` | `(a: string, b: string, max_length: usize): bool` | Returns `true` if `a` and `b` share the same Soundex code of length `max_length`. |
| `soundexDifference` | `(a: string, b: string): u8` | Returns SQL-style phonetic similarity score `0..=4` (matching positions in the 4-char Soundex code). |

## Examples

### 1. Standard 4-Char Soundex Encoding

```typescript
import { soundex } from "soundex-code";

fn main() {
    let out: [4]u8 = undefined;
    let n1 = soundex("Robert", out[0..]);    // "R163" (n1 == 4)
    let n2 = soundex("Rupert", out[0..]);    // "R163" (n2 == 4)
    let n3 = soundex("Ashcraft", out[0..]);  // "A261" (H/W rule applied)
}
```

### 2. Phonetic Matching & Similarity Scoring

```typescript
import { soundsLike, soundexDifference } from "soundex-code";

fn main() {
    let same = soundsLike("Robert", "Rupert");       // true
    let score = soundexDifference("Robert", "Rubin"); // 2
}
```

### 3. Variable-Length & Custom-Padded Codes

```typescript
import { soundexCode, soundexPadded, soundsLikeWithLength } from "soundex-code";

fn main() {
    let buf: [8]u8 = undefined;
    let n1 = soundexCode("Washington", 6, buf[0..]);   // "W25235"
    let n2 = soundexPadded("Lee", 6, buf[0..]);        // "L00000"
    let ok = soundsLikeWithLength("Washington", "Wasington", 6);
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All encoders write directly into caller-provided `out: []u8` stack buffers (`[4]u8` for standard Soundex).
- **Exact NARA H/W Collapsing Rule**: Handles adjacent same-digit consonants separated by `H` or `W` identically to American Soundex (e.g. `Ashcraft -> A261`, `Pfister -> P236`).
- **Dual Ecosystem Parity**: Combines `npm/soundex-code` variable-length `soundexCode` with `rphonetic`/SQL `soundexDifference` scoring.
