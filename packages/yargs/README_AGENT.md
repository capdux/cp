# yargs — AI Agent Quick Ref

Zero-allocation CLI application framework (npm yargs v17 parity) composing yargs-parser, cliui, and y18n for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OptionSpec,
    CommandSpec,
    YargsResult,
    YargsApp,
    ygStrEq,
    ygAppendBytes,
    ygAppendByte,
    ygCopyBuf,
} from "yargs";
```

## Core Types & Constants

```cp
class OptionSpec { name: string, alias_ch: u8, desc: string, kind: u8, /* +4 fields */ } // methods: boolOpt(...) +2
class CommandSpec { name: string, desc: string }
class YargsResult { args: ParsedArgs, command: [64]u8, command_len: usize, show_help: bool, /* +4 fields */ } // methods: hasCommand(...) +10
class YargsApp { script_name: string, usage_text: string, version_str: string, commands: [16]CommandSpec, /* +4 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn ygStrEq(a: string, b: string): bool;
fn ygAppendBytes(dst: []u8, pos: usize, s: string): usize;
fn ygAppendByte(dst: []u8, pos: usize, b: u8): usize;
fn ygCopyBuf(dst: []u8, src: string): usize;
fn ygInsertEntry(parsed: &mut ParsedArgs, name: string, kind: u8, bool_val: bool, int_val: i64, str_val: string);
```

## Minimal Usage

```cp
import { ygStrEq } from "yargs";

pub fn main() {
    let result = ygStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"yargs"`; install via `cap add yargs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
