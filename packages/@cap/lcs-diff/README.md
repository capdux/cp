# @cap/lcs-diff

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator in pure CP.

## Features

- `lcsLength(a, b)`: rolling two-row `[257]usize` DP (up to `MAX_LEN = 256`)
- `diffStats(a, b)`: exact `common`, `insertions`, and `deletions` counts
- `lcsSimilarity(a, b)`: Dice/LCS similarity `2 * common / (len(a) + len(b))`
- `longestCommonSubstring(a, b)`: returns `{ a_start, b_start, len }` of the longest contiguous common substring

## Usage

```cp
import { lcsLength, diffStats, longestCommonSubstring } from "@cap/lcs-diff";

const n = lcsLength("AGGTAB", "GXTXAYB"); // 4 ("GTAB")
const sub = longestCommonSubstring("ABABC", "BABCA"); // len=4 ("BABC")
```
