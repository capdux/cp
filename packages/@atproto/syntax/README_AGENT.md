# @atproto/syntax — AI Agent Quick Ref

Zero-allocation AT Protocol / Bluesky at:// URI, NSID, and Record Key (rkey) validator, parser, and formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AtUri,
    invalidAtUri,
    isLowerAlpha,
    isAlpha,
    isDigit,
    isAlphaNum,
    isValidNsid,
    isValidRkey,
} from "@atproto/syntax";
```

## Core Types & Constants

```cp
class AtUri { authority: string, collection: string, rkey: string, is_did: bool, /* +1 fields */ }
```

## API Signatures

```cp
fn invalidAtUri(): AtUri;
fn isLowerAlpha(b: u8): bool;
fn isAlpha(b: u8): bool;
fn isDigit(b: u8): bool;
fn isAlphaNum(b: u8): bool;
fn isValidNsid(nsid: string): bool;
fn isValidRkey(rkey: string): bool;
fn isValidDid(s: string): bool;
fn isValidHandle(s: string): bool;
fn parseAtUri(uri: string): AtUri;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn formatAtUri(authority: string, collection: string, rkey: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidNsid } from "@atproto/syntax";

pub fn main() {
    let result = isValidNsid("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@atproto/syntax"`; install via `cap add @atproto/syntax`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
