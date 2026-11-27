# toml_edit — AI Agent Quick Ref

Zero-allocation TOML v1.0 document parser, typed query/mutation engine, and canonical formatter (Rust toml_edit v0.22 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TomlEntry,
    TomlDoc,
    tmeStripComment,
    tmeParseInt,
    tmeWriteByte,
    tmeWriteQuotedStr,
    tmeWriteEntry,
} from "toml_edit";
```

## Core Types & Constants

```cp
class TomlEntry { section: string, key: string, kind: u8, str_val: string, /* +2 fields */ } // methods: empty(...)
class TomlDoc { entries: [64]TomlEntry, count: usize } // methods: new(...) +10
```

## API Signatures

```cp
fn tmeStripComment(s: string): string;
fn tmeParseInt(raw: string): i64?;
fn tmeWriteByte(out: []u8, pos: usize, b: u8): usize;
fn tmeWriteQuotedStr(out: []u8, start_pos: usize, s: string): usize;
fn tmeWriteEntry(out: []u8, start_pos: usize, e: TomlEntry): usize;
```

## Minimal Usage

```cp
import { tmeStripComment } from "toml_edit";

pub fn main() {
    let result = tmeStripComment("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"toml_edit"`; install via `cap add toml_edit`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
