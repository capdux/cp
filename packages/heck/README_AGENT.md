# heck — AI Agent Quick Ref

Zero-allocation Rust heck-compatible case conversion engine (PascalCase, camelCase, snake_case, SHOUTY_SNAKE_CASE, kebab-case, SHOUTY-KEBAB-CASE, Title Case, Train-Case) for CP (clean-room MIT). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HckRune,
    hckDecodeUtf8,
    hckClassify,
    hckToLowerCp,
    hckToUpperCp,
    hckEncodeUtf8,
    hckEmitWord,
    hckConvert,
} from "heck";
```

## Core Types & Constants

```cp
class HckRune { cp: u32, len: usize }
```

## API Signatures

```cp
fn hckDecodeUtf8(src: string, offset: usize): HckRune;
fn hckClassify(cp: u32): u8;
fn hckToLowerCp(cp: u32): u32;
fn hckToUpperCp(cp: u32): u32;
fn hckEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
fn hckEmitWord(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize;
fn hckConvert(src: string, out: []u8, sep: u8, style: u8): usize;
fn toUpperCamelCase(src: string, out: []u8): usize;
fn toLowerCamelCase(src: string, out: []u8): usize;
fn toSnakeCase(src: string, out: []u8): usize;
fn toShoutySnakeCase(src: string, out: []u8): usize;
fn toKebabCase(src: string, out: []u8): usize;
fn toShoutyKebabCase(src: string, out: []u8): usize;
fn toTitleCase(src: string, out: []u8): usize;
fn toTrainCase(src: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { hckDecodeUtf8 } from "heck";

pub fn main() {
    let result = hckDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"heck"`; install via `cap add heck`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
