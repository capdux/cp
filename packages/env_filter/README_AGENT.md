# env_filter — AI Agent Quick Ref

Zero-allocation RUST_LOG-style hierarchical log filter parser and longest-prefix module matcher (Rust env_filter parity) for CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    LogLevel,
    FilterDirective,
    EnvFilter,
    efltLevelRank,
    parseLogLevel,
    efltMatchesTarget,
} from "env_filter";
```

## Core Types & Constants

```cp
enum LogLevel { Off, Error, Warn, Info, Debug, Trace }
class FilterDirective { target: string, level: LogLevel }
class EnvFilter { directives: [32]FilterDirective, count: usize } // methods: empty(...) +7
```

## API Signatures

```cp
fn efltLevelRank(lvl: LogLevel): u8;
fn parseLogLevel(s: string): LogLevel?;
fn efltMatchesTarget(target: string, directive_target: string): bool;
```

## Minimal Usage

```cp
import { parseLogLevel } from "env_filter";

pub fn main() {
    let result = parseLogLevel("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"env_filter"`; install via `cap add env_filter`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
