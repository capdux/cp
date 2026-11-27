# `icu_normalizer` — AI Agent Quick Ref

Zero-allocation UAX #15 Unicode NFC / NFD / NFKC / NFKD normalizer, CCC ordering & Hangul codec (ICU4X `icu_normalizer` parity).

## Import

```cp
import {
    HANGUL_SBASE, HANGUL_LBASE, HANGUL_VBASE, HANGUL_TBASE,
    HANGUL_LCOUNT, HANGUL_VCOUNT, HANGUL_TCOUNT, HANGUL_NCOUNT, HANGUL_SCOUNT,
    canonicalCombiningClass, hangulDecompose, hangulCompose,
    latinCanonicalDecompose, latinCanonicalCompose, compatibilityDecompose,
    canonicalOrder,
    normalizeNfd, normalizeNfc, normalizeNfkd, normalizeNfkc,
    isNfc, isNfd, eqNfc,
} from "icu_normalizer";
```

## Core API Signatures

```cp
// High-level UTF-8 APIs (zero-heap, write into caller `out: []u8`)
fn normalizeNfc(input: string, out: []u8): usize;
fn normalizeNfd(input: string, out: []u8): usize;
fn normalizeNfkc(input: string, out: []u8): usize;
fn normalizeNfkd(input: string, out: []u8): usize;
fn isNfc(input: string): bool;
fn isNfd(input: string): bool;
fn eqNfc(a: string, b: string): bool;

// Low-level codepoint primitives
fn canonicalCombiningClass(cp: u32): u8;
fn canonicalOrder(cps: []u32);                           // in-place mutable slice
fn hangulDecompose(cp: u32, out: []u32): usize;          // writes 2 (LV) or 3 (LVT) jamo
fn hangulCompose(l: u32, v: u32, t: u32): u32;           // t=0 for LV; returns 0 if invalid
fn latinCanonicalDecompose(cp: u32, out: []u32): usize;
fn latinCanonicalCompose(base: u32, comb: u32): u32;     // 0 if no composition
fn compatibilityDecompose(cp: u32, out: []u32): usize;
```

## Minimal Example

```cp
import { normalizeNfc, eqNfc } from "icu_normalizer";

const a = "caf\u{00E9}";
const b = "cafe\u{0301}";
if (eqNfc(a, b)) {
    let out: [64]u8 = undefined;
    const n = normalizeNfc(b, out[0..64]); // out[0..n] == "caf\u{00E9}"
}
```

## Gotchas & Borrow Rules

- All `normalize*` functions write into a caller-owned mutable `[]u8` and return bytes written; size `out` generously (e.g., `3 * input.len` for NFD/NFKD expansion).
- `isNfc`, `isNfd`, and `eqNfc` use an internal 1024-byte stack buffer for non-ASCII inputs; for inputs > 1024 UTF-8 bytes, call `normalizeNfc` / `normalizeNfd` with a larger caller buffer instead.
- `canonicalOrder(cps: []u32)` sorts the codepoint slice in-place; pass a mutable `let` array slice.
- `hangulCompose` takes `t = 0` when composing a 2-jamo `LV` syllable.
