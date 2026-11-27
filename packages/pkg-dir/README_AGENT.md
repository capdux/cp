# pkg-dir — AI Agent Quick Ref

Zero-heap Node.js/CP package root directory finder locating nearest ancestor package.json or cap.json (npm pkg-dir equivalent, reuses find-up & npm-run-path) (`v1.0.0`, `MIT`)

## Import

```cp
import { pkdPush, packageDirectoryInList, capPackageDirectoryInList, packageDirectoryBounded } from "pkg-dir";
```

## API Signatures

```cp
fn pkdPush(out: []u8, pos: usize, s: string): usize;
fn packageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;
fn capPackageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;
fn packageDirectoryBounded(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?;
```

## Minimal Usage

```cp
import { pkdPush } from "pkg-dir";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pkdPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pkg-dir"`; install via `cap add pkg-dir`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add pkg-dir
cpc check src/main.cp
```
