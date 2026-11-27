[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/natural-cmp

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:natord` ([https://github.com/lifthrasiir/rust-natord](https://github.com/lifthrasiir/rust-natord))（原库名 `natord` → CP 包名 `@cap/natural-cmp`）

Zero-heap arbitrary-precision human natural sort comparator and in-place sorter in pure CP.

## Features

- `compareNatural(a, b)` & `compareNaturalIgnoreCase(a, b)`: overflow-free arbitrary-length numeric run comparison (`"file2.txt" < "file10.txt"`, `"v1.2" < "v1.10"`)
- `isSortedNatural(items)`: O(N) sorted-order check
- `sortNatural(items)`: zero-heap in-place Ciura Shell sort

## Usage

```cp
import { compareNatural, sortNatural } from "@cap/natural-cmp";

const cmp = compareNatural("file2.txt", "file10.txt"); // -1
```
