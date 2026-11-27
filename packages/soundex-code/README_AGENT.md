# soundex-code (AI Agent Quick Ref)

Zero-heap American Soundex phonetic encoder, variable-length/padded encoder, matcher, and 0..4 scorer.

## Import
```typescript
import {
    DEFAULT_SOUNDEX_LEN,
    soundexCharCode, soundex, soundexCode, soundexPadded,
    soundsLike, soundsLikeWithLength, soundexDifference,
} from "soundex-code";
```

## API Signatures
- `const DEFAULT_SOUNDEX_LEN: usize` (`4`)
- `soundexCharCode(ch: u8): u8` (`'1'..'6'` ASCII digit byte, or `0` for non-coded)
- `soundex(word: string, out: []u8): usize` (standard 4-char code; returns `4` or `0`)
- `soundexCode(word: string, max_length: usize, out: []u8): usize`
- `soundexPadded(word: string, length: usize, out: []u8): usize`
- `soundsLike(a: string, b: string): bool`
- `soundsLikeWithLength(a: string, b: string, max_length: usize): bool`
- `soundexDifference(a: string, b: string): u8` (`0..=4`)

## Minimal Example
```typescript
import { soundex, soundsLike, soundexDifference } from "soundex-code";

fn matchNames(a: string, b: string): bool {
    let code: [4]u8 = undefined;
    let n = soundex(a, code[0..]);
    return n == 4 && soundsLike(a, b);
}
```

## Pitfalls & Borrowing Rules
- `soundex` requires `out.len >= 4`; if `word` contains no ASCII letters (e.g. `""` or `"1234"`), it returns `0` and `soundsLike` returns `false`.
- `soundexCharCode(ch)` returns the ASCII byte `'1'`..`'6'` (`49..54`), NOT integer `1..6`, and returns `0` (null byte) for vowels/H/W/non-alpha.
- `soundexCode` pads with `'0'` only up to `min(4, max_length)`, whereas `soundexPadded` pads all the way to `length`.
- `out` must be a mutable byte slice (`[]u8`); all `string` inputs are read-only borrows with zero heap allocation.
- Only ASCII letters (`A-Z`, `a-z`) are processed; leading non-letters are skipped before taking the initial uppercase letter.
