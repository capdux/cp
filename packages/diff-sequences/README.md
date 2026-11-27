[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# diff-sequences

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:diff-sequences` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest)) / `crates.io:lcs-diff` ([https://github.com/bokuweb/lcs-diff](https://github.com/bokuweb/lcs-diff))（原库名 `diff-sequences` → CP 包名 `diff-sequences`）

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator in pure CP.

## Features

- `lcsLength(a, b)`: rolling two-row `[257]usize` DP (up to `MAX_LEN = 256`)
- `diffStats(a, b)`: exact `common`, `insertions`, and `deletions` counts
- `lcsSimilarity(a, b)`: Dice/LCS similarity `2 * common / (len(a) + len(b))`
- `longestCommonSubstring(a, b)`: returns `{ a_start, b_start, len }` of the longest contiguous common substring

## Usage

```cp
import { lcsLength, diffStats, longestCommonSubstring } from "diff-sequences";

const n = lcsLength("AGGTAB", "GXTXAYB"); // 4 ("GTAB")
const sub = longestCommonSubstring("ABABC", "BABCA"); // len=4 ("BABC")
```
