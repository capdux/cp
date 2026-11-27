# http-digest-client — AI Agent Quick Ref

Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DigestChallenge,
    DigestResponse,
    digIsSpace,
    trimSlice,
    stripDigestPrefix,
    unquoteVal,
    parseChallenge,
    parseAuthorization,
} from "http-digest-client";
```

## Core Types & Constants

```cp
class DigestChallenge { realm: string, nonce: string, opaque_val: string, qop: string, /* +3 fields */ }
class DigestResponse { username: string, realm: string, nonce: string, uri: string, /* +6 fields */ }
```

## API Signatures

```cp
fn digIsSpace(b: u8): bool;
fn trimSlice(s: string): string;
fn stripDigestPrefix(header: string): string;
fn unquoteVal(v: string): string;
fn parseChallenge(header: string): DigestChallenge;
fn parseAuthorization(header: string): DigestResponse;
fn supportsQop(qop_list: string, target: string): bool;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushQuotedField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
fn pushTokenField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
fn formatAuthorization(resp: DigestResponse, out: []u8): usize;
fn formatChallenge(ch: DigestChallenge, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseChallenge } from "http-digest-client";

pub fn main() {
    let result = parseChallenge("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http-digest-client"`; install via `cap add http-digest-client`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
