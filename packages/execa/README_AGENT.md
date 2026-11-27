# execa — AI Agent Quick Ref

Zero-heap subprocess command builder, POSIX/Windows argument escaper, preferLocal PATH resolver, and ExecaResult inspector (npm execa equivalent, reuses npm-run-path) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ExecaOptions,
    ExecaResult,
    exaPush,
    exaPushByte,
    exaNeedsQuote,
    escapePosixArg,
    formatCommand,
    stripFinalNewline,
} from "execa";
```

## Core Types & Constants

```cp
class ExecaOptions { cwd: string, path_env: string, prefer_local: bool, reject: bool, /* +3 fields */ } // methods: defaults(...)
class ExecaResult { exit_code: i32, signal_code: i32, timed_out: bool, is_canceled: bool, /* +3 fields */ } // methods: success(...) +3
```

## API Signatures

```cp
fn exaPush(out: []u8, pos: usize, s: string): usize;
fn exaPushByte(out: []u8, pos: usize, b: u8): usize;
fn exaNeedsQuote(arg: string): bool;
fn escapePosixArg(arg: string, out: []u8): usize;
fn formatCommand(file: string, args: []string, out: []u8): usize;
fn stripFinalNewline(s: string): string;
fn resolveExecPath(opts: ExecaOptions, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatCommand } from "execa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCommand("example", /* args: []string */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"execa"`; install via `cap add execa`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
