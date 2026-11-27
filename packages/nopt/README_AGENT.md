# nopt — AI Agent Quick Ref

Zero-allocation CLI option parser with shorthands, --no-flag negation, and -- terminator (npm nopt parity) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import {
    NoptFlag,
    NoptShorthand,
    NoptResult,
    nptStrEq,
    nptHasPrefix,
    nptIndexOfByte,
    nptStripLeadingDashes,
    nptFindShorthand,
} from "nopt";
```

## Core Types & Constants

```cp
class NoptFlag { key: string, value: string, is_bool: bool, bool_val: bool }
class NoptShorthand { short_key: string, long_key: string }
class NoptResult { flags: [64]NoptFlag, flag_count: usize, remain: [64]string, remain_count: usize } // methods: new(...) +7
```

## API Signatures

```cp
fn nptStrEq(a: string, b: string): bool;
fn nptHasPrefix(s: string, prefix: string): bool;
fn nptIndexOfByte(s: string, ch: u8): usize?;
fn nptStripLeadingDashes(s: string): string;
fn nptFindShorthand(shorthands: &[]NoptShorthand, short_name: string): string?;
fn nptIsOptionToken(arg: string): bool;
fn parseNopt(args: &[]string, shorthands: &[]NoptShorthand): NoptResult;
```

## Minimal Usage

```cp
import { parseNopt } from "nopt";

pub fn main() {
    let result = parseNopt(/* args: &[]string */, /* shorthands: &[]NoptShorthand */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"nopt"`; install via `cap add nopt`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
