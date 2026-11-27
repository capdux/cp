# @cap/is-path-inside

Zero-allocation path traversal containment guard checking whether `child_path` is strictly inside `parent_path` after normalizing `.`, `..`, and mixed POSIX/Windows separators.

## API

- `isPathInside(child_path: string, parent_path: string): bool` — returns `true` iff `child_path` is strictly inside `parent_path` (same path returns `false`, prefix collision `/a/bc` vs `/a/b` returns `false`, `..` escapes return `false`).
- `isPathInsideOrEqual(child_path: string, parent_path: string): bool` — returns `true` if `child_path` is inside or normalizes to the same path as `parent_path`.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`is-path-inside@4.0.0`)
- **Upstream Repository**: https://github.com/sindresorhus/is-path-inside
- **License**: MIT (clean-room CP implementation)
