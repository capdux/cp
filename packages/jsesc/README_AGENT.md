# jsesc — AI Agent Quick Ref

Zero-allocation JavaScript & JSON string escaper with ES6 code point, script-context, and minimal modes (clean-room MIT implementation of npm jsesc v3, #148 / 250M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    JsescOptions,
    JescDecoded,
    jescHexChar,
    jescPush,
    jescPush2,
    jescPushHex2,
    jescPushUnicode4,
    jescPushEs6CodePoint,
} from "jsesc";
```

## Core Types & Constants

```cp
class JsescOptions { quote: u8, wrap: bool, escape_everything: bool, minimal: bool, /* +4 fields */ } // methods: default(...) +1
class JescDecoded { cp: u32, len: usize }
```

## API Signatures

```cp
fn jescHexChar(nibble: u32, lowercase: bool): u8;
fn jescPush(out: []u8, pos: usize, b: u8): usize;
fn jescPush2(out: []u8, pos: usize, b0: u8, b1: u8): usize;
fn jescPushHex2(out: []u8, pos: usize, val: u32, lowercase: bool): usize;
fn jescPushUnicode4(out: []u8, pos: usize, val: u32, lowercase: bool): usize;
fn jescPushEs6CodePoint(out: []u8, pos: usize, cp: u32, lowercase: bool): usize;
fn jescDecodeUtf8(input: string, idx: usize): JescDecoded;
fn jescAsciiLower(b: u8): u8;
fn jescMatchesHtmlComment(input: string, idx: usize): bool;
fn jescMatchesCloseScript(input: string, idx: usize): bool;
fn jescEscapeCodePoint(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize;
fn jsescWithOptions(input: string, opts: JsescOptions, out: []u8): usize;
fn jsesc(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { jescDecodeUtf8 } from "jsesc";

pub fn main() {
    let result = jescDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsesc"`; install via `cap add jsesc`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
