[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pathdiff

Zero-heap relative path difference calculator computing a target path relative to a base directory.

Aligned with Rust crates.io [`pathdiff`](https://github.com/Manishearth/pathdiff) (MIT OR Apache-2.0).

## Installation & Import

```bash
cap add pathdiff
```

```cp
import {
    PATHDIFF_MAX_SEGS,
    hasSameRoot,
    normalizePath,
    diffPaths,
    diffUtf8Paths,
} from "pathdiff";
```

## Core API

### Constants

| Symbol | Type | Description |
| --- | --- | --- |
| `PATHDIFF_MAX_SEGS` | `usize = 64` | Maximum number of normalized path segments held on the stack. |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hasSameRoot` | `(path: string, base: string): bool` | Checks whether `path` and `base` share the same root (both absolute POSIX `/` or both relative). |
| `normalizePath` | `(path: string, out: []u8): usize?` | Lexically normalizes `.` and `..` segments into `out`; returns written byte length or `null` on buffer overflow. |
| `diffPaths` | `(target: string, base: string, out: []u8): usize?` | Computes relative path from `base` to `target` into `out`; returns written length or `null` if roots differ or `out` is too small. |
| `diffUtf8Paths` | `(path: string, base: string, out: []u8): usize?` | UTF-8 alias for `diffPaths` (mirrors `pathdiff::diff_utf8_paths` in Rust). |

## Examples

### 1. Computing Relative Path Between Directories

```cp
import { print } from "cap:io";
import { diffPaths } from "pathdiff";

pub fn main() {
    let buf: [128]u8 = undefined;
    let n = diffPaths("/usr/local/bin/cp", "/usr/share/doc", buf[0..128]);
    if (n != null) {
        print("relative: {}\n", buf[0..n!]); // "../../local/bin/cp"
    }
}
```

### 2. Lexical Path Normalization

```cp
import { print } from "cap:io";
import { normalizePath } from "pathdiff";

pub fn main() {
    let buf: [128]u8 = undefined;
    let n = normalizePath("/a/b/../c/./d", buf[0..128]);
    if (n != null) {
        print("normalized: {}\n", buf[0..n!]); // "/a/c/d"
    }
}
```

### 3. Root Compatibility Check

```cp
import { print } from "cap:io";
import { hasSameRoot, diffUtf8Paths } from "pathdiff";

pub fn main() {
    if (hasSameRoot("/foo/bar", "baz/qux")) {
        let out: [64]u8 = undefined;
        let _ = diffUtf8Paths("/foo/bar", "baz/qux", out[0..64]);
    } else {
        print("cannot diff absolute and relative paths\n");
    }
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: Writes UTF-8 output directly into a caller-provided `out: []u8` buffer instead of allocating `PathBuf` / `Utf8PathBuf`.
- **Optional overflow safety**: Returns `usize?` (`null` on incompatible roots or insufficient output capacity) rather than panicking or truncating silently.
- **Stack segment limit**: Handles up to `PATHDIFF_MAX_SEGS = 64` path segments purely on stack memory without filesystem I/O.
