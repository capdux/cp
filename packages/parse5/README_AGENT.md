# parse5 — AI Agent Quick Ref

Zero-heap WHATWG HTML5 streaming tokenizer, attribute extractor, and text stripper (clean-room port of npm parse5) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HtmlToken,
    HtmlAttr,
    HtmlTokenizer,
    p5IsWs,
    p5IsTagNameChar,
    p5Trim,
    p5FindSubstr,
    isVoidElement,
} from "parse5";
```

## Core Types & Constants

```cp
class HtmlToken { kind: u8, name: string, raw_attrs: string, text: string, /* +2 fields */ }
class HtmlAttr { name: string, value: string }
class HtmlTokenizer { src: string, pos: usize } // methods: new(...) +1
```

## API Signatures

```cp
fn p5IsWs(b: u8): bool;
fn p5IsTagNameChar(b: u8): bool;
fn p5Trim(s: string): string;
fn p5FindSubstr(s: string, start_pos: usize, needle: string): usize;
fn isVoidElement(tag: string): bool;
fn parseAttributes(raw_attrs: string, out_attrs: []HtmlAttr): usize;
fn extractTextContent(html: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isVoidElement } from "parse5";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parse5"`; install via `cap add parse5`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
