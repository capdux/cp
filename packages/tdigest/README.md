[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tdigest

Zero-allocation Merging T-Digest streaming quantile (P50/P95/P99/P99.9) and CDF estimator with distributed merge for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tdigest` ([https://github.com/MnO2/t-digest](https://github.com/MnO2/t-digest))

| Metadata | Value |
| --- | --- |
| Package | `tdigest` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tdigest
```

```cp
import { Centroid, TDigest, emptyCentroid } from "tdigest";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Centroid` | `class` | Fields: `mean: f64, weight: f64` |
| `TDigest` | `class` | Fields: `centroids: [128]Centroid, centroid_len: usize, compression: f64, total_weight: f64, min_val: f64, max_val: f64, +1 more` · Methods: `init, count, min, max, centroidCount, isEmpty, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyCentroid` | `(): Centroid` | Invokes `emptyCentroid` and returns `Centroid`. |

## Minimal Example

```cp
import { emptyCentroid } from "tdigest";

pub fn main() {
    let result = emptyCentroid();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
