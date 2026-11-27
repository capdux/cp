# parse-link-header — AI Agent Quick Ref

Zero-allocation RFC 8288 HTTP Link header parser and builder with GitHub/REST pagination helpers for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LinkEntry,
    LinkHeader,
    relMatches,
    skipToNextEntry,
    parseLinkHeader,
    appendByte,
    appendParam,
    formatLinkHeader,
} from "parse-link-header";
```

## Core Types & Constants

```cp
class LinkEntry { url: string, rel: string, title: string, type_val: string, /* +1 fields */ } // methods: empty(...)
class LinkHeader { entries: [32]LinkEntry, count: usize } // methods: empty(...) +6
```

## API Signatures

```cp
fn relMatches(rel_attr: string, target: string): bool;
fn skipToNextEntry(s: string, start: usize): usize;
fn parseLinkHeader(header: string): LinkHeader;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendParam(out: []u8, pos: usize, name: string, val: string): usize;
fn formatLinkHeader(entries: []LinkEntry, out: []u8): string;
fn appendPageLink(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize;
fn formatPaginationLinks(base_url: string, page: usize, total_pages: usize, out: []u8): string;
```

## Minimal Usage

```cp
import { parseLinkHeader } from "parse-link-header";

pub fn main() {
    let result = parseLinkHeader("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parse-link-header"`; install via `cap add parse-link-header`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
