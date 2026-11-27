# terser — AI Agent Quick Ref

Zero-allocation lexer-safe JS/TS/CSS code minifier (npm terser parity) preserving strings, templates, legal comments, keyword spacing, and unary operator separation for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MinifyOptions,
    MinifyStats,
    tmIsSpace,
    tmIsWord,
    tmNeedsJsSpace,
    tmContainsSubstr,
    tmIsLegalComment,
    tmIsCssPunct,
} from "terser";
```

## Core Types & Constants

```cp
class MinifyOptions { strip_comments: bool, preserve_legal_comments: bool, collapse_whitespace: bool, keep_newlines: bool } // methods: defaults(...)
class MinifyStats { input_len: usize, output_len: usize, comments_removed: usize, ok: bool }
```

## API Signatures

```cp
fn tmIsSpace(ch: u8): bool;
fn tmIsWord(ch: u8): bool;
fn tmNeedsJsSpace(prev: u8, next: u8): bool;
fn tmContainsSubstr(s: string, needle: string): bool;
fn tmIsLegalComment(comment: string): bool;
fn tmIsCssPunct(ch: u8): bool;
fn minifyJs(input: string, opts: MinifyOptions, out: []u8): MinifyStats;
fn minifyCss(input: string, out: []u8): MinifyStats;
```

## Minimal Usage

```cp
import { tmContainsSubstr } from "terser";

pub fn main() {
    let result = tmContainsSubstr("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"terser"`; install via `cap add terser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
