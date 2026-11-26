# @cap/aabb2

Zero-heap-allocation 2D Axis-Aligned Bounding Box (`Aabb2`) collision detection, union, intersection, point distance, and 2D ray slab intersection library in pure CP.

## Features

- **Types**: `Aabb2 { min_x, min_y, max_x, max_y }`, `Pt2 { x, y }`
- **Constructors**: `Aabb2.new`, `Aabb2.fromCenterSize`, `Aabb2.fromPoints`, `Aabb2.empty`
- **Queries & Ops**: `width`, `height`, `area`, `perimeter`, `center`, `isValid`, `containsPoint`, `containsBox`, `intersects`, `intersection`, `unionBox`, `expandBy`, `expandToPoint`, `distanceSqToPoint`, `rayIntersects` (2D Slab test)

## License

MIT
