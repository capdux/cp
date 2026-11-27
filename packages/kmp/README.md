[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# kmp

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kmp` ([https://github.com/magiclen/kmp](https://github.com/magiclen/kmp))（原库名 `kmp` → CP 包名 `kmp`）

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS prefix-function engine for CP.

## Features

- **Zero heap allocation**: uses a fixed stack LPS table (up to 256-byte patterns, with exact fallback for longer patterns).
- **Strict linear-time search**: immune to quadratic worst-case inputs such as `"aaaa...b"`.
- **Full API**: `computeLps`, `findFirst`, `findAll` (overlapping & non-overlapping), `countMatches`, and `contains`.

## Usage

```cp
import { findFirst, findAll, countMatches } from "kmp";

const idx = findFirst("ABABDABACDABABCABAB", "ABABCABAB"); // 10
const n = countMatches("aaaaa", "aa", true); // 4
```
