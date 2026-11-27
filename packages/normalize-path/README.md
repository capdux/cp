[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `normalize-path`

> **Source Attribution**: `npm:normalize-path` ([jonschlinkert/normalize-path](https://github.com/jonschlinkert/normalize-path)) — License: MIT

Zero-heap POSIX/Windows path normalizer with backslash-to-slash conversion, duplicate slash collapsing, dot-segment resolution, and UNC / Windows drive root preservation in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`normalize-path`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: normalize-path)
import normalizePath from "normalize-path";
normalizePath("\\foo\\bar\\baz\\");       // "/foo/bar/baz"
normalizePath("\\foo\\bar\\baz\\", false); // "/foo/bar/baz/"
```

```cp
// CP (zero-heap, direct buffer output)
import { normalizePath, normalizePathDefault } from "normalize-path";

let out: [128]u8 = undefined;
let n1 = normalizePathDefault("\\foo\\bar\\baz\\", out[0..128]); // "/foo/bar/baz"
let n2 = normalizePath("\\foo\\bar\\baz\\", false, out[0..128]); // "/foo/bar/baz/"
```

## Installation

Add `normalize-path` to your `cap.json`:

```json
{
  "dependencies": {
    "normalize-path": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `NormalizePathOptions` | `class { strip_trailing: bool, resolve_dots: bool, fn defaults(): NormalizePathOptions }` | Configurable normalization options |
| `normalizePath` | `(path: string, strip_trailing: bool, out: []u8): usize` | Convert `\` to `/`, collapse slashes, preserve UNC `//` & root `/` |
| `normalizePathDefault` | `(path: string, out: []u8): usize` | Shorthand for `normalizePath(path, true, out)` |
| `normalizePathWith` | `(path: string, opts: NormalizePathOptions, out: []u8): usize` | Full normalization with optional `.` / `..` resolution |
| `toPosixPath` | `(path: string, out: []u8): usize` | Convert all backslashes to forward slashes |
| `stripTrailingSlash` | `(path: string): string` | Zero-copy strip of trailing `/` (preserves `/` and drive root) |
| `hasBackslash` | `(path: string): bool` | Check if path contains `\` |
| `isUncPath` | `(path: string): bool` | Detect UNC paths (`\\server\share` or `//server/share`) |
| `isWindowsDrivePath` | `(path: string): bool` | Detect Windows drive paths (`C:\...` or `C:/...`) |
| `npIsSlash` / `npIsAsciiAlpha` | `(ch: u8): bool` | Low-level byte predicates |

## Practical Examples

### 1. Standard & Windows / UNC Normalization

```cp
import { normalizePathDefault, normalizePath, isUncPath, isWindowsDrivePath } from "normalize-path";

let out: [128]u8 = undefined;

let n1 = normalizePathDefault("\\\\server\\share\\\\dir\\", out[0..128]); // "//server/share/dir"
let n2 = normalizePathDefault("C:\\Users\\\\dev\\", out[0..128]);         // "C:/Users/dev"
let n3 = normalizePath("foo/bar/", false, out[0..128]);                   // "foo/bar/"

let unc = isUncPath("\\\\server\\share");   // true
let drv = isWindowsDrivePath("C:\\src");    // true
```

### 2. Resolving Dot Segments & Zero-Copy Helpers

```cp
import { NormalizePathOptions, normalizePathWith, toPosixPath, stripTrailingSlash } from "normalize-path";

let out: [128]u8 = undefined;

let opts = NormalizePathOptions.defaults();
opts.resolve_dots = true;
opts.strip_trailing = true;

let n1 = normalizePathWith("a/b/../c/./d/", opts, out[0..128]); // "a/c/d"
let n2 = toPosixPath("src\\lib\\index.cp", out[0..128]);        // "src/lib/index.cp"
let clean = stripTrailingSlash("/var/log///");                  // "/var/log"
```

## Notes & Edge Cases

- **Root preservation**: Single root `"/"` and Windows drive root `"C:/"` are preserved even when `strip_trailing` is `true`.
- **UNC preservation**: Leading double slashes (`\\host\share` → `//host/share`) are preserved instead of collapsed to a single `/`.
- **Zero heap allocations**: Buffer-writing functions return the written byte count `usize`; `stripTrailingSlash` returns a zero-copy slice.
