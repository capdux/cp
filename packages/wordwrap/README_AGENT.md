# wordwrap — AI Agent Quick Ref

Zero-heap column-width text word wrapper with indentation, soft/hard modes, and newline preservation in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WwrpEmitRes,
    wwrpIsInlineWs,
    wwrpPushByte,
    wwrpPushSpaces,
    wwrpPushSlice,
    wwrpEmitWord,
    wrapText,
} from "wordwrap";
```

## Core Types & Constants

```cp
class WwrpEmitRes { pos: usize, line_len: usize }
```

## API Signatures

```cp
fn wwrpIsInlineWs(c: u8): bool;
fn wwrpPushByte(out: []u8, pos: usize, b: u8): usize;
fn wwrpPushSpaces(out: []u8, pos: usize, count: usize): usize;
fn wwrpPushSlice(out: []u8, pos: usize, s: string): usize;
fn wwrpEmitWord(word: string, start_col: usize, max_w: usize, hard_mode: bool, out: []u8, start_pos: usize, cur_line_len: usize): WwrpEmitRes;
fn wrapText(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { wwrpPushSlice } from "wordwrap";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wwrpPushSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wordwrap"`; install via `cap add wordwrap`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
