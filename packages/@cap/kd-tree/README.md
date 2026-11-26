# `@cap/kd-tree`

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
