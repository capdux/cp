# cliui — AI Agent Quick Ref

Zero-allocation multi-column CLI layout engine with ANSI-aware wrapping, alignment, and padding in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Align,
    Column,
    CliUi,
    alignToU8,
    cuWriteByte,
    cuWriteBytes,
    cuWriteSpaces,
    cuVisibleWidth,
} from "cliui";
```

## Core Types & Constants

```cp
enum Align { Left, Right, Center }
class Column { text: string, width: usize, alignment: u8, pad_left: usize, /* +2 fields */ } // methods: plain(...) +1
class CliUi { width: usize, len: usize, row_count: usize, in_span: bool, /* +1 fields */ } // methods: new(...) +4
```

## API Signatures

```cp
fn alignToU8(a: Align): u8;
fn cuWriteByte(out: []u8, pos: usize, b: u8): usize;
fn cuWriteBytes(out: []u8, pos: usize, s: string): usize;
fn cuWriteSpaces(out: []u8, pos: usize, count: usize): usize;
fn cuVisibleWidth(s: string): usize;
fn cuCountLines(s: string): usize;
fn cuGetLine(s: string, target_line: usize): string;
fn cuResolveWidths(total_width: usize, cols: &[]Column, n_cols: usize, widths: []usize);
fn formatTwoCol(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatTwoCol } from "cliui";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatTwoCol("example", "example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cliui"`; install via `cap add cliui`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
