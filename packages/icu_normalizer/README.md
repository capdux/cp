[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `icu_normalizer`

Zero-allocation UAX #15 Unicode NFC / NFD / NFKC / NFKD normalization, canonical combining class (CCC) ordering, Hangul algorithmic (de)composition, and Latin/compatibility mappings in pure CP — a clean-room MIT counterpart to ICU4X [`icu_normalizer`](https://github.com/unicode-org/icu4x) (v1.5 parity).

## Installation & Import

```bash
cap add icu_normalizer
```

```cp
import {
    HANGUL_SBASE, HANGUL_LBASE, HANGUL_VBASE, HANGUL_TBASE,
    HANGUL_LCOUNT, HANGUL_VCOUNT, HANGUL_TCOUNT, HANGUL_NCOUNT, HANGUL_SCOUNT,
    canonicalCombiningClass,
    hangulDecompose,
    hangulCompose,
    latinCanonicalDecompose,
    latinCanonicalCompose,
    compatibilityDecompose,
    canonicalOrder,
    normalizeNfd,
    normalizeNfc,
    normalizeNfkd,
    normalizeNfkc,
    isNfc,
    isNfd,
    eqNfc,
} from "icu_normalizer";
```

## Core API

### High-Level UTF-8 Normalization & Predicates

| Function | Signature | Description |
| :--- | :--- | :--- |
| `normalizeNfc` | `(input: string, out: []u8): usize` | Normalize UTF-8 string to Canonical Composition (NFC) into `out`; returns bytes written |
| `normalizeNfd` | `(input: string, out: []u8): usize` | Normalize UTF-8 string to Canonical Decomposition (NFD) into `out` |
| `normalizeNfkc` | `(input: string, out: []u8): usize` | Normalize UTF-8 string to Compatibility Composition (NFKC) into `out` |
| `normalizeNfkd` | `(input: string, out: []u8): usize` | Normalize UTF-8 string to Compatibility Decomposition (NFKD) into `out` |
| `isNfc` / `isNfd` | `(input: string): bool` | Fast-path ASCII (`O(n)`) + stack-buffered check whether `input` is already NFC / NFD |
| `eqNfc` | `(a: string, b: string): bool` | Test canonical equivalence under Unicode NFC without heap allocation |

### Low-Level UAX #15 Primitives & Hangul Constants

| Symbol | Signature / Values | Description |
| :--- | :--- | :--- |
| `HANGUL_*` | `SBASE=0xAC00`, `LBASE=0x1100`, `VBASE=0x1161`, `TBASE=0x11A7`, `LCOUNT=19`, `VCOUNT=21`, `TCOUNT=28`, `NCOUNT=588`, `SCOUNT=11172` | Standard Unicode Hangul syllable arithmetic constants |
| `canonicalCombiningClass` | `(cp: u32): u8` | Return Unicode Canonical Combining Class (`0..=240`) for codepoint `cp` |
| `canonicalOrder` | `(cps: []u32)` | In-place stable UAX #15 canonical reordering of combining mark runs |
| `hangulDecompose` / `hangulCompose` | `(cp: u32, out: []u32): usize` / `(l: u32, v: u32, t: u32): u32` | Algorithmic Hangul syllable decomposition (`LV` / `LVT`) and composition |
| `latinCanonicalDecompose` / `latinCanonicalCompose` | `(cp: u32, out: []u32): usize` / `(base: u32, comb: u32): u32` | Precomposed Latin canonical decomposition and composition pairs |
| `compatibilityDecompose` | `(cp: u32, out: []u32): usize` | Compatibility mappings (fullwidth ASCII, ligatures, superscripts/subscripts, fractions, etc.) |

## Examples

### 1. Comparing Canonically Equivalent Strings (`é` Precomposed vs `e` + Combining Acute)

```cp
import { print } from "cap:io";
import { normalizeNfc, normalizeNfd, eqNfc, isNfc } from "icu_normalizer";

fn main() {
    const composed = "caf\u{00E9}";       // "café" (U+00E9)
    const decomposed = "cafe\u{0301}";     // "cafe" + U+0301

    print("canonical eq:", eqNfc(composed, decomposed)); // true
    print("composed isNfc:", isNfc(composed));           // true
    print("decomposed isNfc:", isNfc(decomposed));       // false

    let buf: [128]u8 = undefined;
    const n = normalizeNfc(decomposed, buf[0..128]);
    print("normalized:", buf[0..n]);
}
```

### 2. NFKC Compatibility Folding & Hangul Algorithmic Decomposition

```cp
import { print } from "cap:io";
import { normalizeNfkc, hangulDecompose, hangulCompose } from "icu_normalizer";

fn main() {
    // Fullwidth "ＡＢＣ" -> ASCII "ABC"
    let out: [64]u8 = undefined;
    const n = normalizeNfkc("\u{FF21}\u{FF22}\u{FF23}", out[0..64]);
    print("NFKC:", out[0..n]); // "ABC"

    // Hangul syllable '한' (U+D55C) -> L, V, T jamo and back
    let jamo: [3]u32 = [0, 0, 0];
    const k = hangulDecompose(0xD55C, jamo[0..3]);
    if (k == 3) {
        const recomposed = hangulCompose(jamo[0], jamo[1], jamo[2]);
        print("recomposed:", recomposed == 0xD55C); // true
    }
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: All normalization pipelines operate on caller-supplied `[]u8` / `[]u32` buffers and fixed stack scratch space — no `String` or `Vec` allocations.
- **ASCII Fast Path**: `isNfc`, `isNfd`, and `eqNfc` short-circuit pure ASCII inputs via `cap:ascii.isAscii` and `cap:bin.bytesEq` in linear `O(n)` time with zero decoding overhead.
- **Compact Pure-CP Footprint**: Implements algorithmic Hangul (11,172 syllables with zero tables), CCC ordering, and core Latin/compatibility mappings without bundling multi-megabyte ICU data blobs.
