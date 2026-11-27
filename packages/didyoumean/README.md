# @cap/didyoumean

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:didyoumean` ([https://github.com/dcporter/didyoumean.js](https://github.com/dcporter/didyoumean.js))

Zero-allocation spell-suggestion & edit-distance engine for CP — clean-room MIT parity with npm `didyoumean` / `fastest-levenshtein` and Rust `strsim`.

## Install

```bash
cap add @cap/didyoumean
```

## Features

- **Levenshtein & Damerau-Levenshtein**: exact edit distance with adjacent transposition support (`"teh"` → `"the"` = 1) using O(N) stack rows and zero heap allocations.
- **CLI & API Command Suggestion**: `didYouMean(input, candidates)` & `didYouMeanWith(input, candidates, max_dist)` return the closest match with distance and index.
- **Normalized Similarity**: `similarity(a, b)` returns `0.0 .. 1.0`.

## Usage

```cp
import { didYouMean, damerauLevenshtein } from "@cap/didyoumean";

const cmds: [4]string = ["deploy", "publish", "install", "remove"];
const hit = didYouMean("depoly", cmds[0..4]); // "deploy"
```
