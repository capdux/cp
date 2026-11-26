# @cap/kmp-search

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS prefix-function engine for CP.

## Features

- **Zero heap allocation**: uses a fixed stack LPS table (up to 256-byte patterns, with exact fallback for longer patterns).
- **Strict linear-time search**: immune to quadratic worst-case inputs such as `"aaaa...b"`.
- **Full API**: `computeLps`, `findFirst`, `findAll` (overlapping & non-overlapping), `countMatches`, and `contains`.

## Usage

```cp
import { findFirst, findAll, countMatches } from "@cap/kmp-search";

const idx = findFirst("ABABDABACDABABCABAB", "ABABCABAB"); // 10
const n = countMatches("aaaaa", "aa", true); // 4
```
