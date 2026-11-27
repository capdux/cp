# argparse — AI Agent Quick Ref

Zero-allocation Python-style CLI argument parser supporting short/long flags, counts, bundling, and required checks for CP (`v1.0.0`, `Python-2.0`)

## Import

```cp
import {
    ArgAction,
    ArgDef,
    ArgSlot,
    ParseResult,
    ArgParser,
    argpStrEq,
    argpStripDashes,
    argpFindEq,
} from "argparse";
```

## Core Types & Constants

```cp
enum ArgAction { Store, StoreTrue, StoreFalse, Count }
class ArgDef { short_flag: u8, long_name: string, dest: string, action: ArgAction, /* +3 fields */ }
class ArgSlot { dest: string, action: ArgAction, has_str: bool, str_val: string, /* +3 fields */ }
class ParseResult { ok: bool, err_code: i32, slot_count: usize, slots: [32]ArgSlot, /* +2 fields */ } // methods: getString(...) +4
class ArgParser { def_count: usize, defs: [32]ArgDef } // methods: init(...) +2
```

## API Signatures

```cp
fn argpStrEq(a: string, b: string): bool;
fn argpStripDashes(s: string): string;
fn argpFindEq(s: string): usize?;
fn argpResolveDest(def: ArgDef): string;
fn argpFindLong(defs: &[]ArgDef, name: string): usize?;
fn argpFindShort(defs: &[]ArgDef, ch: u8): usize?;
```

## Minimal Usage

```cp
import { argpStrEq } from "argparse";

pub fn main() {
    let result = argpStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"argparse"`; install via `cap add argparse`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
