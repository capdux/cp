[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# suffix

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:suffix` ([https://github.com/BurntSushi/suffix](https://github.com/BurntSushi/suffix))（原库名 `suffix` → CP 包名 `suffix`）

Zero-heap static Suffix Array + Kasai O(N) LCP array in pure CP.

## Features

- Fixed-capacity zero-heap storage (`MAX_TEXT = 256` bytes)
- Lexicographically sorted Suffix Array (`sa`) + Kasai O(N) Longest Common Prefix (`lcp`) array
- `O(M log N)` binary search via `contains(pat)` and `count(pat)`
- `O(N)` longest repeated substring detection via `longestRepeatedSubstring()`

## Usage

```cp
import { SuffixArray } from "suffix";

const sa = SuffixArray.build("banana");
// sa.suffixAt(0..6) => [5, 3, 1, 0, 4, 2]
// sa.lcpAt(0..6)    => [0, 1, 3, 0, 0, 2]
const found = sa.contains("ana");   // true
const times = sa.count("ana");      // 2
const lrs = sa.longestRepeatedSubstring(); // start=1, len=3, found=true
```
