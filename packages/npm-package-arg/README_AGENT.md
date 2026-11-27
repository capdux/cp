# npm-package-arg — AI Agent Quick Ref

Zero-heap npm package name validator and package specifier (@scope/name@version) parser and formatter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PackageSpec,
    isValidSegment,
    isValidPackageName,
    parsePackageSpec,
    appendSlice,
    formatPackageSpec,
} from "npm-package-arg";
```

## Core Types & Constants

```cp
class PackageSpec { scope: string, name: string, full_name: string, version_spec: string, /* +2 fields */ }
```

## API Signatures

```cp
fn isValidSegment(seg: string): bool;
fn isValidPackageName(name: string): bool;
fn parsePackageSpec(input: string): PackageSpec;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn formatPackageSpec(spec: PackageSpec, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidSegment } from "npm-package-arg";

pub fn main() {
    let result = isValidSegment("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"npm-package-arg"`; install via `cap add npm-package-arg`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
