# clsx — AI Agent Quick Ref

Zero-heap conditional CSS class name builder with whitespace normalization, deduplication, and word-boundary checks (npm clsx/classnames parity). (`v1.0.0`, `MIT`)

## Import

```cp
import { CondClass, ClassBuilder, joinClasses, joinCondClasses } from "clsx";
```

## Core Types & Constants

```cp
class CondClass { name: &[]u8, enabled: bool }
class ClassBuilder { buf: []u8, pos: usize, overflow: bool } // methods: init(...) +10
```

## API Signatures

```cp
fn joinClasses(classes: &[]&[]u8, dst: []u8): i64;
fn joinCondClasses(items: &[]CondClass, dst: []u8): i64;
```

## Minimal Usage

```cp
import { joinClasses } from "clsx";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinClasses(/* classes: &[]&[]u8 */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"clsx"`; install via `cap add clsx`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
