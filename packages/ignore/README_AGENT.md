# ignore — AI Agent Quick Ref

Zero-heap .gitignore specification rule engine (clean-room CP implementation of npm ignore) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    IgnoreRule,
    IgnoreMatcher,
    igTrimRightSpaces,
    igHasInnerSlash,
    igRuleMatches,
    igMatchSingle,
} from "ignore";
```

## Core Types & Constants

```cp
class IgnoreRule { pat: [128]u8, pat_len: usize, negated: bool, dir_only: bool, /* +1 fields */ }
class IgnoreMatcher { rules: [64]IgnoreRule, count: usize, ignore_case: bool } // methods: new(...) +6
```

## API Signatures

```cp
fn igTrimRightSpaces(s: string): usize;
fn igHasInnerSlash(s: string, start: usize, end: usize): bool;
fn igRuleMatches(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool;
fn igMatchSingle(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool;
```

## Minimal Usage

```cp
import { igTrimRightSpaces } from "ignore";

pub fn main() {
    let result = igTrimRightSpaces("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ignore"`; install via `cap add ignore`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
