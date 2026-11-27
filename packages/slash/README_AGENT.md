# slash — AI Agent Quick Ref

Zero-allocation Windows backslash <-> POSIX forward-slash path normalizer preserving extended-length (\\?\) paths for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { isExtendedLengthPath, slash, toBackslash, countSeparators } from "slash";
```

## API Signatures

```cp
fn isExtendedLengthPath(path: string): bool;
fn slash(path: string, out: []u8): usize;
fn toBackslash(path: string, out: []u8): usize;
fn countSeparators(path: string): usize;
```

## Minimal Usage

```cp
import { isExtendedLengthPath } from "slash";

pub fn main() {
    let result = isExtendedLengthPath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"slash"`; install via `cap add slash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add slash
cpc check src/main.cp
```
