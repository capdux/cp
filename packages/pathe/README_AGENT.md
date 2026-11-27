# pathe — AI Agent Quick Ref

Zero-allocation universal POSIX-normalized path utilities preserving Windows drive letters and URL protocols (clean-room MIT implementation of npm pathe, #219 / 207M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    pthIsSep,
    pthIsAlpha,
    pthIsSchemeChar,
    pthHasDrivePrefix,
    pthProtocolPrefixLen,
    pthPush,
    pthPushSlice,
    pthStrEq,
} from "pathe";
```

## API Signatures

```cp
fn pthIsSep(b: u8): bool;
fn pthIsAlpha(b: u8): bool;
fn pthIsSchemeChar(b: u8): bool;
fn pthHasDrivePrefix(s: string): bool;
fn pthProtocolPrefixLen(s: string): usize;
fn pthPush(out: []u8, pos: usize, b: u8): usize;
fn pthPushSlice(out: []u8, pos: usize, s: string): usize;
fn pthStrEq(a: string, b: string): bool;
fn isAbsolute(path: string): bool;
fn normalize(path: string, out: []u8): usize;
fn join(a: string, b: string, out: []u8): usize;
fn joinPath(a: string, b: string, out: []u8): usize;
fn pthStripTrailingSlash(out: []u8, len: usize): usize;
fn resolve(base: string, rel: string, out: []u8): usize;
fn dirname(path: string): string;
fn basename(path: string): string;
fn extname(path: string): string;
fn pthSplitSegments(s: string, starts: []usize, lens: []usize): usize;
fn relative(from_path: string, to_path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isAbsolute } from "pathe";

pub fn main() {
    let result = isAbsolute("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pathe"`; install via `cap add pathe`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
