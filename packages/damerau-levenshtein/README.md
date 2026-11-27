[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# damerau-levenshtein

Zero-heap Damerau-Levenshtein (Optimal String Alignment & True Damerau-Levenshtein) edit distance and similarity ratio engine for CP.

Aligned with npm [`damerau-levenshtein`](https://github.com/tad-lispy/node-damerau-levenshtein) `1.0.8` (MIT).

## Installation & Import

```bash
cap add damerau-levenshtein
```

```cp
import {
    DamLevResult,
    DL_MAX_OSA_LEN,
    DL_MAX_TRUE_LEN,
    damerauLevenshtein,
    damerauLevenshteinSteps,
    damerauLevenshteinBounded,
    trueDamerauLevenshtein,
    similarity,
    trueSimilarity,
} from "damerau-levenshtein";
```

## Core API

### Constants & Types

| Symbol | Type / Fields | Description |
| --- | --- | --- |
| `DL_MAX_OSA_LEN` | `usize = 255` | Maximum byte length supported by stack-allocated OSA DP buffers. |
| `DL_MAX_TRUE_LEN` | `usize = 64` | Maximum byte length supported by stack-allocated true Damerau-Levenshtein DP buffers. |
| `DamLevResult` | `{ steps: usize, relative: f64, similarity: f64 }` | Edit distance result (`relative = steps / maxLen`, `similarity = 1.0 - relative`). |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `damerauLevenshtein` | `(a: string, b: string): DamLevResult` | Compute OSA edit distance, relative distance, and similarity ratio. |
| `damerauLevenshteinSteps` | `(a: string, b: string): usize` | Compute OSA edit distance in steps for UTF-8 strings. |
| `damerauLevenshteinStepsBytes` | `(a: []const u8, b: []const u8): usize` | Compute OSA edit distance on raw byte slices. |
| `osaDistance` | `(a: string, b: string): usize` | Alias for `damerauLevenshteinSteps`. |
| `osaDistanceBytes` | `(a: []const u8, b: []const u8): usize` | Alias for `damerauLevenshteinStepsBytes`. |
| `trueDamerauLevenshtein` | `(a: string, b: string): DamLevResult` | Compute unrestricted (true) Damerau-Levenshtein result allowing edits on transposed substrings. |
| `trueDamerauLevenshteinSteps` | `(a: string, b: string): usize` | Compute unrestricted Damerau-Levenshtein step count for strings. |
| `trueDamerauLevenshteinBytes` | `(a: []const u8, b: []const u8): usize` | Compute unrestricted Damerau-Levenshtein step count for byte slices. |
| `damerauLevenshteinBounded` | `(a: string, b: string, maxDistance: usize): usize?` | Banded OSA distance returning `null` early if distance exceeds `maxDistance`. |
| `damerauLevenshteinBoundedBytes` | `(a: []const u8, b: []const u8, maxDistance: usize): usize?` | Byte-slice variant of bounded OSA distance; returns `null` when exceeding threshold. |
| `similarity` | `(a: string, b: string): f64` | Normalized OSA similarity in `[0.0, 1.0]`. |
| `trueSimilarity` | `(a: string, b: string): f64` | Normalized true Damerau-Levenshtein similarity in `[0.0, 1.0]`. |

## Examples

### 1. Basic Edit Distance & Similarity

```cp
import { print } from "cap:io";
import { damerauLevenshtein, similarity } from "damerau-levenshtein";

pub fn main() {
    let res = damerauLevenshtein("kitten", "sitting");
    print("steps={}, relative={}, similarity={}\n", res.steps, res.relative, res.similarity);

    let score = similarity("abcdef", "abcfed");
    print("similarity={}\n", score);
}
```

### 2. Bounded Early-Exit Fuzzy Matching

```cp
import { print } from "cap:io";
import { damerauLevenshteinBounded } from "damerau-levenshtein";

pub fn main() {
    let maybe = damerauLevenshteinBounded("recieve", "receive", 2);
    if (maybe != null) {
        print("matched within threshold: {} edits\n", maybe!);
    } else {
        print("too far apart\n");
    }
}
```

### 3. OSA vs. True Damerau-Levenshtein

```cp
import { print } from "cap:io";
import { osaDistance, trueDamerauLevenshteinSteps } from "damerau-levenshtein";

pub fn main() {
    // "ca" -> "abc": OSA = 3, True Damerau-Levenshtein = 2
    let osa = osaDistance("ca", "abc");
    let true_dl = trueDamerauLevenshteinSteps("ca", "abc");
    print("OSA={}, True={}\n", osa, true_dl);
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: All DP tables live on the stack (bounded by `DL_MAX_OSA_LEN = 255` and `DL_MAX_TRUE_LEN = 64`), avoiding GC and allocator overhead.
- **Both OSA & True algorithms**: Upstream npm `damerau-levenshtein` only provides OSA; CP additionally exposes unrestricted true Damerau-Levenshtein (`trueDamerauLevenshtein*`) and early-exit banded search (`damerauLevenshteinBounded*`).
- **Read-only borrowing**: All input slices (`[]const u8` / `string`) are borrowed immutably with zero copies.
