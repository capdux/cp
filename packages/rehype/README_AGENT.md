# rehype — AI Agent Quick Ref

Zero-allocation HAST (Hypertext Abstract Syntax Tree) builder & HTML5 serializer (npm rehype/hast parity) composing escape-html for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HastAttr,
    HastNode,
    HastTree,
    rhWriteByte,
    isVoidElement,
    rhMakeNode,
    rhSerializeHtml,
    rhExtractText,
} from "rehype";
```

## Core Types & Constants

```cp
class HastAttr { name: [48]u8, name_len: usize, value: [192]u8, value_len: usize, /* +1 fields */ }
class HastNode { kind: u8, text: [256]u8, text_len: usize, first_child: i32, /* +4 fields */ }
class HastTree { nodes: [64]HastNode, node_len: usize, attrs: [64]HastAttr, attr_len: usize } // methods: new(...) +9
```

## API Signatures

```cp
fn rhWriteByte(out: []u8, pos: usize, b: u8): usize;
fn isVoidElement(tag: string): bool;
fn rhMakeNode(kind: u8, text: string): HastNode;
fn rhSerializeHtml(nodes: &[]HastNode, attrs: &[]HastAttr, node_id: usize, out: []u8, pos: usize, depth: usize): usize;
fn rhExtractText(nodes: &[]HastNode, node_id: usize, out: []u8, pos: usize, depth: usize): usize;
```

## Minimal Usage

```cp
import { isVoidElement } from "rehype";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rehype"`; install via `cap add rehype`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
