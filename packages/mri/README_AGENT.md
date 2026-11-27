# mri — AI Agent Quick Ref

Zero-allocation POSIX/GNU CLI argument parser (--flag, --key=val, --key val, -abc short clusters, -- terminator, stdin '-') in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedArg,
    strEq,
    parseArgs,
    hasFlag,
    getOption,
    countPositionals,
} from "mri";
```

## Core Types & Constants

```cp
class ParsedArg { kind: u8, name: string, value: string }
```

## API Signatures

```cp
fn strEq(a: string, b: string): bool;
fn parseArgs(argv: &[]string, out: []ParsedArg): usize;
fn hasFlag(argv: &[]string, long_name: string, short_char: u8): bool;
fn getOption(argv: &[]string, long_name: string, short_char: u8): string;
fn countPositionals(argv: &[]string): usize;
```

## Minimal Usage

```cp
import { parseArgs } from "mri";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */, /* out: []ParsedArg */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mri"`; install via `cap add mri`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
