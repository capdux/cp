# saxes — AI Agent Quick Ref

Zero-allocation streaming SAX XML 1.0 event parser with tag stack validation and entity decoding (npm saxes parity) for CP (`v1.0.0`, `ISC`)

## Import

```cp
import {
    XmlEventKind,
    XmlAttr,
    XmlEvent,
    SaxesParser,
    saxEmptyAttrs,
    saxMakeEvent,
    saxIsWs,
    saxIsNameStart,
} from "saxes";
```

## Core Types & Constants

```cp
enum XmlEventKind { OpenTag, CloseTag, SelfClosingTag, Text, Cdata, Comment, ... }
class XmlAttr { name: string, value: string }
class XmlEvent { kind: XmlEventKind, name: string, text: string, attrs: [8]XmlAttr, /* +1 fields */ }
class SaxesParser { stack: [32]string, depth: usize, has_err: bool, root_count: usize } // methods: new(...) +3
```

## API Signatures

```cp
fn saxEmptyAttrs(): [8]XmlAttr;
fn saxMakeEvent(kind: XmlEventKind, name: string, text: string): XmlEvent;
fn saxIsWs(ch: u8): bool;
fn saxIsNameStart(ch: u8): bool;
fn saxIsNameChar(ch: u8): bool;
fn saxStrEq(a: string, b: string): bool;
fn saxStartsWithAt(s: string, pos: usize, prefix: string): bool;
fn saxFindSubstrFrom(s: string, start: usize, needle: string): i64;
fn saxAllWs(s: string): bool;
fn saxEmit(out: []XmlEvent, count: usize, ev: XmlEvent): usize;
fn saxEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
fn saxHexVal(ch: u8): i32;
fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { saxMakeEvent } from "saxes";

pub fn main() {
    let result = saxMakeEvent(/* kind: XmlEventKind */, "example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"saxes"`; install via `cap add saxes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
