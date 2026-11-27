# morgan — AI Agent Quick Ref

Zero-heap HTTP access log template formatter supporting Morgan tiny/dev/short/common/combined presets and custom tokens (clean-room port of morgan v1.10) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HttpLogEvent,
    mrgIsTokenChar,
    mrgWriteByte,
    mrgWriteOrDash,
    mrgWriteResponseTime,
    formatMorganLog,
    formatPreset,
} from "morgan";
```

## Core Types & Constants

```cp
class HttpLogEvent { remote_addr: string, remote_user: string, date: string, method: string, /* +8 fields */ }
```

## API Signatures

```cp
fn mrgIsTokenChar(c: u8): bool;
fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize;
fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize;
fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize;
fn formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize;
fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatMorganLog } from "morgan";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatMorganLog("example", /* ev: HttpLogEvent */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"morgan"`; install via `cap add morgan`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
