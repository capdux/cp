# shebang-command — AI Agent Quick Ref

Zero-allocation shebang executable command extractor reusing shebang-regex for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ShebangCmd,
    Parts,
    skipSpaces,
    scanTokenEnd,
    trimRight,
    basenameStart,
    locateParts,
    shebangCommand,
} from "shebang-command";
```

## Core Types & Constants

```cp
class ShebangCmd { ok: bool, bin_len: usize, arg_len: usize }
class Parts { ok: bool, bin_start: usize, bin_len: usize, arg_start: usize, /* +1 fields */ }
```

## API Signatures

```cp
fn skipSpaces(s: string, pos: usize, end: usize): usize;
fn scanTokenEnd(s: string, pos: usize, end: usize): usize;
fn trimRight(s: string, start: usize, end: usize): usize;
fn basenameStart(s: string, start: usize, end: usize): usize;
fn locateParts(input: string): Parts;
fn shebangCommand(input: string, out_cmd: []u8): usize;
fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd;
```

## Minimal Usage

```cp
import { parseShebangCmd } from "shebang-command";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = parseShebangCmd("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shebang-command"`; install via `cap add shebang-command`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
