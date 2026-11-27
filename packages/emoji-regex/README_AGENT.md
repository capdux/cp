# emoji-regex — AI Agent Quick Ref

Zero-allocation Unicode RGI Emoji UTF-8 scanner and extractor (npm emoji-regex parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EmojiSpan,
    EmjDecoded,
    emjDecodeUtf8,
    emjIsRegionalIndicator,
    emjIsSkinTone,
    emjIsKeycapBase,
    emjIsTagChar,
    isEmojiCodePoint,
} from "emoji-regex";
```

## Core Types & Constants

```cp
class EmojiSpan { start: usize, end: usize }
class EmjDecoded { cp: u32, len: usize }
```

## API Signatures

```cp
fn emjDecodeUtf8(s: string, pos: usize): EmjDecoded;
fn emjIsRegionalIndicator(cp: u32): bool;
fn emjIsSkinTone(cp: u32): bool;
fn emjIsKeycapBase(cp: u32): bool;
fn emjIsTagChar(cp: u32): bool;
fn isEmojiCodePoint(cp: u32): bool;
fn emjConsumeModifiers(s: string, start_pos: usize): usize;
fn findNextEmoji(s: string, from_pos: usize): EmojiSpan?;
fn countEmojis(s: string): usize;
fn containsEmoji(s: string): bool;
fn stripEmojis(s: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { emjDecodeUtf8 } from "emoji-regex";

pub fn main() {
    let result = emjDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"emoji-regex"`; install via `cap add emoji-regex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
