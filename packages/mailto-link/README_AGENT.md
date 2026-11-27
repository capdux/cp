# mailto-link — AI Agent Quick Ref

Zero-allocation RFC 6068 mailto: URI parser, recipient counter, email checker, and percent-encoding builder in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MailtoUri,
    invalidMailto,
    mailtoIsSpace,
    trimSlice,
    parseMailto,
    countRecipients,
    isBasicEmail,
    isUnreserved,
} from "mailto-link";
```

## Core Types & Constants

```cp
class MailtoUri { to: string, cc: string, bcc: string, subject: string, /* +2 fields */ }
```

## API Signatures

```cp
fn invalidMailto(): MailtoUri;
fn mailtoIsSpace(b: u8): bool;
fn trimSlice(s: string): string;
fn parseMailto(uri: string): MailtoUri;
fn countRecipients(to_list: string): usize;
fn isBasicEmail(addr: string): bool;
fn isUnreserved(b: u8): bool;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushPctEncoded(dst: []u8, pos: usize, src: string, allow_addr_chars: bool): i64;
fn formatMailto(to: string, subject: string, body: string, cc: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseMailto } from "mailto-link";

pub fn main() {
    let result = parseMailto("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mailto-link"`; install via `cap add mailto-link`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
