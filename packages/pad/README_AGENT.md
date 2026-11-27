# `pad` — AI Agent Quick Reference

- **Package**: `pad` (`1.0.0`, MIT)
- **Origin**: `npm:pad` + `crates.io:pad` + `npm:left-pad`
- **Import**: `import { pad, padStart, padEnd, padLeft, padRight, padCenter, padCenterWith, PadOptions, repeatStr, countChars, truncateChars } from "pad";`

## Types

```cp
class PadOptions {
    pad_char: u8,
    pad_str: string,
    truncate: bool,
    left: bool,
    fn defaults(): PadOptions,
}
```

## Core Signatures

```cp
// High-level padding (UTF-8 codepoint-aware)
fn pad(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize
fn padCenterWith(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize
fn padStart(s: string, target_chars: usize, pad_char: u8, out: []u8): usize
fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8): usize
fn padLeft(s: string, target_chars: usize, pad_char: u8, out: []u8): usize
fn padRight(s: string, target_chars: usize, pad_char: u8, out: []u8): usize
fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8): usize

// Repetition, UTF-8 utilities & low-level helpers
fn repeatStr(s: string, count: usize, out: []u8): usize
fn countChars(s: string): usize
fn truncateChars(s: string, max_chars: usize): string
fn calcPadBytes(pad_chars: usize, pad_char: u8, pad_str: string): usize
fn writePadPattern(dst: []u8, pos: usize, pad_chars: usize, pad_char: u8, pad_str: string): usize
fn copyBytes(dst: []u8, pos: usize, src: string)
fn fillByte(dst: []u8, pos: usize, count: usize, ch: u8)
```

## Minimal Pattern

```cp
let buf: [64]u8 = undefined;
let opts = PadOptions.defaults();
opts.pad_str = ".";
opts.left = true;
let n = pad("42", 5, opts, buf[0..64]); // "...42"
let result = buf[0..n] as string;
```

## Pitfalls

1. **`target_chars` is codepoints, not bytes**: UTF-8 multi-byte chars count as 1 each; ensure `out` has enough bytes (use `calcPadBytes` if unsure).
2. **`pad_str` overrides `pad_char`**: In `PadOptions`, if `pad_str` is non-empty, `pad_str` is used instead of `pad_char`.
3. **Return value is byte length**: Always slice `out[0..n] as string` using the returned `usize`.
4. **Zero-copy truncation**: `truncateChars` returns a slice into the original `string` without copying.
