# npm-run-path — AI Agent Quick Ref

Zero-heap PATH builder that prepends ancestor node_modules/.bin (and cap_modules/.bin) directories for local binary execution (npm npm-run-path equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RunPathOptions,
    nrpPushBytes,
    nrpPushChar,
    nrpIsSep,
    dirnameOf,
    countAncestorDirs,
    buildRunPath,
    pathKey,
} from "npm-run-path";
```

## Core Types & Constants

```cp
class RunPathOptions { cwd: string, path_env: string, exec_path: string, prefer_local: bool, /* +3 fields */ } // methods: defaults(...) +1
```

## API Signatures

```cp
fn nrpPushBytes(out: []u8, pos: usize, s: string): usize;
fn nrpPushChar(out: []u8, pos: usize, c: u8): usize;
fn nrpIsSep(c: u8, is_windows: bool): bool;
fn dirnameOf(path: string, is_windows: bool): string;
fn countAncestorDirs(cwd: string, is_windows: bool): usize;
fn buildRunPath(opts: RunPathOptions, out: []u8): usize;
fn pathKey(is_windows: bool): string;
```

## Minimal Usage

```cp
import { nrpPushBytes } from "npm-run-path";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nrpPushBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"npm-run-path"`; install via `cap add npm-run-path`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
