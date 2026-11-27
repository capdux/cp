[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/kd-tree`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kiddo` ([https://github.com/sdd/kiddo](https://github.com/sdd/kiddo))（原库名 `kiddo` → CP 包名 `@cap/kd-tree`）

Zero-heap-allocation static 2D KD-Tree spatial index in pure CP.

## Features

- **Balanced Median-Split Construction**: Iterative stack-based builder supporting up to `MAX_POINTS = 256` 2D points.
- **Exact Nearest Neighbor (`nearest`)**: Branch-and-bound pruning guaranteed to match brute-force Euclidean distance.
- **Radius Range Search (`withinRadius`)**: Prunes subtrees outside `[-radius, +radius]` along each split axis.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { MAX_POINTS, KdPoint2D, NearestResult, KdTree2D } from "@cap/kd-tree";
```

## License

MIT
