# yargs-parser — AI Agent Quick Ref

Zero-allocation CLI argument parser (npm yargs-parser parity) with long/short flags, --no- negation, camelCase/kebab-case key lookup, numeric/boolean coercion, and -- rest passthrough for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedItem,
    ParsedEntry,
    ParsedArgs,
    ypIsNegativeNumber,
    ypFormatInt,
    ypIsFlagToken,
    ypEmptyItem,
    ypEmptyEntry,
} from "yargs-parser";
```

## Core Types & Constants

```cp
class ParsedItem { data: [128]u8, len: usize }
class ParsedEntry { key: [64]u8, key_len: usize, camel_key: [64]u8, camel_len: usize, /* +7 fields */ }
class ParsedArgs { entries: [32]ParsedEntry, entry_count: usize, positionals: [32]ParsedItem, pos_count: usize, /* +2 fields */ } // methods: findIndex(...) +9
```

## API Signatures

```cp
fn ypIsNegativeNumber(s: string): bool;
fn ypFormatInt(valIn: i64, out: []u8): i64;
fn ypIsFlagToken(s: string): bool;
fn ypEmptyItem(): ParsedItem;
fn ypEmptyEntry(): ParsedEntry;
fn ypEntryMatchesKey(entry: ParsedEntry, query: string): bool;
fn ypPushPositional(args: &mut ParsedArgs, s: string);
fn ypPushRest(args: &mut ParsedArgs, s: string);
fn ypSetEntry(args: &mut ParsedArgs, rawKey: string, kind: u8, boolVal: bool, intVal: i64, strVal: string);
fn ypSetCoerced(args: &mut ParsedArgs, rawKey: string, rawVal: string);
fn parseArgs(argv: &[]string): ParsedArgs;
```

## Minimal Usage

```cp
import { parseArgs } from "yargs-parser";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"yargs-parser"`; install via `cap add yargs-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
