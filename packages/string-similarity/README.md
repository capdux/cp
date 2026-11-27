[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# string-similarity

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-similarity` ([https://github.com/aceakash/string-similarity](https://github.com/aceakash/string-similarity))（原库名 `string-similarity` → CP 包名 `string-similarity`）

Zero-allocation Sørensen-Dice bigram string similarity coefficient and best-match finder for CP.

## Features

- **Zero heap allocation**: multiset bigram intersection using fixed stack buffers (`[256]u8`, `[256]bool`).
- **Standard & exact modes**: `diceCoefficient` (case-insensitive, ignores whitespace) and `diceCaseSensitive` (exact case & whitespace).
- **Candidate ranking**: `bestMatchIndex(target, candidates, min_rating)` selects the top fuzzy match above threshold.

## Usage

```cp
import { diceCoefficient, bestMatchIndex } from "string-similarity";

const sim = diceCoefficient("healed", "sealed"); // 0.8
```
