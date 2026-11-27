[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# commondir

Zero-heap computation of the deepest common parent directory across a list of file paths, with POSIX path normalization, relative resolution, and base-directory resolution in pure CP.

- **Package**: `commondir` (`v1.0.1`)
- **Upstream Alignment**: Clean-room CP port of `npm/commondir` (`v1.0.1`, `MIT`, `substack/node-commondir`), supporting both `commondir(files)` and `commondir(basedir, relfiles)` forms.

## Installation & Import

```bash
cap add commondir
```

```typescript
import {
    COMMONDIR_MAX_PATH,
    normalizePosixPath, resolvePosixPath,
    commonDirTwo, commonDir,
    commonDirTwoNormalized, commonDirNormalized, commonDirWithBase,
} from "commondir";
```

## Core API

| Symbol | Signature / Value | Description |
| --- | --- | --- |
| `COMMONDIR_MAX_PATH` | `1024` (`usize`) | Maximum internal path normalization buffer size in bytes. |
| `normalizePosixPath` | `(path: string, out: []u8): usize` | Collapses duplicate slashes, `.`, and `..` segments into `out`; returns bytes written. |
| `resolvePosixPath` | `(base: string, rel: string, out: []u8): usize` | Resolves `rel` against `base` (or normalizes `rel` if absolute) into `out`. |
| `commonDirTwo` | `(a: string, b: string): string` | Zero-copy deepest common parent directory of two already-normalized paths (borrows from `a`). |
| `commonDir` | `(paths: &[]string): string` | Zero-copy deepest common parent directory across a slice of normalized paths (borrows from `paths[0]`). |
| `commonDirTwoNormalized` | `(a: string, b: string, out: []u8): usize` | Normalizes both paths first, then writes their common parent directory into `out`. |
| `commonDirNormalized` | `(paths: &[]string, out: []u8): usize` | Normalizes all paths first, then writes their deepest common directory into `out`. |
| `commonDirWithBase` | `(base: string, files: &[]string, out: []u8): usize` | Resolves each relative path in `files` against `base` and writes the common directory into `out` (matches `commondir(basedir, relfiles)`). |

## Examples

### 1. Zero-Copy Common Directory on Normalized Paths

```typescript
import { commonDir, commonDirTwo } from "commondir";

fn main() {
    let paths: [3]string = [
        "/x/y/z/a.txt",
        "/x/y/w/b.txt",
        "/x/y/z/c.txt",
    ];
    let dir = commonDir(paths[0..]);             // "/x/y"
    let two = commonDirTwo("/a/b/c", "/a/b/d");  // "/a/b"
}
```

### 2. Unnormalized Paths with `..` and Duplicate Slashes

```typescript
import { commonDirNormalized, normalizePosixPath } from "commondir";

fn main() {
    let paths: [2]string = [
        "/a/b/../c/x.cp",
        "/a//c/./y.cp",
    ];
    let out: [256]u8 = undefined;
    let n = commonDirNormalized(paths[0..], out[0..]);
    // out[0..n] == "/a/c"
}
```

### 3. Relative Files with Base Directory (`commondir(base, files)`)

```typescript
import { commonDirWithBase } from "commondir";

fn main() {
    let rels: [2]string = ["../c/foo.cp", "../c/bar/baz.cp"];
    let out: [256]u8 = undefined;
    let n = commonDirWithBase("/a/b", rels[0..], out[0..]);
    // out[0..n] == "/a/c"
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: `commonDir` / `commonDirTwo` return zero-copy slices directly borrowing input memory; normalized variants write into caller-supplied `out: []u8` stack buffers.
- **Explicit Normalization Split**: Separates zero-copy `commonDir` (for clean paths) from `commonDirNormalized` and `commonDirWithBase` (for paths containing `.` / `..` / `//` or relative bases).
- **Safe Boundary Handling**: Correctly distinguishes `/foo` vs `/foobar` (segment-boundary comparison, never partial prefix matches) and preserves root `"/"`.
