# damerau-levenshtein (AI Agent Quick Ref)

- **Import**: `import { damerauLevenshtein, damerauLevenshteinSteps, DamLevResult } from "damerau-levenshtein";`
- **Source**: `npm:damerau-levenshtein@1.0.8` (MIT, zero-heap 3-row rolling DP, `undefined` stack rows).

## Core Signatures

```cp
export class DamLevResult {
    steps: usize,
    relative: f64,   // steps / max(a.len, b.len)
    similarity: f64, // 1.0 - relative
}

export fn damerauLevenshteinSteps(a: string, b: string): usize;
export fn damerauLevenshtein(a: string, b: string): DamLevResult;
```

## Return Values Reference

| Case | `steps` | `relative` | `similarity` |
| :--- | :---: | :---: | :---: |
| `("", "")` | `0` | `0.0` | `1.0` |
| `("abc", "abc")` | `0` | `0.0` | `1.0` |
| `("ca", "ac")` (transposition) | `1` | `0.5` | `0.5` |
| `("kitten", "sitting")` | `3` | `~0.4286` | `~0.5714` |

## Minimal Pattern

```cp
let steps = damerauLevenshteinSteps("teh", "the"); // 1
let res = damerauLevenshtein("commit", "comimt");
if (res.steps <= 2 && res.similarity >= 0.75) { /* fuzzy match */ }
```

## Pitfalls

1. **127-byte prefix cap**: Inputs longer than 127 bytes are clamped to the first 127 bytes (`m = 127`, `n = 127`) to fit the fixed `[128]usize` rolling stack rows.
2. **Byte-level comparison**: Operates on UTF-8 bytes, matching ASCII/identifier fuzzy matching; multi-byte UTF-8 codepoints count per byte.
3. **Empty strings**: `damerauLevenshtein("", "")` safely returns `{ steps: 0, relative: 0.0, similarity: 1.0 }` without division by zero.
4. **OSA semantics**: Implements Optimal String Alignment (no substring may be edited more than once), matching npm `damerau-levenshtein`.
5. **Prefer `damerauLevenshteinSteps`**: Avoids `f64` division when only integer edit distance is needed.
