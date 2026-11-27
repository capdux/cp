# `@cap/icu-normalizer`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:icu_normalizer` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))（原库名 `icu_normalizer` → CP 包名 `@cap/icu-normalizer`）

Zero-allocation UAX #15 Unicode NFC / NFD / NFKC / NFKD text normalizer in pure CP (ICU4X `icu_normalizer` parity).

## API

- `canonicalCombiningClass(cp: u32): u8` — Unicode Canonical Combining Class (ASCII + Combining Diacritical Marks `U+0300..U+036F`)
- `hangulDecompose(cp: u32, out: []u32): usize` / `hangulCompose(l: u32, v: u32, t: u32): u32` — Complete Unicode Standard §3.12 algorithmic (de)composition for all 11,172 Hangul syllables
- `latinCanonicalDecompose(cp: u32, out: []u32): usize` / `latinCanonicalCompose(base: u32, comb: u32): u32` — Canonical mappings for Latin-1 Supplement (`U+00C0..U+00FF`) and common Latin Extended-A
- `compatibilityDecompose(cp: u32, out: []u32): usize` — Fullwidth ASCII (`U+FF01..U+FF5E`), ideographic/NBSP spaces, superscripts (`¹`/`²`/`³`), and typographic ligatures (`ﬀ`/`ﬁ`/`ﬂ`/`ﬃ`/`ﬄ`)
- `canonicalOrder(cps: []u32)` — Stable UAX #15 Canonical Ordering Algorithm
- `normalizeNfd(input: string, out: []u8): usize` / `normalizeNfc(input: string, out: []u8): usize`
- `normalizeNfkd(input: string, out: []u8): usize` / `normalizeNfkc(input: string, out: []u8): usize`
- `isNfc(input: string): bool` / `isNfd(input: string): bool` / `eqNfc(a: string, b: string): bool`
