# module-details-from-path — AI Agent Quick Ref

Zero-heap node_modules and cap_modules path parser extracting package name, basedir, and subpath (`v1.0.4`, `MIT`)

## Import

```cp
import {
    ModuleDetails,
    mdfpIsSep,
    mdfpMatchDir,
    mdfpSkipSeps,
    mdfpFindSep,
    mdfpEmpty,
    parseModulePath,
} from "module-details-from-path";
```

## Core Types & Constants

```cp
class ModuleDetails { name: string, basedir: string, subpath: string, found: bool }
```

## API Signatures

```cp
fn mdfpIsSep(ch: u8): bool;
fn mdfpMatchDir(s: string, pos: usize, dir: string): bool;
fn mdfpSkipSeps(s: string, start: usize): usize;
fn mdfpFindSep(s: string, start: usize): usize;
fn mdfpEmpty(): ModuleDetails;
fn parseModulePath(file_path: string): ModuleDetails;
```

## Minimal Usage

```cp
import { parseModulePath } from "module-details-from-path";

pub fn main() {
    let result = parseModulePath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"module-details-from-path"`; install via `cap add module-details-from-path`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
