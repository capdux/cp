# @cap/dice-coeff

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-similarity` ([https://github.com/aceakash/string-similarity](https://github.com/aceakash/string-similarity))（原库名 `string-similarity` → CP 包名 `@cap/dice-coeff`）

Zero-allocation Sørensen-Dice bigram string similarity coefficient and best-match finder for CP.

## Features

- **Zero heap allocation**: multiset bigram intersection using fixed stack buffers (`[256]u8`, `[256]bool`).
- **Standard & exact modes**: `diceCoefficient` (case-insensitive, ignores whitespace) and `diceCaseSensitive` (exact case & whitespace).
- **Candidate ranking**: `bestMatchIndex(target, candidates, min_rating)` selects the top fuzzy match above threshold.

## Usage

```cp
import { diceCoefficient, bestMatchIndex } from "@cap/dice-coeff";

const sim = diceCoefficient("healed", "sealed"); // 0.8
```
