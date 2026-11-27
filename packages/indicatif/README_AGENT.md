# indicatif — AI Agent Quick Ref

Zero-heap terminal progress bar, throughput, and ETA renderer composing pretty-bytes and pretty-ms in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ProgressState,
    indWriteByte,
    indWriteStr,
    indWriteU64,
    percent,
    etaMs,
    bytesPerSec,
    renderBar,
} from "indicatif";
```

## Core Types & Constants

```cp
class ProgressState { pos: u64, total: u64, elapsed_ms: u64, width: usize }
```

## API Signatures

```cp
fn indWriteByte(out: []u8, pos: usize, b: u8): usize;
fn indWriteStr(out: []u8, pos: usize, s: &[]u8): usize;
fn indWriteU64(out: []u8, pos: usize, val: u64): usize;
fn percent(s: ProgressState): f64;
fn etaMs(s: ProgressState): u64;
fn bytesPerSec(s: ProgressState): f64;
fn renderBar(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize;
fn renderLine(s: ProgressState, prefix: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { renderLine } from "indicatif";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderLine(/* s: ProgressState */, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"indicatif"`; install via `cap add indicatif`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
