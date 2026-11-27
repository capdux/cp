# magnet-url — AI Agent Quick Ref

Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 btih & btmh) parser, tracker extractor, and builder for CP (compatible with npm magnet-uri & Rust magnet-url) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MagnetLink,
    TopicMatch,
    startsWithIgnoreCase,
    isHexStr,
    isBase32Str,
    parsePositiveI64,
    classifyExactTopic,
    parseMagnet,
} from "magnet-url";
```

## Core Types & Constants

```cp
class MagnetLink { hash_kind: u8, info_hash: string, display_name: string, exact_length: i64, /* +3 fields */ }
class TopicMatch { kind: u8, hash: string }
```

## API Signatures

```cp
fn startsWithIgnoreCase(s: string, prefix: string): bool;
fn isHexStr(s: string): bool;
fn isBase32Str(s: string): bool;
fn parsePositiveI64(s: string): i64;
fn classifyExactTopic(xt: string): TopicMatch;
fn parseMagnet(uri: string): MagnetLink;
fn appendBytes(dst: []u8, pos: usize, src: string): usize;
fn formatMagnetV1(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isHexStr } from "magnet-url";

pub fn main() {
    let result = isHexStr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"magnet-url"`; install via `cap add magnet-url`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
