# cli-spinners — AI Agent Quick Ref

Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols (cli-spinners / ora / log-symbols parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SpinnerKind,
    SymbolKind,
    intervalMs,
    frameCount,
    frameAt,
    frameAtTime,
    findSpinnerByName,
    logSymbol,
} from "cli-spinners";
```

## Core Types & Constants

```cp
enum SpinnerKind { Dots, Dots2, Line, Pipe, Arc, Circle, ... }
enum SymbolKind { Success, Error, Warning, Info }
```

## API Signatures

```cp
fn intervalMs(kind: SpinnerKind): u32;
fn frameCount(kind: SpinnerKind): usize;
fn frameAt(kind: SpinnerKind, tick: usize): string;
fn frameAtTime(kind: SpinnerKind, elapsed_ms: u64): string;
fn findSpinnerByName(name: string): SpinnerKind?;
fn logSymbol(kind: SymbolKind, ascii_only: bool): string;
fn renderSpinnerLine(kind: SpinnerKind, tick: usize, text: string, out: []u8): string;
fn renderStatusLine(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string;
```

## Minimal Usage

```cp
import { findSpinnerByName } from "cli-spinners";

pub fn main() {
    let result = findSpinnerByName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cli-spinners"`; install via `cap add cli-spinners`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
