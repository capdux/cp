# has-flag — AI Agent Quick Ref

Check if CLI argv contains a specific flag (clean-room CP implementation of npm has-flag) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    hfEq,
    hfStartsWith,
    hfMatchesFlag,
    hfComputePrefixLen,
    findFlagIndex,
    hasFlag,
    getFlagValue,
    isFlagEnabled,
} from "has-flag";
```

## API Signatures

```cp
fn hfEq(a: string, b: string): bool;
fn hfStartsWith(s: string, prefix: string): bool;
fn hfMatchesFlag(arg: string, flag: string, prefix_len: usize): bool;
fn hfComputePrefixLen(flag: string): usize;
fn findFlagIndex(flag: string, argv: &[]string): i64;
fn hasFlag(flag: string, argv: &[]string): bool;
fn getFlagValue(flag: string, argv: &[]string, out: []u8): i64;
fn isFlagEnabled(flag: string, argv: &[]string, default_val: bool): bool;
```

## Minimal Usage

```cp
import { getFlagValue } from "has-flag";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getFlagValue("example", /* argv: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"has-flag"`; install via `cap add has-flag`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
