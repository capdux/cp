# postcss — AI Agent Quick Ref

Zero-heap CSS AST parser, query/mutation engine, and serializer for CP (clean-room MIT implementation of npm postcss v8). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PostCssDecl,
    PostCssNode,
    PostCssComment,
    PostCssSheet,
    pcCopyTrimmed,
    pcAppendSlice,
    pcAppendByte,
    pcIsDeclAtRule,
} from "postcss";
```

## Core Types & Constants

```cp
class PostCssDecl { kind: u8, parent_node: i32, prop: [48]u8, prop_len: usize, /* +3 fields */ } // methods: empty(...) +2
class PostCssNode { kind: u8, parent_node: i32, name: [64]u8, name_len: usize, /* +3 fields */ } // methods: empty(...) +2
class PostCssComment { kind: u8, text: [128]u8, text_len: usize } // methods: empty(...) +1
class PostCssSheet { nodes: [32]PostCssNode, node_len: usize, decls: [64]PostCssDecl, decl_len: usize, /* +3 fields */ } // methods: empty(...) +9
```

## API Signatures

```cp
fn pcCopyTrimmed(src: string, dst: []u8): usize;
fn pcAppendSlice(out: []u8, pos: &mut usize, src: string);
fn pcAppendByte(out: []u8, pos: &mut usize, b: u8);
fn pcIsDeclAtRule(name: string): bool;
fn pcAddComment(sheet: &mut PostCssSheet, raw_text: &[]u8);
fn pcSkipWsAndComments(css: string, pos: &mut usize, end: usize, sheet: &mut PostCssSheet): bool;
fn pcScanString(css: string, pos: &mut usize, end: usize, sheet: &mut PostCssSheet): bool;
fn pcExtractCleanText(css: string, start: usize, end: usize, dst: []u8, sheet: &mut PostCssSheet): usize;
fn pcStripImportant(buf: []u8, len: &mut usize): bool;
fn pcParseDecls(css: string, start: usize, end: usize, parent_node: i32, sheet: &mut PostCssSheet);
fn pcFindMatchingBrace(css: string, open_pos: usize, end: usize, out_close: &mut usize, sheet: &mut PostCssSheet): bool;
fn pcParseBlockList(css: string, start: usize, end: usize, parent_node: i32, depth: usize, sheet: &mut PostCssSheet);
fn pcSerializeNode(sheet: PostCssSheet, node_idx: usize, indent: usize, out: []u8, pos: &mut usize);
```

## Minimal Usage

```cp
import { pcCopyTrimmed } from "postcss";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pcCopyTrimmed("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"postcss"`; install via `cap add postcss`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
