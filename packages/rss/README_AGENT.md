# rss — AI Agent Quick Ref

Zero-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RssChannel,
    RssItem,
    RssWriter,
    pushSlice,
    pushXmlEscaped,
    pushTag,
    extractTagValue,
    countItems,
} from "rss";
```

## Core Types & Constants

```cp
class RssChannel { title: &[]u8, link: &[]u8, description: &[]u8, language: &[]u8 }
class RssItem { title: &[]u8, link: &[]u8, description: &[]u8, guid: &[]u8, /* +1 fields */ }
class RssWriter { buf: []u8, pos: usize, item_count: usize, failed: bool, /* +1 fields */ } // methods: init(...) +4
```

## API Signatures

```cp
fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
fn pushXmlEscaped(dst: []u8, pos: usize, src: &[]u8): i64;
fn pushTag(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64;
fn extractTagValue(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8;
fn countItems(xml: &[]u8): usize;
```

## Minimal Usage

```cp
import { pushSlice } from "rss";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = pushSlice(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rss"`; install via `cap add rss`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
