[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sketches-ddsketch

Zero-heap DDSketch relative-accuracy quantile sketch for low-memory latency statistics

| Metadata | Value |
| --- | --- |
| Package | `sketches-ddsketch` |
| Version | `0.3.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sketches-ddsketch
```

```cp
import { DDSketch, ddskZeroBuckets, ddskClampAlpha, ddskAddKeyCount } from "sketches-ddsketch";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DDSketch` | `class` | Fields: `buckets: [DDSK_BUCKETS]u64, alpha: f64, gamma: f64, log_gamma: f64, min_index: i32, has_anchor: bool, +5 more` · Methods: `new, add, count, sum, min, max, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ddskZeroBuckets` | `(): [DDSK_BUCKETS]u64` | Invokes `ddskZeroBuckets` and returns `[DDSK_BUCKETS]u64`. |
| `ddskClampAlpha` | `(alpha: f64): f64` | Invokes `ddskClampAlpha` and returns `f64`. |
| `ddskAddKeyCount` | `(sk: &mut DDSketch, key: i32, cnt: u64)` | Executes `ddskAddKeyCount` in-place. |

## Minimal Example

```cp
import { ddskZeroBuckets } from "sketches-ddsketch";

pub fn main() {
    let result = ddskZeroBuckets();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
