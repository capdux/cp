# accept-language-parser — AI Agent Quick Ref

Zero-heap RFC 9110 / BCP-47 `Accept-Language` parser and content-language negotiator in pure CP (npm `accept-language-parser` parity).

## Import

```cp
import {
    LangRange,
    parseQuality,
    parseAcceptLanguage,
    matchesTag,
    pickLanguage,
} from "accept-language-parser";
```

## API Signatures

```cp
class LangRange {
    tag: string,
    primary: string,
    region: string,
    quality: u16, // 0..=1000 (q=1.0 -> 1000, q=0.8 -> 800)
}

fn parseQuality(s: string): i32; // 0..=1000 or -1 on invalid qvalue
fn parseAcceptLanguage(header: string, out: []LangRange): usize;
fn matchesTag(range_tag: string, supported_tag: string): bool;
fn pickLanguage(header: string, supported: &[]string): string;
```

## Minimal Example

```cp
import { LangRange, parseAcceptLanguage, pickLanguage } from "accept-language-parser";

let ranges: [8]LangRange = undefined;
let n = parseAcceptLanguage("en-US,en;q=0.8,zh-CN;q=0.9", ranges[0..8]);
// ranges[0].tag == "en-US" (1000), ranges[1].tag == "zh-CN" (900)

let supported: [2]string = ["en", "zh-CN"];
let best = pickLanguage("zh-CN;q=0.9,en;q=0.8", supported[0..2]); // "zh-CN"
```

## Pitfalls & Borrowing Rules

- **Fixed-point quality scale**: `LangRange.quality` is `u16` scaled by 1000 (`1000` = `1.0`, `500` = `0.5`, `0` = excluded); `parseQuality` returns `-1` for invalid strings (e.g. `> 1.0`, `> 3` decimals, or non-digits).
- **No-match return**: `pickLanguage` returns an empty string `""` (check `best.len == 0`) when no supported language matches; matched returns borrow directly from `supported`.
- **Lifetime**: `LangRange` fields (`tag`, `primary`, `region`) are zero-copy slices borrowing from `header`; keep `header` alive while reading `ranges`.
- **Output cap**: `parseAcceptLanguage` writes at most `out.len` entries and sorts them in-place by descending `quality`.
- **Exclusions**: Entries with `q=0` are preserved in `parseAcceptLanguage` and respected as exclusions in `pickLanguage`.
