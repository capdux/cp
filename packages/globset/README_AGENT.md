# globset — AI Agent Quick Ref

Zero-allocation multi-glob set matcher and override engine (Rust globset v0.4 parity) composing picomatch and normalize-path (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GlobSetOptions,
    GlobSet,
    GlobOverride,
    gsToLower,
    gsByteEq,
    gsHasPrefix,
    gsToPicoOptions,
    gsMatchOne,
} from "globset";
```

## Core Types & Constants

```cp
class GlobSetOptions { case_insensitive: bool, literal_separator: bool, backslash_escape: bool } // methods: defaults(...)
class GlobSet { patterns: [32]string, base_buf: [32][64]u8, base_lens: [32]usize, count: usize, /* +1 fields */ } // methods: new(...) +10
class GlobOverride { set: GlobSet, negated: [32]bool, has_whitelist: bool } // methods: new(...) +7
```

## API Signatures

```cp
fn gsToLower(b: u8): u8;
fn gsByteEq(a: u8, b: u8, nocase: bool): bool;
fn gsHasPrefix(path: string, prefix: &[]u8, nocase: bool): bool;
fn gsToPicoOptions(opts: GlobSetOptions): PicoOptions;
fn gsMatchOne(normPath: string, pat: string, opts: GlobSetOptions, picoOpts: PicoOptions): bool;
```

## Minimal Usage

```cp
import { gsHasPrefix } from "globset";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = gsHasPrefix("example", out_buf[0..256], false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"globset"`; install via `cap add globset`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
