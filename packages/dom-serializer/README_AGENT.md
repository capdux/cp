# dom-serializer — AI Agent Quick Ref

Zero-heap HTML/XML DOM node serializer (covers domelementtype and w3c-xmlserializer) (`v2.0.0`, `MIT`)

## Import

```cp
import {
    DomNodeKind,
    DomAttr,
    SerializeOptions,
    dsrlWriteByte,
    dsrlWriteAttrValue,
    isVoidElement,
    serializeText,
    serializeOpenTag,
} from "dom-serializer";
```

## Core Types & Constants

```cp
enum DomNodeKind { Element, Text, Comment, Cdata, Doctype }
class DomAttr { name: string, value: string }
class SerializeOptions { xml_mode: bool, self_closing_tags: bool }
```

## API Signatures

```cp
fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize;
fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize;
fn isVoidElement(tag: string): bool;
fn serializeText(text: string, out: []u8): usize;
fn serializeOpenTag(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize;
fn serializeElement(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize;
fn serializeComment(text: string, out: []u8): usize;
fn serializeCdata(text: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isVoidElement } from "dom-serializer";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dom-serializer"`; install via `cap add dom-serializer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
