# which — AI Agent Quick Ref

Zero-heap PATH executable candidate resolver for POSIX and Windows (clean-room CP implementation of npm which) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WhichIterator,
    whEqIgnoreCase,
    isExplicitPath,
    countPathDirs,
    pathDirAt,
    hasPathExt,
    formatCandidatePath,
} from "which";
```

## Core Types & Constants

```cp
class WhichIterator { cmd: string, path_env: string, pathext: string, delim: u8, /* +5 fields */ } // methods: newPosix(...) +2
```

## API Signatures

```cp
fn whEqIgnoreCase(a: string, b: string): bool;
fn isExplicitPath(cmd: string): bool;
fn countPathDirs(path_env: string, delim: u8): usize;
fn pathDirAt(path_env: string, delim: u8, idx: usize, out: []u8): usize;
fn hasPathExt(cmd: string, pathext: string): bool;
fn formatCandidatePath(dir: string, cmd: string, ext: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isExplicitPath } from "which";

pub fn main() {
    let result = isExplicitPath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"which"`; install via `cap add which`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
