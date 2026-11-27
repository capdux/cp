# convert-source-map — AI Agent Quick Ref

Zero-allocation inline sourceMappingURL comment extractor, Base64 decoder/encoder, and comment stripper (clean-room MIT implementation of npm convert-source-map, #96 / 307M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SourceMapCommentKind,
    SourceMapComment,
    csmStartsWith,
    csmPush,
    csmB64Val,
    csmB64Encode,
    csmB64Decode,
    findSourceMapComment,
} from "convert-source-map";
```

## Core Types & Constants

```cp
enum SourceMapCommentKind { None, InlineBase64, ExternalFile }
class SourceMapComment { kind: SourceMapCommentKind, comment_start: usize, comment_end: usize, payload: string }
```

## API Signatures

```cp
fn csmStartsWith(s: string, pos: usize, lit: string): bool;
fn csmPush(out: []u8, pos: usize, s: string): usize;
fn csmB64Val(b: u8): i32;
fn csmB64Encode(data: &[]u8, out: []u8, start_pos: usize): usize;
fn csmB64Decode(b64: string, out: []u8): i64;
fn findSourceMapComment(src: string): SourceMapComment;
fn fromSource(src: string, out: []u8): i64;
fn toComment(json_map: string, multiline: bool, out: []u8): usize;
fn removeComments(src: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { csmStartsWith } from "convert-source-map";

pub fn main() {
    let result = csmStartsWith("example", 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"convert-source-map"`; install via `cap add convert-source-map`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
