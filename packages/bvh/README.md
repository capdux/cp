# `@cap/bvh`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bvh` ([https://github.com/svenstaro/bvh](https://github.com/svenstaro/bvh))

Zero-heap-allocation static 3D AABB Bounding Volume Hierarchy (BVH) in pure CP.

## Features

- **`Aabb` Primitives**: `merge`, `overlaps`, `containsPoint`, `surfaceArea`, and Slab `rayHit`.
- **Balanced Static BVH (`MAX_ITEMS = 128`)**: Iterative longest-axis median-split builder with at most 255 nodes.
- **Fast Queries**: `queryAabb` overlap detection and branch-and-bound closest-hit `raycast`.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { MAX_ITEMS, Aabb, BvhItem, RayHit, Bvh } from "@cap/bvh";
```

## License

MIT
