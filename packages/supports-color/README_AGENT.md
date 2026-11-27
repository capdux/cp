# supports-color — AI Agent Quick Ref

Zero-allocation terminal color capability detection (supports-color v9 parity) powered by has-flag in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ColorSupport,
    TerminalEnv,
    scContainsIgnoreCase,
    translateLevel,
    scDetectCliOverride,
    scParseForceColor,
    detectColorLevel,
    detectColorSupport,
} from "supports-color";
```

## Core Types & Constants

```cp
class ColorSupport { level: u8, has_basic: bool, has_256: bool, has_16m: bool }
class TerminalEnv { is_tty: bool, force_color: string, no_color: string, term: string, /* +6 fields */ } // methods: empty(...) +1
```

## API Signatures

```cp
fn scContainsIgnoreCase(haystack: string, needle: string): bool;
fn translateLevel(level: u8): ColorSupport;
fn scDetectCliOverride(argv: &[]string): i8;
fn scParseForceColor(val: string): i8;
fn detectColorLevel(env: TerminalEnv, argv: &[]string): u8;
fn detectColorSupport(env: TerminalEnv, argv: &[]string): ColorSupport;
```

## Minimal Usage

```cp
import { scContainsIgnoreCase } from "supports-color";

pub fn main() {
    let result = scContainsIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"supports-color"`; install via `cap add supports-color`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
