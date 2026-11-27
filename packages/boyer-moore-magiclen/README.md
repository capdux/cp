[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# boyer-moore-magiclen

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:boyer-moore-magiclen` ([https://github.com/magiclen/boyer-moore-magiclen](https://github.com/magiclen/boyer-moore-magiclen))（原库名 `boyer-moore-magiclen` → CP 包名 `boyer-moore-magiclen`）

Zero-heap Boyer-Moore-Horspool sublinear substring search in pure CP.

## Features

- `compilePattern(needle)` & `findWithCompiled(haystack, needle, compiled)`: precomputed 256-byte bad-character shift table reusable across multiple texts
- `findFirst(haystack, needle)` & `findLast(haystack, needle)`: forward and reverse BMH search
- `countNonOverlapping(haystack, needle)` & `findIgnoreCaseAscii(haystack, needle)`

## Usage

```cp
import { compilePattern, findWithCompiled, findLast } from "boyer-moore-magiclen";

const pat = compilePattern("needle");
const pos = findWithCompiled("find a needle in haystack", "needle", pat); // 7
```
