[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# damerau-levenshtein

> **Source Attribution**: Clean-room MIT CP port of **npm** [`damerau-levenshtein`](https://github.com/tad-lispy/node-damerau-levenshtein) (`v1.0.8` / `MIT`), CP package `damerau-levenshtein@1.0.8`.

Zero-heap Damerau-Levenshtein (Optimal String Alignment) edit distance and similarity ratio in pure CP.

## Why CP `damerau-levenshtein`?

- **Zero Heap Allocation**: Uses 3 rolling stack rows (`row0`, `row1`, `row2` of `[128]usize`) instead of allocating an `O(m × n)` 2D matrix on the heap.
- **Zero-Overhead Stack Initialization**: All 3 rolling DP rows are initialized with `undefined` and strictly written before read, eliminating 3KB of redundant stack zeroing per call.
- **Typo-Aware Distance**: Counts insertion, deletion, substitution, and adjacent character transposition (`"ab"` ↔ `"ba"` = 1 step) — ideal for fuzzy CLI command suggestions, spellcheckers, and identifier matching.

## Zero-Cost Migration (JS/TS → CP)

Keep the exact same package name (`damerau-levenshtein`) — no renaming required.

**TypeScript (`package.json` / `index.ts`):**
```typescript
import levenshtein from "damerau-levenshtein";
const res = levenshtein("kitten", "sitting");
// { steps: 3, relative: 0.4285..., similarity: 0.5714... }
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    damerauLevenshtein, damerauLevenshteinSteps, DamLevResult
} from "damerau-levenshtein";
```

## Quick Start

### Compute Distance & Similarity Ratio

```typescript
import { damerauLevenshtein, damerauLevenshteinSteps } from "damerau-levenshtein";

export fn demoFuzzyMatch(): bool {
    // Fast steps-only path (no float math)
    let steps = damerauLevenshteinSteps("ca", "ac"); // 1 (adjacent transposition)

    // Full result with relative distance & similarity score
    let res = damerauLevenshtein("commit", "comimt");
    // res.steps == 1, res.relative == 1.0/6.0, res.similarity == 5.0/6.0
    return steps == 1 && res.similarity > 0.8;
}
```

## API Reference

- **`DamLevResult`**: `{ steps: usize, relative: f64, similarity: f64 }`
  - `steps`: Optimal String Alignment edit distance
  - `relative`: `steps / max(a.len, b.len)` (`0.0` when both strings are empty)
  - `similarity`: `1.0 - relative` (`1.0` for identical strings, `0.0` for completely disjoint strings)
- **`damerauLevenshteinSteps(a: string, b: string): usize`** — Computes edit distance steps (up to 127 bytes per string).
- **`damerauLevenshtein(a: string, b: string): DamLevResult`** — Computes `steps`, `relative`, and `similarity`.

## License

MIT
