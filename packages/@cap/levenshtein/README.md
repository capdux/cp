# @cap/levenshtein

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastest-levenshtein` ([https://github.com/ka-weihe/fastest-levenshtein](https://github.com/ka-weihe/fastest-levenshtein)) / `crates.io:levenshtein` ([https://github.com/wooorm/levenshtein-rs](https://github.com/wooorm/levenshtein-rs))（原库名 `fastest-levenshtein` → CP 包名 `@cap/levenshtein`）

Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup in pure CP.

## Features

- `distance(a, b)`: single rolling-row zero-heap Levenshtein with common prefix/suffix stripping (up to `MAX_LEN = 256`)
- `damerauDistance(a, b)`: Optimal String Alignment Damerau-Levenshtein supporting adjacent transpositions (`"ca"` vs `"ac"` = 1)
- `similarity(a, b)`: normalized `0.0..1.0` similarity score
- `withinDistance(a, b, max_dist)`: fast length-difference pruning
- `closestIndex(query, candidates, max_dist)`: CLI typo suggestion / nearest-neighbor finder

## Usage

```cp
import { distance, damerauDistance, closestIndex } from "@cap/levenshtein";

const d = distance("kitten", "sitting");      // 3
const t = damerauDistance("teh", "the");      // 1
```
