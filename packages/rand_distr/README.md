[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_distr

Zero-heap probability distribution samplers (Uniform, Normal Box-Muller, Exponential, Bernoulli, WeightedIndex) built on xoshiro256 (clean-room port of Rust rand_distr)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_distr` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand))（原库名 `rand_distr` → CP 包名 `rand_distr`）

| Metadata | Value |
| --- | --- |
| Package | `rand_distr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rand_distr
```

```cp
import {
    UniformRange,
    NormalDistr,
    ExpDistr,
    BernoulliDistr,
    WeightedIndex,
    rdstAbs,
    rdstSqrt,
    rdstLn,
} from "rand_distr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UniformRange` | `class` | Fields: `low: f64, high: f64` · Methods: `new, sample` |
| `NormalDistr` | `class` | Fields: `mean: f64, std_dev: f64` · Methods: `new, sample` |
| `ExpDistr` | `class` | Fields: `lambda: f64` · Methods: `new, sample` |
| `BernoulliDistr` | `class` | Fields: `p: f64` · Methods: `new, sample` |
| `WeightedIndex` | `class` | Fields: `cumulative: [16]f64, count: usize, total: f64` · Methods: `fromWeights, sample` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rdstAbs` | `(x: f64): f64` | Invokes `rdstAbs` and returns `f64`. |
| `rdstSqrt` | `(x: f64): f64` | Invokes `rdstSqrt` and returns `f64`. |
| `rdstLn` | `(x: f64): f64` | Invokes `rdstLn` and returns `f64`. |
| `rdstCos` | `(angle: f64): f64` | Invokes `rdstCos` and returns `f64`. |
| `rdstOpen01` | `(rng: &mut Xoshiro256): f64` | Invokes `rdstOpen01` and returns `f64`. |

## Minimal Example

```cp
import { rdstAbs } from "rand_distr";

pub fn main() {
    let result = rdstAbs(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
