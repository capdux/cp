# figures — AI Agent Quick Ref

Zero-allocation Unicode terminal symbols with automatic ASCII/Windows fallbacks composing is-unicode-supported in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FigureKind,
    figStartsAt,
    figWriteStr,
    figureFor,
    figureForEnv,
    replaceSymbols,
} from "figures";
```

## Core Types & Constants

```cp
enum FigureKind { Tick, Cross, Star, Square, Circle, Info, ... }
```

## API Signatures

```cp
fn figStartsAt(input: string, pos: usize, needle: string): bool;
fn figWriteStr(out: []u8, pos: usize, s: string): usize;
fn figureFor(kind: FigureKind, unicode_ok: bool): string;
fn figureForEnv(kind: FigureKind, env: EnvInfo): string;
fn replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { figStartsAt } from "figures";

pub fn main() {
    let result = figStartsAt("example", 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"figures"`; install via `cap add figures`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
