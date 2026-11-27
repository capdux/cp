# @cap/strsim

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strsim` ([https://github.com/rapidfuzz/strsim-rs](https://github.com/rapidfuzz/strsim-rs))

Clean-room MIT, zero-heap string similarity & edit-distance library for CP — modeled on Rust's `strsim` crate (850M+ downloads).

## API
- `hamming(a, b): usize?`
- `levenshtein(a, b): usize`
- `osaDistance(a, b): usize` (restricted Damerau-Levenshtein with adjacent transposition)
- `normalizedLevenshteinBps(a, b): u32` (`0..10000` basis points)
- `jaroBps(a, b): u32` & `jaroWinklerBps(a, b): u32` (`0..10000`)
- `sorensenDiceBps(a, b): u32` (`0..10000`)
