[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fastq

Zero-allocation fast in-memory fixed-concurrency work queue with push, unshift, pause, resume, kill, and drain (clean-room MIT implementation of npm fastq, #253 / 192M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastq` ([https://github.com/mcollina/fastq](https://github.com/mcollina/fastq)) | 许可：`ISC`

| Metadata | Value |
| --- | --- |
| Package | `fastq` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fastq
```

```cp
import { FastQueue } from "fastq";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FastQueue` | `class` | Fields: `buf: [64]u64, head: usize, tail: usize, queued: usize, active: usize, concurrency: usize, +1 more` · Methods: `new, length, running, idle, isPaused, setConcurrency, +7 more` |

## Minimal Example

```cp
import { FastQueue } from "fastq";

pub fn main() {
    // Refer to src/index.d.cp for FastQueue fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
