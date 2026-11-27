# path-slash — AI Agent Quick Ref

Zero-heap POSIX '/' and Windows '\' path separator conversion (UNC, drive letters, lossy UTF-8) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    pshIsAsciiAlpha,
    pshIsSep,
    pshWriteByte,
    pshValidUtf8Len,
    pshIsAbs,
    toSlash,
    toSlashLossy,
    fromSlash,
} from "path-slash";
```

## API Signatures

```cp
fn pshIsAsciiAlpha(b: u8): bool;
fn pshIsSep(b: u8): bool;
fn pshWriteByte(out: []u8, pos: usize, b: u8): usize;
fn pshValidUtf8Len(s: string, offset: usize): usize;
fn pshIsAbs(path: string): bool;
fn toSlash(path: string, out: []u8): usize;
fn toSlashLossy(path: string, out: []u8): usize;
fn fromSlash(path: string, out: []u8): usize;
fn hasBackslash(path: string): bool;
fn hasForwardSlash(path: string): bool;
fn trimTrailingSlashes(path: string): string;
fn joinSlash(a: string, b: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { pshValidUtf8Len } from "path-slash";

pub fn main() {
    let result = pshValidUtf8Len("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"path-slash"`; install via `cap add path-slash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
