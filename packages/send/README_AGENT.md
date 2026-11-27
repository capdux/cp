# send — AI Agent Quick Ref

Zero-heap pure CP static file transfer planner with traversal protection, dotfiles policy, ETag/304, Range/206, and redirect URL encoding (clean-room MIT implementation of npm send v1) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SendOptions,
    SendPlan,
    sndStrEq,
    sndIsWs,
    sndTrimWs,
    sndHasMaliciousPath,
    sndContainsDotfile,
    sndIfNoneMatchMatches,
} from "send";
```

## Core Types & Constants

```cp
class SendOptions { root: string, dotfiles: u8, etag: bool, accept_ranges: bool, /* +2 fields */ } // methods: defaults(...)
class SendPlan { status: u16, content_type: string, is_partial: bool, range_start: u64, /* +3 fields */ } // methods: errorPlan(...)
```

## API Signatures

```cp
fn sndStrEq(a: string, b: string): bool;
fn sndIsWs(b: u8): bool;
fn sndTrimWs(s: string): string;
fn sndHasMaliciousPath(req_path: string): bool;
fn sndContainsDotfile(req_path: string): bool;
fn sndIfNoneMatchMatches(if_none_match: string, current_etag: string): bool;
fn planSendFile(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan;
fn formatRedirectUrl(path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatRedirectUrl } from "send";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatRedirectUrl("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"send"`; install via `cap add send`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
