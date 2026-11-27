# optionator — AI Agent Quick Ref

Zero-heap CLI option specification and auto-aligned --help documentation generator in pure CP (`v0.9.4`, `MIT`)

## Import

```cp
import {
    OptionSpec,
    optnLeftColLen,
    formatOptionRow,
    generateHelp,
    findOptionSpec,
} from "optionator";
```

## Core Types & Constants

```cp
class OptionSpec { long_name: string, short_alias: string, value_type: string, description: string, /* +2 fields */ }
```

## API Signatures

```cp
fn optnLeftColLen(spec: OptionSpec): usize;
fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize;
fn generateHelp(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize;
fn findOptionSpec(specs: &[]OptionSpec, flag: string): OptionSpec?;
```

## Minimal Usage

```cp
import { findOptionSpec } from "optionator";

pub fn main() {
    let result = findOptionSpec(/* specs: &[]OptionSpec */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"optionator"`; install via `cap add optionator`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
