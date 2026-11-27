# path-parse — AI Agent Quick Ref

Zero-allocation POSIX and Win32 path.parse() splitter into { root, dir, base, ext, name } (clean-room MIT implementation of npm path-parse, #238 / 196M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedPath,
    PprNameExt,
    pprIsWinSep,
    pprIsAlpha,
    pprSplitBaseExt,
    parsePosix,
    parseWin32,
} from "path-parse";
```

## Core Types & Constants

```cp
class ParsedPath { root: string, dir: string, base: string, ext: string, /* +1 fields */ }
class PprNameExt { name: string, ext: string }
```

## API Signatures

```cp
fn pprIsWinSep(b: u8): bool;
fn pprIsAlpha(b: u8): bool;
fn pprSplitBaseExt(base: string): PprNameExt;
fn parsePosix(path: string): ParsedPath;
fn parseWin32(path: string): ParsedPath;
```

## Minimal Usage

```cp
import { parsePosix } from "path-parse";

pub fn main() {
    let result = parsePosix("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"path-parse"`; install via `cap add path-parse`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
