# syslog — AI Agent Quick Ref

Zero-heap RFC 5424 Syslog message parser and formatter with PRI facility/severity codec, structured data skipping, and NILVALUE handling. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SyslogMessage,
    TokenSpan,
    encodePri,
    decodeFacility,
    decodeSeverity,
    readSpToken,
    parseRfc5424,
    appendSlice,
} from "syslog";
```

## Core Types & Constants

```cp
class SyslogMessage { facility: u8, severity: u8, version: u8, ts_start: usize, /* +12 fields */ }
class TokenSpan { start: usize, len: usize, next_pos: usize, ok: bool }
```

## API Signatures

```cp
fn encodePri(facility: u8, severity: u8): u8;
fn decodeFacility(pri: u8): u8;
fn decodeSeverity(pri: u8): u8;
fn readSpToken(raw: &[]u8, pos: usize): TokenSpan;
fn parseRfc5424(raw: &[]u8): SyslogMessage;
fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
fn appendFieldOrNil(dst: []u8, pos: usize, field: &[]u8): i64;
fn appendU8Dec(dst: []u8, pos: usize, val: u8): i64;
fn formatRfc5424(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { encodePri } from "syslog";

pub fn main() {
    let result = encodePri(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"syslog"`; install via `cap add syslog`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
