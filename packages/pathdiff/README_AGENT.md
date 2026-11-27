# pathdiff — AI Agent Quick Ref

Zero-heap lexical relative path difference & normalization engine in pure CP (crates.io `pathdiff` parity).

## Import

```cp
import {
    PATHDIFF_MAX_SEGS,
    hasSameRoot,
    normalizePath,
    diffPaths,
    diffUtf8Paths,
} from "pathdiff";
```

## API Signatures

```cp
const PATHDIFF_MAX_SEGS: usize; // 64

fn hasSameRoot(path: string, base: string): bool;
fn normalizePath(path: string, out: []u8): usize?;
fn diffPaths(target: string, base: string, out: []u8): usize?;
fn diffUtf8Paths(path: string, base: string, out: []u8): usize?;
```

## Minimal Example

```cp
import { diffPaths, normalizePath } from "pathdiff";

let out: [128]u8 = undefined;
let n = diffPaths("/a/b/c", "/a/d", out[0..128]);
if (n != null) {
    // out[0..n!] == "../b/c"
}

let m = normalizePath("/a/x/../b/./c", out[0..128]);
if (m != null) {
    // out[0..m!] == "/a/b/c"
}
```

## Pitfalls & Borrowing Rules

- **Argument order**: `diffPaths(target, base, out)` computes the path to `target` **from** `base` (same order as Rust `pathdiff::diff_paths(path, base)`).
- **Optional return `usize?`**: Returns `null` if one path is absolute and the other is relative (`!hasSameRoot`), or if `out.len` is too small. Always check `!= null` before unwrapping.
- **No filesystem access**: Pure lexical normalization (`.` and `..`); symlinks are not resolved.
- **Segment cap**: Stack segment array holds at most `PATHDIFF_MAX_SEGS` (64) path components.
- **Zero-copy input**: `string` arguments are borrowed immutably; only `out: []u8` is mutated.
