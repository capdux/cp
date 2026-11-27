# css-what — AI Agent Quick Ref

Zero-allocation CSS selector tokenizer, parser, and serializer (Cheerio / css-select core) in pure CP (`v1.0.0`, `BSD-2-Clause`)

## Import

```cp
import {
    CssTokenKind,
    AttrAction,
    CssSelectorToken,
    CwhWriter,
    cwhIsIdentStart,
    cwhIsIdentChar,
    cwhSkipSpace,
    cwhScanIdent,
} from "css-what";
```

## Core Types & Constants

```cp
enum CssTokenKind { Tag, Universal, Id, Class, Attribute, Pseudo, ... }
enum AttrAction { Exists, Equals, Start, End, Any, Not, ... }
class CssSelectorToken { kind: CssTokenKind, name: string, value: string, action: AttrAction, /* +1 fields */ }
class CwhWriter { buf: []u8, pos: usize, overflow: bool } // methods: writeByte(...) +1
```

## API Signatures

```cp
fn cwhIsIdentStart(c: u8): bool;
fn cwhIsIdentChar(c: u8): bool;
fn cwhSkipSpace(s: string, start: usize): usize;
fn cwhScanIdent(s: string, start: usize): usize;
fn cwhIsCombinatorChar(c: u8): bool;
fn cwhToken(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken;
fn parseCssSelector(selector: string, out: []CssSelectorToken): i64;
fn stringifyCssSelector(tokens: &[]CssSelectorToken, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseCssSelector } from "css-what";

pub fn main() {
    let result = parseCssSelector("example", /* out: []CssSelectorToken */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"css-what"`; install via `cap add css-what`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
